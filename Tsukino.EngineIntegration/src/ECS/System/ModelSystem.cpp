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
#include <Tsukino/BuiltIn/ECS/Component/RigidbodyComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/HighlightComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/MotionVectorComponent.hpp>
#include <Tsukino/Engine/Asset/AssetManager.hpp>
#include <Tsukino/Engine/Asset/Model/ModelAsset.hpp>
#include <Tsukino/Engine/Asset/Shader/ShaderAsset.hpp>
#include <Tsukino/Engine/Asset/Material/MaterialAsset.hpp>
#include <Tsukino/Engine/Asset/Texture/TextureAsset.hpp>
#include <Tsukino/Renderer/Renderer.hpp>
#include <Tsukino/Renderer/DX11/MeshBuffer.hpp>
#include <Tsukino/Renderer/ShaderSlots.hpp>
#include <Tsukino/GraphicsCommon/Model/ModelData.hpp>
#include <Tsukino/GraphicsCommon/Vertex/VertexFormat.hpp>

#include <Tsukino/Core/Math/Matrix.hpp>
#include <Tsukino/Core/Log.hpp>

#include <entt/entt.hpp>

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

        //-------------------------------------------------------------
        // 水面の時間更新
        //-------------------------------------------------------------
        ctx->renderer->UpdateWaterTime(deltaTime);

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
            auto meshCacheIt = m_modelMeshCache.find(handleVal);
            if(meshCacheIt == m_modelMeshCache.end()) {
                std::vector<Tsukino::Renderer::MeshBuffer> buffers;
                for(const auto& mesh : modelAsset->modelData.meshes) {
                    buffers.push_back(Tsukino::Renderer::CreateMeshBuffer(ctx->renderer->GetDevice(), mesh));
                }
                auto result = m_modelMeshCache.emplace(handleVal, std::move(buffers));
                meshCacheIt = result.first;
            }
            const auto& meshBuffers = meshCacheIt->second;

            auto* skeletonOut = registry.try_get<SkeletonOutputComponent>(entity);
            bool  isSkeletal  = skeletonOut && skeletonOut->bone_count > 0;

            //--------------------------------------------------------------
            // モーションブラー用の前フレームデータ
            //
            // MotionVectorSnapshotSystem がフレーム先頭で退避した値。
            // スキンメッシュの場合は前フレームのボーン本数が今フレームと
            // 一致していることまで確認する。食い違ったまま渡すと、VS側で
            // 前フレームのスキニング行列がゼロ行列になり、透視除算で
            // w=0 → NaN になって画面が壊れる。
            //--------------------------------------------------------------
            auto* motionVec = registry.try_get<MotionVectorComponent>(entity);
            bool  hasPrev   = motionVec && motionVec->valid;
            if(hasPrev && isSkeletal && motionVec->prevBoneCount != skeletonOut->bone_count)
                hasPrev = false;

            const Tsukino::Core::Math::matrix prevWorldMatrix = hasPrev ? motionVec->prevWorld : transform.worldMatrix;

            // ノードとメッシュの巡回ループ
            for(const auto& node : modelAsset->modelData.nodes) {
                for(u32 meshIdx : node.meshIndices) {
                    if(meshIdx >= meshBuffers.size())
                        continue;

                    const auto& meshData         = modelAsset->modelData.meshes[meshIdx];
                    const auto& targetMeshBuffer = meshBuffers[meshIdx];

                    // 行列の計算
                    // 前フレーム分も同じ組み立てで作る（ノード変換はモデル固有の
                    // 静的値なので、ワールド行列だけ差し替えればよい）
                    Tsukino::Core::Math::matrix finalTransform;
                    Tsukino::Core::Math::matrix prevFinalTransform;
                    if(isSkeletal) {
                        finalTransform     = transform.worldMatrix;
                        prevFinalTransform = prevWorldMatrix;
                    } else {
                        Tsukino::Core::Math::matrix scaleMat = Tsukino::Core::Math::matrix::scale(hlslpp::float3(node.scale.x, node.scale.y, node.scale.z));
                        Tsukino::Core::Math::matrix rotMat =
                            Tsukino::Core::Math::matrix::rotate(hlslpp::quaternion(node.rotation.x, node.rotation.y, node.rotation.z, node.rotation.w));
                        Tsukino::Core::Math::matrix transMat =
                            Tsukino::Core::Math::matrix::translate(hlslpp::float3(node.translation.x, node.translation.y, node.translation.z));
                        Tsukino::Core::Math::matrix nodeTransform = hlslpp::mul(hlslpp::mul(scaleMat, rotMat), transMat);
                        finalTransform                            = hlslpp::mul(nodeTransform, transform.worldMatrix);
                        prevFinalTransform                        = hlslpp::mul(nodeTransform, prevWorldMatrix);

                       /* Tsukino::Core::Log::Info("node.translation = (" + std::to_string(node.translation.x) + ", " + std::to_string(node.translation.y) + ", "
                                                 + std::to_string(node.translation.z) + ")");*/
                    }

                    // コリジョンオフセットの逆変換
                    // ※この補正はRigidbodyType::Dynamicの場合のみ有効。DynamicはPhysicsSystemの「Dynamic同期」で
                    //   TransformComponent.positionが毎フレーム物理ボディの中心位置へ上書きされるため、
                    //   モデル（原点=足元）を正しい位置に描画するにはoffsetPositionを引き戻す必要がある。
                    //   Kinematic/Static等はtf.positionが書き換えられず元の位置（=モデル原点と一致）のままなので、
                    //   ここで補正をかけるとモデルだけ余計にズレてコリジョンと食い違ってしまう。
                    auto* col = registry.try_get<CollisionComponent>(entity);
                    auto* rb  = registry.try_get<RigidbodyComponent>(entity);
                    if(col && col->isInitialized && rb && rb->type == RigidbodyType::Dynamic) {

                        hlslpp::quaternion q = hlslpp::quaternion(col->offsetRotation.x, col->offsetRotation.y, col->offsetRotation.z, col->offsetRotation.w);
                        hlslpp::quaternion conj = hlslpp::quaternion(-q.x, -q.y, -q.z, q.w);

                        Tsukino::Core::Math::matrix invRotMat = Tsukino::Core::Math::matrix::rotate(conj);
                        Tsukino::Core::Math::matrix invTransMat =
                            Tsukino::Core::Math::matrix::translate(-hlslpp::float3(col->offsetPosition.x, col->offsetPosition.y, col->offsetPosition.z));

                        Tsukino::Core::Math::matrix invOffsetMat = hlslpp::mul(invRotMat, invTransMat);
                        finalTransform                           = hlslpp::mul(invOffsetMat, finalTransform);
                        prevFinalTransform                       = hlslpp::mul(invOffsetMat, prevFinalTransform);
                    }

                    // マテリアル定数バッファの構築
                    Tsukino::Renderer::CBufferMaterial cbMat{};
                    cbMat.baseColor = hlslpp::float4(1.0f, 1.0f, 1.0f, 1.0f);
                    cbMat.emissive  = hlslpp::float3(0.0f, 0.0f, 0.0f);
                    cbMat.metallic  = 0.0f;
                    cbMat.roughness = 0.5f;
                    cbMat.specular  = 0.5f;

                    //--------------------------------------------------------------
                    // マテリアルテクスチャ（t0〜t4）の解決
                    // 未設定のスロットはここでは nullptr のままにしておき、
                    // Material構築時にデフォルトテクスチャへフォールバックさせる。
                    // null SRV をそのままバインドするとサンプル結果が0になり、
                    // 特に法線マップは DecodeNormal が normalize(-1,-1,-1) になって破綻する。
                    //--------------------------------------------------------------
                    ID3D11ShaderResourceView* albedoSRV   = nullptr;
                    ID3D11ShaderResourceView* normalSRV   = nullptr;
                    ID3D11ShaderResourceView* mrSRV       = nullptr;
                    ID3D11ShaderResourceView* emissiveSRV = nullptr;
                    ID3D11ShaderResourceView* aoSRV       = nullptr;

                    Tsukino::GraphicsCommon::ShadingModel shadingModel = Tsukino::GraphicsCommon::ShadingModel::PBR;

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

                            shadingModel = matAsset->data.shadingModel;

                            // AssetHandle から SRV を引く（無効ハンドル・未ロードは nullptr）
                            auto resolveSRV = [&](const Tsukino::Asset::AssetHandle& handle) -> ID3D11ShaderResourceView* {
                                if(!handle.IsValid())
                                    return nullptr;

                                auto texAssetBase = ctx->assetManager->Get(handle);
                                if(!texAssetBase)
                                    return nullptr;

                                auto texAsset = std::static_pointer_cast<Tsukino::Asset::TextureAsset>(texAssetBase);
                                return ctx->renderer->GetTextureSRV(*texAsset);
                            };

                            albedoSRV   = resolveSRV(matAsset->albedoHandle);
                            normalSRV   = resolveSRV(matAsset->normalHandle);
                            mrSRV       = resolveSRV(matAsset->metallicRoughnessHandle);
                            emissiveSRV = resolveSRV(matAsset->emissiveHandle);
                            aoSRV       = resolveSRV(matAsset->aoHandle);
                        }
                    }

                    // エンティティ単位のハイライト上書き（マテリアルアセットより後に適用する）
                    if(auto* highlight = registry.try_get<HighlightComponent>(entity); highlight && highlight->active) {
                        cbMat.rimColor  = hlslpp::float4(highlight->rimColor, highlight->rimIntensity);
                        cbMat.rimParams = hlslpp::float4(highlight->rimPower, highlight->glow, 0.0f, 0.0f);
                    }

                    m_cbufferMaterialBuffer.push_back(cbMat);
                    Tsukino::Renderer::CBufferMaterial* pCbMat = &m_cbufferMaterialBuffer.back();

                    // シェーダーアセットの取得
                    // VSはワールド座標・法線・UVを出力するだけなので、フォワード(Water)/
                    // ディファード(GBuffer)いずれのPSでも共用できる
                    Tsukino::Asset::AssetHandle vsHandle = isSkeletal ? ctx->builtinAssets->shaders.modelVS : ctx->builtinAssets->shaders.staticModelVS;
                    Tsukino::Asset::AssetHandle psHandle = ctx->builtinAssets->shaders.gbufferPS;

                    // ShadingModel に応じて PS と BlendMode を切り替え
                    // （shadingModel はテクスチャ解決と同じ MaterialAsset 取得で拾っている）
                    Tsukino::Renderer::BlendMode blendMode = Tsukino::Renderer::BlendMode::Opaque;
                    if(shadingModel == Tsukino::GraphicsCommon::ShadingModel::Water) {
                        psHandle  = ctx->builtinAssets->shaders.waterPS;
                        blendMode = Tsukino::Renderer::BlendMode::Alpha;
                    }

                    auto vsAsset = std::static_pointer_cast<Tsukino::Asset::ShaderAsset>(ctx->assetManager->Get(vsHandle));
                    auto psAsset = std::static_pointer_cast<Tsukino::Asset::ShaderAsset>(ctx->assetManager->Get(psHandle));

                    if(!vsAsset || !psAsset)
                        continue;

                    // 頂点フォーマット
                    Tsukino::GraphicsCommon::VertexFormat vertexFormat =
                        isSkeletal ? Tsukino::GraphicsCommon::VertexFormat::Skinned : Tsukino::GraphicsCommon::VertexFormat::PositionNormalUV;

                    // パイプライン生成（BlendMode を渡す）
                    auto pipeline =
                        ctx->renderer->GetPipelineFactory()->Create(*vsAsset, *psAsset, vertexFormat, Tsukino::Renderer::DepthMode::ReadWrite, blendMode);

                    if(!pipeline)
                        continue;

                    // マテリアル構築
                    Tsukino::Renderer::Material mat{};
                    mat.SetPipeline(pipeline.get());
                    mat.SetSampler(ctx->renderer->GetSampler(Tsukino::GraphicsCommon::SamplerType::AnisotropicWrap));

                    //--------------------------------------------------------------
                    // t0〜t4 をバインド。未設定はデフォルトへフォールバックする。
                    //   白        : シェーダー側で cbuffer 定数との乗算になるため恒等元
                    //   フラット法線: 適用しても頂点法線がそのまま保たれる
                    //--------------------------------------------------------------
                    ID3D11ShaderResourceView* whiteSRV      = ctx->renderer->GetWhiteTextureSRV();
                    ID3D11ShaderResourceView* flatNormalSRV = ctx->renderer->GetFlatNormalTextureSRV();

                    mat.SetTexture(Tsukino::Renderer::SRVSlot::Albedo, albedoSRV ? albedoSRV : whiteSRV);
                    mat.SetTexture(Tsukino::Renderer::SRVSlot::Normal, normalSRV ? normalSRV : flatNormalSRV);
                    mat.SetTexture(Tsukino::Renderer::SRVSlot::MetallicRoughness, mrSRV ? mrSRV : whiteSRV);
                    mat.SetTexture(Tsukino::Renderer::SRVSlot::Emissive, emissiveSRV ? emissiveSRV : whiteSRV);
                    mat.SetTexture(Tsukino::Renderer::SRVSlot::AO, aoSRV ? aoSRV : whiteSRV);

                    m_materialBuffer.push_back(mat);

                    // 描画コマンド
                    Tsukino::Renderer::DrawCommand cmd{};
                    cmd.mesh         = const_cast<Tsukino::Renderer::MeshBuffer*>(&targetMeshBuffer);
                    cmd.transform    = finalTransform;
                    cmd.material     = &m_materialBuffer.back();
                    cmd.materialData = pCbMat;
                    if(isSkeletal) {
                        cmd.boneMatrices = skeletonOut->local_matrices;
                        cmd.boneCount    = skeletonOut->bone_count;
                    }

                    //--------------------------------------------------------------
                    // モーションブラー用の前フレームデータ
                    // hasPrevFrame が false なら Renderer 側は一切読まない
                    //--------------------------------------------------------------
                    cmd.prevTransform = prevFinalTransform;
                    cmd.hasPrevFrame  = hasPrev;
                    if(hasPrev && isSkeletal)
                        cmd.prevBoneMatrices = motionVec->prevBones;
                    // Water はフォワードの専用パス（半透明のためディファード対象外）。
                    // それ以外（PBR/Unlit/Toon）は不透明としてG-Bufferパスへ回す。
                    cmd.pass = (shadingModel == Tsukino::GraphicsCommon::ShadingModel::Water) ? Tsukino::Renderer::RenderPass::Water
                                                                                              : Tsukino::Renderer::RenderPass::GBuffer;

                    ctx->renderer->PushDrawCommand(cmd);
                }
            }
        });
    }

}    // namespace Tsukino::BuiltIn::ECS
