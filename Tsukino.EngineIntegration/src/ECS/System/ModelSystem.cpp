//-------------------------------------------------------------
//! @file   ModelSystem.cpp
//! @brief  モデル描画システムの実装
//! @author 山﨑愛
//-------------------------------------------------------------
#define NOMINMAX

#include <Tsukino/EngineIntegration/ECS/System/ModelSystem.hpp>
#include <Tsukino/EngineIntegration/EngineContext.hpp>
#include <Tsukino/BuiltIn/BuiltInAssets.hpp>
#include <Tsukino/BuiltIn/ECS/Component/ModelComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/SkeletonOutputComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/CollisionComponent.hpp>
#include <Tsukino/Engine/Asset/AssetManager.hpp>
#include <Tsukino/Engine/Asset/Model/ModelAsset.hpp>
#include <Tsukino/Engine/Asset/Shader/ShaderAsset.hpp>
#include <Tsukino/Engine/Asset/Material/MaterialAsset.hpp>
#include <Tsukino/Engine/Asset/Texture/TextureAsset.hpp>
#include <Tsukino/Renderer/Renderer.hpp>
#include <Tsukino/Renderer/DX11/MeshBuffer.hpp>
#include <Tsukino/GraphicsCommon/Model/ModelData.hpp>
#include <Tsukino/GraphicsCommon/Vertex/VertexFormat.hpp>

#include <Tsukino/Core/Math/Matrix.hpp>

#include <entt/entt.hpp>
#include <unordered_map>

// メッシュキャッシュ
static std::unordered_map<uint64_t, std::vector<Tsukino::Renderer::MeshBuffer>> s_modelMeshCache;

namespace Tsukino::BuiltIn::ECS {

