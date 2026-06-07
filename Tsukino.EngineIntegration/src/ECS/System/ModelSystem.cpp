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

        // 初回のみパイプライン生成
        if(!m_pipelineCache) {
            std::shared_ptr<Tsukino::Asset::ShaderAsset> vsStaticAsset =
                std::static_pointer_cast<Tsukino::Asset::ShaderAsset>(ctx->assetManager->Get(ctx->builtinAssets->shaders.staticModelVS));
            std::shared_ptr<Tsukino::Asset::ShaderAsset> vsAsset =
                std::static_pointer_cast<Tsukino::Asset::ShaderAsset>(ctx->assetManager->Get(ctx->builtinAssets->shaders.modelVS));
            std::shared_ptr<Tsukino::Asset::ShaderAsset> psAsset =
                std::static_pointer_cast<Tsukino::Asset::ShaderAsset>(ctx->assetManager->Get(ctx->builtinAssets->shaders.modelPS));

            if(vsStaticAsset && vsAsset && psAsset) {
                D3D11_INPUT_ELEMENT_DESC staticLayout[] = {
                    {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
                    {"NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
                    {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
                };
                m_pipelineCache = ctx->renderer->GetPipelineFactory()->Create(
                    *vsStaticAsset, *psAsset, staticLayout, ARRAYSIZE(staticLayout), Tsukino::Renderer::DepthMode::ReadWrite);

                D3D11_INPUT_ELEMENT_DESC skeletalLayout[] = {
                    {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
                    {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
                    {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
                    {"BONE_INDICES",
                     0, DXGI_FORMAT_R32G32B32A32_UINT,
                     1, offsetof(Tsukino::GraphicsCommon::BoneWeight, boneIndices),
                     D3D11_INPUT_PER_VERTEX_DATA, 0},
                    {"BONE_WEIGHTS",
                     0, DXGI_FORMAT_R32G32B32A32_FLOAT,
                     1, offsetof(Tsukino::GraphicsCommon::BoneWeight, weights),
                     D3D11_INPUT_PER_VERTEX_DATA, 0},
                };
                m_skeletalPipelineCache = ctx->renderer->GetPipelineFactory()->Create(
                    *vsAsset, *psAsset, skeletalLayout, ARRAYSIZE(skeletalLayout), Tsukino::Renderer::DepthMode::ReadWrite);
            }
        }

        if(!m_pipelineCache || !m_skeletalPipelineCache)
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

            // メッシュバッファがキャッシュされていなければ作成する
            if(s_modelMeshCache.find(handleVal) == s_modelMeshCache.end()) {
                std::vector<Tsukino::Renderer::MeshBuffer> buffers;
                for(const auto& mesh : modelAsset->modelData.meshes) {
                    buffers.push_back(Tsukino::Renderer::CreateMeshBuffer(ctx->renderer->GetDevice(), mesh));
                }
                s_modelMeshCache[handleVal] = std::move(buffers);
            }

            const auto& meshBuffers = s_modelMeshCache[handleVal];

            auto* skeletonOut = registry.try_get<SkeletonOutputComponent>(entity);
            bool  isSkeletal  = skeletonOut && skeletonOut->bone_count > 0;

            for(const auto& node : modelAsset->modelData.nodes) {
                for(u32 meshIdx : node.meshIndices) {
                    if(meshIdx >= meshBuffers.size())
                        continue;

                    const auto& meshData         = modelAsset->modelData.meshes[meshIdx];
                    const auto& targetMeshBuffer = meshBuffers[meshIdx];

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

                    // コリジョンオフセットの逆変換を適用
                    auto* col = registry.try_get<CollisionComponent>(entity);
                    if(col && col->isInitialized) {
                        // クォータニオンの共役 = (x, y, z) を反転、w はそのまま
                        hlslpp::quaternion q = hlslpp::quaternion(col->offsetRotation.x, col->offsetRotation.y, col->offsetRotation.z, col->offsetRotation.w);
                        hlslpp::quaternion conj = hlslpp::quaternion(-q.x, -q.y, -q.z, q.w);

                        Tsukino::Core::Math::matrix invRotMat = Tsukino::Core::Math::matrix::rotate(conj);
                        Tsukino::Core::Math::matrix invTransMat =
                            Tsukino::Core::Math::matrix::translate(-hlslpp::float3(col->offsetPosition.x, col->offsetPosition.y, col->offsetPosition.z));

                        Tsukino::Core::Math::matrix invOffsetMat = hlslpp::mul(invRotMat, invTransMat);
                        finalTransform                           = hlslpp::mul(invOffsetMat, finalTransform);
                    }

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

                    Tsukino::Renderer::Material mat{};
                    mat.SetPipeline(isSkeletal ? m_skeletalPipelineCache.get() : m_pipelineCache.get());
                    mat.SetSampler(ctx->renderer->GetSampler(Tsukino::GraphicsCommon::SamplerType::LinearClamp));
                    if(srv)
                        mat.SetTexture(srv);

                    m_materialBuffer.push_back(mat);

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
