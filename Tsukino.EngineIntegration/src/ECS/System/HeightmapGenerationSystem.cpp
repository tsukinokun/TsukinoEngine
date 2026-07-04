//-------------------------------------------------------------
//! @file   HeightmapGenerationSystem.cpp
//! @brief  HeightmapGenerationSystemクラスの実装
//! @author 山﨑愛
//-------------------------------------------------------------
#include <Tsukino/EngineIntegration/ECS/System/HeightmapGenerationSystem.hpp>

#include <Tsukino/BuiltIn/ECS/Component/TerrainGenerationRequestComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/CollisionComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/ModelComponent.hpp>

#include <Tsukino/EngineIntegration/EngineContext.hpp>
#include <Tsukino/Engine/Asset/AssetManager.hpp>

#include <Tsukino/Engine/Asset/Model/ModelAsset.hpp>
#include <Tsukino/Core/Log.hpp>
#include <Tsukino/Core/Math/Matrix.hpp>

#include <cmath>
#include <vector>
#include <optional>

// 名前空間 :Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {

    namespace {
        //-------------------------------------------------------------
        //! @brief  座標とシードから決定論的な疑似乱数値(0.0～1.0)を得るハッシュ関数
        //-------------------------------------------------------------
        float Hash(int32_t x, int32_t y, uint32_t seed) {
            uint32_t h = (uint32_t)x * 374761393u + (uint32_t)y * 668265263u + seed * 2654435761u;
            h          = (h ^ (h >> 13)) * 1274126177u;
            h          = h ^ (h >> 16);
            return (float)(h & 0x00FFFFFFu) / (float)0x00FFFFFFu;
        }

        //-------------------------------------------------------------
        //! @brief  滑らかな補間カーブ（3t^2 - 2t^3）
        //-------------------------------------------------------------
        float SmoothStep(float t) {
            return t * t * (3.0f - 2.0f * t);
        }

        float Lerp(float a, float b, float t) {
            return a + (b - a) * t;
        }

        //-------------------------------------------------------------
        //! @brief  2次元ヴァリューノイズ（格子点をハッシュし、補間して滑らかにする）
        //-------------------------------------------------------------
        float ValueNoise2D(float x, float y, uint32_t seed) {
            int32_t x0 = (int32_t)std::floor(x);
            int32_t y0 = (int32_t)std::floor(y);
            int32_t x1 = x0 + 1;
            int32_t y1 = y0 + 1;

            float sx = SmoothStep(x - (float)x0);
            float sy = SmoothStep(y - (float)y0);

            float n00 = Hash(x0, y0, seed);
            float n10 = Hash(x1, y0, seed);
            float n01 = Hash(x0, y1, seed);
            float n11 = Hash(x1, y1, seed);

            float ix0 = Lerp(n00, n10, sx);
            float ix1 = Lerp(n01, n11, sx);
            return Lerp(ix0, ix1, sy);
        }

        //-------------------------------------------------------------
        //! @brief  複数オクターブを重ねたフラクタルノイズ（fBm）。戻り値は0.0～1.0に正規化
        //-------------------------------------------------------------
        float FractalNoise2D(float x, float y, uint32_t seed, int32_t octaves, float frequency, float persistence) {
            float amplitude = 1.0f;
            float total     = 0.0f;
            float maxValue  = 0.0f;
            float freq      = frequency;

            for(int32_t i = 0; i < octaves; ++i) {
                total     += ValueNoise2D(x * freq, y * freq, seed + (uint32_t)i * 1013u) * amplitude;
                maxValue  += amplitude;
                amplitude *= persistence;
                freq      *= 2.0f;
            }
            return maxValue > 0.0f ? total / maxValue : 0.0f;
        }
    }    // namespace

    namespace {
        //-------------------------------------------------------------
        //! @brief  垂直下向きレイと三角形の交差判定（Möller–Trumbore法）
        //! @return ヒットしたワールドY座標（ヒットしなければstd::nullopt）
        //-------------------------------------------------------------
        std::optional<float> RayTriangleIntersectVertical(const hlslpp::float3& rayOrigin,    // (worldX, rayStartY, worldZ)
                                                          float                 rayLength,    // 下向きの長さ（正の値）
                                                          const hlslpp::float3& v0,
                                                          const hlslpp::float3& v1,
                                                          const hlslpp::float3& v2) {
            constexpr float kEpsilon = 1e-6f;
            hlslpp::float3  rayDir(0.0f, -1.0f, 0.0f);

            hlslpp::float3 edge1 = v1 - v0;
            hlslpp::float3 edge2 = v2 - v0;
            hlslpp::float3 h     = hlslpp::cross(rayDir, edge2);
            float          a     = hlslpp::dot(edge1, h);

            if(std::fabs(a) < kEpsilon)
                return std::nullopt;    // レイと三角形がほぼ平行

            float          f = 1.0f / a;
            hlslpp::float3 s = rayOrigin - v0;
            float          u = f * hlslpp::dot(s, h);
            if(u < 0.0f || u > 1.0f)
                return std::nullopt;

            hlslpp::float3 q = hlslpp::cross(s, edge1);
            float          v = f * hlslpp::dot(rayDir, q);
            if(v < 0.0f || u + v > 1.0f)
                return std::nullopt;

            float t = f * hlslpp::dot(edge2, q);
            if(t < kEpsilon || t > rayLength)
                return std::nullopt;    // 交点がレイの範囲外

            return rayOrigin.y - t;    // 下向きレイなのでY座標はt分だけ下がる
        }

        //-------------------------------------------------------------
        //! @brief  指定した(worldX, worldZ)の真下にある三角形群から、最も高い交点のY座標を返す
        //-------------------------------------------------------------
        float SampleHeightAt(float                              worldX,
                             float                              worldZ,
                             float                              rayStartY,
                             float                              rayLength,
                             const std::vector<hlslpp::float3>& vertices,
                             const std::vector<uint32_t>&       indices,
                             float                              fallbackHeight) {
            hlslpp::float3 rayOrigin(worldX, rayStartY, worldZ);

            float bestHeight = fallbackHeight;
            bool  hitAny     = false;

            for(size_t i = 0; i + 2 < indices.size(); i += 3) {
                const hlslpp::float3& v0 = vertices[indices[i]];
                const hlslpp::float3& v1 = vertices[indices[i + 1]];
                const hlslpp::float3& v2 = vertices[indices[i + 2]];

                auto hit = RayTriangleIntersectVertical(rayOrigin, rayLength, v0, v1, v2);
                if(hit.has_value()) {
                    if(!hitAny || *hit > bestHeight) {    // 複数ヒットしたら一番高い面を採用（地形の上面）
                        bestHeight = *hit;
                        hitAny     = true;
                    }
                }
            }
            return bestHeight;
        }

        //-------------------------------------------------------------
        //! @brief  モデルの全ノード・全メッシュをワールド座標の頂点・インデックスに変換する
        //-------------------------------------------------------------
        void CollectWorldTriangles(const Tsukino::GraphicsCommon::ModelData& modelData,
                                   std::vector<hlslpp::float3>&              outVertices,
                                   std::vector<uint32_t>&                    outIndices) {
            if(modelData.nodes.empty())
                return;

            std::function<void(uint32_t, const Tsukino::Core::Math::matrix&)> Visit = [&](uint32_t nodeIdx, const Tsukino::Core::Math::matrix& parentWorld) {
                const auto& node = modelData.nodes[nodeIdx];

                // このノードのローカル行列（Scale × Rotate × Translate の順で合成）
                hlslpp::quaternion rot(node.rotation.x, node.rotation.y, node.rotation.z, node.rotation.w);

                Tsukino::Core::Math::matrix scaleMat = Tsukino::Core::Math::matrix::scale(hlslpp::float3(node.scale.x, node.scale.y, node.scale.z));
                Tsukino::Core::Math::matrix rotMat   = Tsukino::Core::Math::matrix::rotate(rot);
                Tsukino::Core::Math::matrix transMat =
                    Tsukino::Core::Math::matrix::translate(hlslpp::float3(node.translation.x, node.translation.y, node.translation.z));

                Tsukino::Core::Math::matrix localMat = hlslpp::mul(scaleMat, hlslpp::mul(rotMat, transMat));
                Tsukino::Core::Math::matrix worldMat = hlslpp::mul(localMat, parentWorld);

                for(uint32_t meshIdx : node.meshIndices) {
                    const auto& mesh = modelData.meshes[meshIdx];

                    uint32_t baseVertex = static_cast<uint32_t>(outVertices.size());

                    for(uint32_t v = 0; v < mesh.vertexCount; ++v) {
                        const uint8_t* raw = mesh.vertexData.data() + (size_t)v * mesh.vertexStride;
                        float          px, py, pz;
                        std::memcpy(&px, raw + 0, sizeof(float));
                        std::memcpy(&py, raw + 4, sizeof(float));
                        std::memcpy(&pz, raw + 8, sizeof(float));

                        hlslpp::float4 worldPos4 = hlslpp::mul(hlslpp::float4(px, py, pz, 1.0f), worldMat);
                        outVertices.emplace_back(worldPos4.x, worldPos4.y, worldPos4.z);
                    }

                    for(size_t i = 0; i < mesh.indices.size(); ++i) {
                        outIndices.push_back(baseVertex + mesh.indices[i]);
                    }
                }

                for(uint32_t child : node.childIndices) {
                    Visit(child, worldMat);
                }
            };

            Visit(modelData.rootNodeIndex, Tsukino::Core::Math::matrix::identity());
        }
    }    // namespace

    //-------------------------------------------------------------
    // システムの更新処理
    //-------------------------------------------------------------
    void HeightmapGenerationSystem::Update(Tsukino::ECS::Registry& registry, float deltaTime) {
        Tsukino::EngineIntegration::EngineContext* context = registry.GetContext<Tsukino::EngineIntegration::EngineContext*>();
        if(!context->assetManager)
            return;

        std::vector<entt::entity> entitiesToRemoveRequest;

        auto view = registry.View<TerrainGenerationRequestComponent, ModelComponent>();
        view.each([&](auto entity, auto& req, auto& modelComp) {
            Tsukino::Core::Ref<Tsukino::Asset::IAsset>     asset      = context->assetManager->Get(modelComp.modelHandle);
            Tsukino::Core::Ref<Tsukino::Asset::ModelAsset> modelAsset = std::static_pointer_cast<Tsukino::Asset::ModelAsset>(asset);

            if(!modelAsset || modelAsset->modelData.meshes.empty())
                return;

            // モデル全体のAABBを算出
            hlslpp::float3 minBound(FLT_MAX, FLT_MAX, FLT_MAX);
            hlslpp::float3 maxBound(-FLT_MAX, -FLT_MAX, -FLT_MAX);
            size_t         totalVertexCount = 0;

            for(const auto& mesh : modelAsset->modelData.meshes) {
                // Y軸反転
                hlslpp::float3 meshMin  = hlslpp::float3(mesh.bounds.min.x, -mesh.bounds.min.y, mesh.bounds.min.z);
                hlslpp::float3 meshMax  = hlslpp::float3(mesh.bounds.max.x, -mesh.bounds.max.y, mesh.bounds.max.z);
                minBound                = hlslpp::min(minBound, meshMin);
                maxBound                = hlslpp::max(maxBound, meshMax);
                totalVertexCount       += mesh.vertexCount;

                std::string matName = "(不明)";
                if(mesh.materialIndex < modelAsset->modelData.materials.size()) {
                    matName = modelAsset->modelData.materials[mesh.materialIndex].name;
                }

                Tsukino::Core::Log::Info("mesh materialIndex=" + std::to_string(mesh.materialIndex) + " materialName=" + matName
                                         + " min.y=" + std::to_string(mesh.bounds.min.y) + " max.y=" + std::to_string(mesh.bounds.max.y)
                                         + " min.z=" + std::to_string(mesh.bounds.min.z) + " max.z=" + std::to_string(mesh.bounds.max.z));
            }

            // Zはswapしてから反転する（Z軸の正方向が手前になるようにする）
            std::swap(minBound.z, maxBound.z);

            minBound.z *= -1;
            maxBound.z *= -1;

            // 実メッシュのワールド頂点・インデックスを収集
            std::vector<hlslpp::float3> worldVerts;
            std::vector<uint32_t>       worldIndices;
            CollectWorldTriangles(modelAsset->modelData, worldVerts, worldIndices);

            if(worldVerts.empty() || worldIndices.empty())
                return;

            uint32_t size           = 64;
            float    widthX         = maxBound.x - minBound.x;
            float    depthZ         = maxBound.z - minBound.z;
            float    sampleSpacingX = widthX / (float)(size - 1);
            float    sampleSpacingZ = depthZ / (float)(size - 1);

            float rayStartY = maxBound.y + 50.0f;
            float rayLength = (maxBound.y - minBound.y) + 100.0f;

            std::vector<float> samples(size * size);
            for(uint32_t z = 0; z < size; ++z) {
                for(uint32_t x = 0; x < size; ++x) {
                    float worldX = minBound.x + (float)x * sampleSpacingX;
                    float worldZ = minBound.z + (float)z * sampleSpacingZ;

                    // 絶対Yを取得
                    float absoluteY = SampleHeightAt(worldX, worldZ, rayStartY, rayLength, worldVerts, worldIndices, minBound.y);

                    // 最低点を 0 に正規化する（相対高さに変換）
                    samples[z * size + x] = absoluteY - minBound.y;
                }
            }

            auto& col              = registry.GetComponent<CollisionComponent>(entity);
            col.type               = ColliderType::Heightfield;
            col.heightfieldSize    = size;
            col.heightfieldSamples = std::move(samples);
            col.heightfieldOffset  = {minBound.x, minBound.y, minBound.z};
            col.heightfieldScale   = {sampleSpacingX, 1.0f, sampleSpacingZ};    // サンプル値が絶対Y座標なのでY方向は1.0

            entitiesToRemoveRequest.push_back(entity);
        });

        for(auto entity : entitiesToRemoveRequest) {    // ← 抜けていた削除処理
            registry.RemoveComponent<TerrainGenerationRequestComponent>(entity);
        }
    }
}    // namespace Tsukino::BuiltIn::ECS