    //-------------------------------------------------------------
    //! @brief システムの更新
    //-------------------------------------------------------------
    void ModelSystem::Update(Tsukino::ECS::Registry& registry, float deltaTime) {
        Tsukino::EngineIntegration::EngineContext* ctx = registry.GetContext<Tsukino::EngineIntegration::EngineContext*>();
        if(!ctx || !ctx->renderer)
            return;

        m_materialBuffer.clear();
        m_cbufferMaterialBuffer.clear();

        auto view = registry.View<TransformComponent, ModelComponent>();

        view.each([&](entt::entity entity, const TransformComponent& transform, const ModelComponent& modelComp) {
            if(!modelComp.visible)
                return;

            auto asset = ctx->assetManager->Get(modelComp.modelHandle);
            if(!asset || asset->GetType() != Tsukino::Asset::AssetType::Model)
                return;

            auto modelAsset = std::static_pointer_cast<Tsukino::Asset::ModelAsset>(asset);
            auto handleVal  = modelComp.modelHandle.Value();

            //--------------------------------------------------------------
            // メッシュバッファのキャッシュ取得
            //--------------------------------------------------------------
            auto meshCacheIt = s_modelMeshCache.find(handleVal);
            if(meshCacheIt == s_modelMeshCache.end()) {
                std::vector<Tsukino::Renderer::MeshBuffer> buffers;
                for(const auto& mesh : modelAsset->modelData.meshes) {
                    buffers.push_back(Tsukino::Renderer::CreateMeshBuffer(ctx->renderer->GetDevice(), mesh));
                }
                auto result = s_modelMeshCache.emplace(handleVal, std::move(buffers));
                meshCacheIt = result.first;
            }
            const auto& meshBuffers = meshCacheIt->second;

            auto* skeletonOut = registry.try_get<SkeletonOutputComponent>(entity);
            bool  isSkeletal  = skeletonOut && skeletonOut->bone_count > 0;

            // ノードとメッシュの巡回ループ
            for(const auto& node : modelAsset->modelData.nodes) {
                for(u32 meshIdx : node.meshIndices) {
                    if(meshIdx >= meshBuffers.size())
                        continue;

                    const auto& meshData         = modelAsset->modelData.meshes[meshIdx];
                    const auto& targetMeshBuffer = meshBuffers[meshIdx];

                    // 行列の計算
                    Tsukino::Core::Math::matrix finalTransform;
                    if(isSkeletal) {
                        finalTransform = transform.worldMatrix;
                    } else {
                        Tsukino::Core::Math::matrix scaleMat = Tsukino::Core::Math::matrix::scale(hlslpp::float3(node.scale.x, node.scale.y, node.scale.z));
                        Tsukino::Core::Math::matrix rotMat =
                            Tsukino::Core::Math::matrix::rotate(hlslpp::quaternion(node.rotation.x, node.rotation.y, node.rotation.z, node.rotation.w));
                        Tsukino::Core::Math::matrix transMat =
                            Tsukino::Core::Math::matrix::translate(hlslpp::float3(node.translation.x, node.translation.y, node.translation.z));
                        Tsukino::Core::Math::matrix nodeTransform = hlslpp::mul(hlslpp::mul(scaleMat, rotMat), transMat);
                        finalTransform                            = hlslpp::mul(nodeTransform, transform.worldMatrix);
                    }

                    // コリジョンオフセットの逆変換
                    auto* col = registry.try_get<CollisionComponent>(entity);
                    if(col && col->isInitialized) {
                        hlslpp::quaternion q = hlslpp::quaternion(col->offsetRotation.x, col->offsetRotation.y, col->offsetRotation.z, col->offsetRotation.w);
                        hlslpp::quaternion conj = hlslpp::quaternion(-q.x, -q.y, -q.z, q.w);

                        Tsukino::Core::Math::matrix invRotMat = Tsukino::Core::Math::matrix::rotate(conj);
                        Tsukino::Core::Math::matrix invTransMat =
                            Tsukino::Core::Math::matrix::translate(-hlslpp::float3(col->offsetPosition.x, col->offsetPosition.y, col->offsetPosition.z));

                        Tsukino::Core::Math::matrix invOffsetMat = hlslpp::mul(invRotMat, invTransMat);
                        finalTransform                           = hlslpp::mul(invOffsetMat, finalTransform);
                    }

                    // マテリアル定数バッファの構築
                    Tsukino::Renderer::CBufferMaterial cbMat{};
                    cbMat.baseColor = hlslpp::float4(1.0f, 1.0f, 1.0f, 1.0f);
                    cbMat.emissive  = hlslpp::float3(0.0f, 0.0f, 0.0f);
                    cbMat.metallic  = 0.0f;
                    cbMat.roughness = 0.5f;
                    cbMat.specular  = 0.5f;

                    ID3D11ShaderResourceView* srv = nullptr;

                    if(meshData.materialIndex < modelAsset->materialHandles.size()) {
                        auto matHandle    = modelAsset->materialHandles[meshData.materialIndex];
                        auto matAssetBase = ctx->assetManager->Get(matHandle);
                        if(matAssetBase && matAssetBase->GetType() == Tsukino::Asset::AssetType::Material) {
                            Tsukino::Core::Ref<Tsukino::Asset::MaterialAsset> matAsset = std::static_pointer_cast<Tsukino::Asset::MaterialAsset>(matAssetBase);

                            cbMat.baseColor =
                                hlslpp::float4(matAsset->data.baseColor.x, matAsset->data.baseColor.y, matAsset->data.baseColor.z, matAsset->data.baseColor.w);
                            cbMat.emissive  = hlslpp::float3(matAsset->data.emissive.x, matAsset->data.emissive.y, matAsset->data.emissive.z);
                            cbMat.metallic  = matAsset->data.metallic;
                            cbMat.roughness = matAsset->data.roughness;
                            cbMat.specular  = matAsset->data.specular;

                            if(matAsset->albedoHandle.IsValid()) {
                                auto texAssetBase = ctx->assetManager->Get(matAsset->albedoHandle);
                                if(texAssetBase) {
                                    auto texAsset = std::static_pointer_cast<Tsukino::Asset::TextureAsset>(texAssetBase);
                                    srv           = ctx->renderer->GetTextureSRV(*texAsset);
                                }
                            }
                        }
                    }

                    m_cbufferMaterialBuffer.push_back(cbMat);
                    Tsukino::Renderer::CBufferMaterial* pCbMat = &m_cbufferMaterialBuffer.back();

                    // シェーダーアセットの取得
                    Tsukino::Asset::AssetHandle vsHandle = isSkeletal ? ctx->builtinAssets->shaders.modelVS : ctx->builtinAssets->shaders.staticModelVS;
                    Tsukino::Asset::AssetHandle psHandle = ctx->builtinAssets->shaders.modelPS;

                    // ShadingModelに応じてPSを切り替え
                    if(meshData.materialIndex < modelAsset->materialHandles.size()) {
                        auto matHandle    = modelAsset->materialHandles[meshData.materialIndex];
                        auto matAssetBase = ctx->assetManager->Get(matHandle);
                        if(matAssetBase && matAssetBase->GetType() == Tsukino::Asset::AssetType::Material) {
                            auto matAsset = std::static_pointer_cast<Tsukino::Asset::MaterialAsset>(matAssetBase);
                            if(matAsset->data.shadingModel == Tsukino::GraphicsCommon::ShadingModel::Water) {
                                psHandle = ctx->builtinAssets->shaders.waterPS;
                            }
                        }
                    }

                    auto vsAsset = std::static_pointer_cast<Tsukino::Asset::ShaderAsset>(ctx->assetManager->Get(vsHandle));
                    auto psAsset = std::static_pointer_cast<Tsukino::Asset::ShaderAsset>(ctx->assetManager->Get(psHandle));

                    if(!vsAsset || !psAsset)
                        continue;

                    //--------------------------------------------------------------
                    // 頂点フォーマット列挙型を判定してファクトリーに投げる
                    //--------------------------------------------------------------
                    Tsukino::GraphicsCommon::VertexFormat vertexFormat =
                        isSkeletal ? Tsukino::GraphicsCommon::VertexFormat::Skinned : Tsukino::GraphicsCommon::VertexFormat::PositionNormalUV;

                    // パイプラインキャッシュの取得・生成
                    auto pipeline = ctx->renderer->GetPipelineFactory()->Create(*vsAsset, *psAsset, vertexFormat, Tsukino::Renderer::DepthMode::ReadWrite);

                    if(!pipeline)
                        continue;

                    // マテリアルオブジェクトの構築
                    Tsukino::Renderer::Material mat{};
                    mat.SetPipeline(pipeline.get());
                    mat.SetSampler(ctx->renderer->GetSampler(Tsukino::GraphicsCommon::SamplerType::AnisotropicWrap));

                    if(srv) {
                        mat.SetTexture(srv);
                    } else {
                        mat.SetTexture(ctx->renderer->GetWhiteTextureSRV());
                    }

                    m_materialBuffer.push_back(mat);

                    // 描画コマンドの組み立てと発行
                    Tsukino::Renderer::DrawCommand cmd{};
                    cmd.mesh         = const_cast<Tsukino::Renderer::MeshBuffer*>(&targetMeshBuffer);
                    cmd.transform    = finalTransform;
                    cmd.material     = &m_materialBuffer.back();
                    cmd.materialData = pCbMat;
                    if(isSkeletal) {
                        cmd.boneMatrices = skeletonOut->local_matrices;
                        cmd.boneCount    = skeletonOut->bone_count;
                    }

                    ctx->renderer->PushDrawCommand(cmd);
                }
            }
        });
    }

}    // namespace Tsukino::BuiltIn::ECS
