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
#include <Tsukino/BuiltIn/ECS/Component/RimGlowComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/MaterialPropertyBlockComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/ScreenModelComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Transform/TransformUtility.hpp>
#include <Tsukino/BuiltIn/ECS/UI/UICanvas.hpp>
#include <Tsukino/BuiltIn/ECS/UI/UIClipUtility.hpp>
#include <Tsukino/BuiltIn/ECS/UI/UIVisibilityUtility.hpp>
#include <Tsukino/BuiltIn/ECS/Component/MotionVectorComponent.hpp>
#include <Tsukino/Engine/Asset/AssetManager.hpp>
#include <Tsukino/Engine/Asset/Model/ModelAsset.hpp>
#include <Tsukino/Engine/Asset/Shader/ShaderAsset.hpp>
#include <Tsukino/Engine/Asset/Material/MaterialAsset.hpp>
#include <Tsukino/Engine/Asset/Texture/TextureAsset.hpp>
#include <Tsukino/Core/Window.hpp>
#include <Tsukino/Renderer/ConstantBuffer.hpp>
#include <Tsukino/Renderer/Renderer.hpp>
#include <Tsukino/Renderer/DX11/MeshBuffer.hpp>
#include <Tsukino/Renderer/ShaderSlots.hpp>
#include <Tsukino/GraphicsCommon/Model/ModelData.hpp>
#include <Tsukino/GraphicsCommon/Vertex/VertexFormat.hpp>

#include <Tsukino/Core/Math/Matrix.hpp>
#include <Tsukino/Core/Log.hpp>

#include <entt/entt.hpp>

#include <algorithm>

namespace Tsukino::BuiltIn::ECS {
    namespace {
        //-------------------------------------------------------------
        //! UI の層に描くモデルのカメラで、正射影の奥行きの半分（unit）。
        //! この範囲に収まる大きさのモデルなら、前後が切れずに描ける
        //-------------------------------------------------------------
        constexpr float kScreenModelDepthHalfRange = 1000.0f;

        //-------------------------------------------------------------
        //! UI の層に描くモデルのカメラ（正射影）を作ります。
        //! 持ち主のワールド位置を画面の screenPixel に、1unit を pixelsPerUnit ピクセルで映し、
        //! 手前（+z の側）から -z の向きに見ます。深度はリバースZ（手前ほど大きい）
        //! @param  [in] center        持ち主のワールド位置
        //! @param  [in] screenPixel   持ち主を映す画面の位置（ピクセル。左上が原点）
        //! @param  [in] pixelsPerUnit 1unit を何ピクセルで描くか
        //! @param  [in] screenWidth   画面の幅（ピクセル）
        //! @param  [in] screenHeight  画面の高さ（ピクセル）
        //! @return カメラ行列を詰めたシーン定数（view / projection / viewProj / invViewProj / cameraPos）
        //-------------------------------------------------------------
        Tsukino::Renderer::CBufferScene MakeScreenModelCamera(const hlslpp::float3& center, const hlslpp::float2& screenPixel, float pixelsPerUnit,
                                                              float screenWidth, float screenHeight) {
            pixelsPerUnit            = std::max(pixelsPerUnit, 1.0e-4f);
            const hlslpp::float3 eye = center + hlslpp::float3(0.0f, 0.0f, kScreenModelDepthHalfRange);

            // ビュー空間では持ち主が (0, 0, 奥行きの半分) に来る。画面の左上 (0, 0) から右下までを unit に直して映す
            const float sx     = float(screenPixel.x);
            const float sy     = float(screenPixel.y);
            const float left   = -sx / pixelsPerUnit;
            const float right  = (screenWidth - sx) / pixelsPerUnit;
            const float top    = sy / pixelsPerUnit;
            const float bottom = (sy - screenHeight) / pixelsPerUnit;

            Tsukino::Renderer::CBufferScene camera{};
            camera.view = Tsukino::Core::Math::matrix::lookAtLH(eye, center, hlslpp::float3(0.0f, 1.0f, 0.0f));
            // near と far を入れ替えてリバースZにする（ワールドのカメラと同じ深度の向き）
            camera.projection  = Tsukino::Core::Math::matrix::orthographicOffCenterLH(left, right, bottom, top, kScreenModelDepthHalfRange * 2.0f, 0.0f);
            camera.viewProj    = hlslpp::mul(camera.view, camera.projection);
            camera.invViewProj = hlslpp::inverse(camera.viewProj);
            camera.cameraPos   = hlslpp::float4(eye, 1.0f);
            return camera;
        }
    }    // namespace

    //-------------------------------------------------------------
    //! @brief システムの更新
    //-------------------------------------------------------------
    void ModelSystem::Update(Tsukino::ECS::Registry& registry, float deltaTime) {
        Tsukino::EngineIntegration::EngineContext* ctx = registry.GetContext<Tsukino::EngineIntegration::EngineContext*>();
        if(!ctx || !ctx->renderer)
            return;

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

            //-------------------------------------------------------------
            // UI の層に描くモデル（自身か親に ScreenModelComponent がある）は、
            // そのカメラを作って Overlay パスへ積む（ワールドには描かない）。
            // anchor（UI の部品）があれば、その画面上の位置を基準にし、その祖先の UIClipComponent の枠で切り取る
            //-------------------------------------------------------------
            const Tsukino::ECS::Entity             screenOwner  = TransformUtility::FindNearestWith<ScreenModelComponent>(registry, entity);
            const ScreenModelComponent*            screenModel  = (screenOwner != entt::null) ? &registry.GetComponent<ScreenModelComponent>(screenOwner) : nullptr;
            const Tsukino::Renderer::CBufferScene* screenCamera = nullptr;
            bool                                   screenClipped = false;
            Tsukino::Renderer::ClipRect            screenClip;
            if(screenModel) {
                if(!ctx->window)
                    return;

                // 自身か祖先の UIVisibilityComponent で隠されている（UI の層のモデルだけ。ワールドのモデルは対象外）
                if(UIVisibilityUtility::IsHidden(registry, entity))
                    return;

                hlslpp::float2       screenPixel = screenModel->screenPosition;
                const Tsukino::ECS::Entity anchor = screenModel->anchor;
                if(anchor != entt::null) {
                    if(!registry.IsValid(anchor) || !registry.HasComponent<TransformComponent>(anchor))
                        return;
                    // 基準の UI の部品が隠されていれば、モデルも隠す（スクロールする一覧の行など）
                    if(UIVisibilityUtility::IsHidden(registry, anchor))
                        return;
                    const hlslpp::float3 anchorPosition = TransformUtility::GetWorldPosition(registry.GetComponent<TransformComponent>(anchor));
                    screenPixel += hlslpp::float2(anchorPosition.xy);

                    UIClipUtility::ClipBounds clip;
                    if(UIClipUtility::TryGetClipBounds(registry, anchor, clip)) {
                        // 枠は UI の座標、シザーは画面のピクセルなので変換する
                        const UIClipUtility::ClipBounds pixel = GetUICanvas(registry).ToPixel(clip);
                        screenClipped                         = true;
                        screenClip.left                       = static_cast<i32>(std::floor(pixel.left));
                        screenClip.top                        = static_cast<i32>(std::floor(pixel.top));
                        screenClip.right                      = static_cast<i32>(std::ceil(pixel.right));
                        screenClip.bottom                     = static_cast<i32>(std::ceil(pixel.bottom));
                    }
                }

                // ここまでは UI の座標。カメラは画面のピクセルで作るので、位置と大きさをピクセルにする
                const UICanvas& canvas = GetUICanvas(registry);
                screenPixel            = canvas.ToPixel(screenPixel);

                const hlslpp::float3             center = TransformUtility::GetWorldPosition(registry.GetComponent<TransformComponent>(screenOwner));
                Tsukino::Renderer::CBufferScene& camera = ctx->renderer->GetDrawQueue().AllocSceneData();
                camera = MakeScreenModelCamera(center, screenPixel, screenModel->pixelsPerUnit * canvas.scale, static_cast<float>(ctx->window->GetWidth()),
                                               static_cast<float>(ctx->window->GetHeight()));
                screenCamera = &camera;
            }

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

                    // 照らし方とトゥーンの値（既定は MaterialData の既定値）
                    Tsukino::GraphicsCommon::MaterialData defaultMaterial;
                    Tsukino::GraphicsCommon::ShadingModel shadingModel     = defaultMaterial.shadingModel;
                    float                                 toonThreshold    = defaultMaterial.toonThreshold;
                    float                                 toonSmoothness   = defaultMaterial.toonSmoothness;
                    hlslpp::float3                        toonShadeColor   = hlslpp::float3(defaultMaterial.toonShadeColor.x, defaultMaterial.toonShadeColor.y, defaultMaterial.toonShadeColor.z);
                    float                                 toonSpecularSize = defaultMaterial.toonSpecularSize;

                    // アルファテストのしきい値（0 = 無効）。ModelImporterが自動設定する
                    float alphaCutoff = 0.0f;

                    //--------------------------------------------------------------
                    // 使うマテリアル。ModelComponent::materials に差し替え（.tmat など）があればそれを、
                    // 無ければモデルのマテリアルを使う
                    //--------------------------------------------------------------
                    Tsukino::Asset::AssetHandle matHandle = Tsukino::Asset::AssetHandle::Invalid();
                    if(meshData.materialIndex < modelComp.materials.size() && modelComp.materials[meshData.materialIndex].IsValid())
                        matHandle = modelComp.materials[meshData.materialIndex];
                    else if(meshData.materialIndex < modelAsset->materialHandles.size())
                        matHandle = modelAsset->materialHandles[meshData.materialIndex];

                    if(matHandle.IsValid()) {
                        auto matAssetBase = ctx->assetManager->Get(matHandle);
                        if(matAssetBase && matAssetBase->GetType() == Tsukino::Asset::AssetType::Material) {
                            Tsukino::Core::Ref<Tsukino::Asset::MaterialAsset> matAsset = std::static_pointer_cast<Tsukino::Asset::MaterialAsset>(matAssetBase);

                            cbMat.baseColor =
                                hlslpp::float4(matAsset->data.baseColor.x, matAsset->data.baseColor.y, matAsset->data.baseColor.z, matAsset->data.baseColor.w);
                            cbMat.emissive  = hlslpp::float3(matAsset->data.emissive.x, matAsset->data.emissive.y, matAsset->data.emissive.z);
                            cbMat.metallic  = matAsset->data.metallic;
                            cbMat.roughness = matAsset->data.roughness;
                            cbMat.specular  = matAsset->data.specular;

                            shadingModel     = matAsset->data.shadingModel;
                            toonThreshold    = matAsset->data.toonThreshold;
                            toonSmoothness   = matAsset->data.toonSmoothness;
                            toonShadeColor   = hlslpp::float3(matAsset->data.toonShadeColor.x, matAsset->data.toonShadeColor.y, matAsset->data.toonShadeColor.z);
                            toonSpecularSize = matAsset->data.toonSpecularSize;
                            alphaCutoff      = matAsset->data.alphaCutoff;

                            // AssetHandle から SRV を引く（無効ハンドル・未ロードは nullptr）
                            auto resolveSRV = [&](const Tsukino::Asset::AssetHandle& handle) -> ID3D11ShaderResourceView* {
                                if(!handle.IsValid())
                                    return nullptr;

                                auto texAssetBase = ctx->assetManager->Get(handle);
                                if(!texAssetBase)
                                    return nullptr;

                                auto texAsset = std::static_pointer_cast<Tsukino::Asset::TextureAsset>(texAssetBase);
                                return ctx->renderer->GetResources().GetTextureSRV(*texAsset);
                            };

                            albedoSRV   = resolveSRV(matAsset->albedoHandle);
                            normalSRV   = resolveSRV(matAsset->normalHandle);
                            mrSRV       = resolveSRV(matAsset->metallicRoughnessHandle);
                            emissiveSRV = resolveSRV(matAsset->emissiveHandle);
                            aoSRV       = resolveSRV(matAsset->aoHandle);
                        }
                    }

                    //--------------------------------------------------------------
                    // エンティティ単位のマテリアルの値の上書き（MaterialPropertyBlock に当たる）。
                    // 値の入っている項目だけをマテリアルの値の代わりに使う（アセットは変えない）
                    //--------------------------------------------------------------
                    if(const auto* block = registry.try_get<MaterialPropertyBlockComponent>(entity)) {
                        if(block->baseColor)
                            cbMat.baseColor = *block->baseColor;
                        if(block->emissive)
                            cbMat.emissive = *block->emissive;
                        if(block->metallic)
                            cbMat.metallic = *block->metallic;
                        if(block->roughness)
                            cbMat.roughness = *block->roughness;
                        if(block->shadingModel)
                            shadingModel = *block->shadingModel;
                        if(block->toonThreshold)
                            toonThreshold = *block->toonThreshold;
                        if(block->toonSmoothness)
                            toonSmoothness = *block->toonSmoothness;
                        if(block->toonShadeColor)
                            toonShadeColor = *block->toonShadeColor;
                        if(block->toonSpecularSize)
                            toonSpecularSize = *block->toonSpecularSize;
                    }

                    // 照らし方。番号は ShadingModel の並び（シェーダー側の SHADING_MODEL_* と一致させる）
                    cbMat.shading        = hlslpp::float4(static_cast<float>(shadingModel), toonThreshold, toonSmoothness, toonSpecularSize);
                    cbMat.toonShadeColor = hlslpp::float4(toonShadeColor, 0.0f);

                    //--------------------------------------------------------------
                    // エンティティ単位のリムグローの上乗せ（マテリアルアセットより後に適用する）。
                    // コンポーネントが無い／非activeなら cbMat{} のゼロ初期化がそのまま残り、
                    // rimColorもrimIntensityも0なので描画には一切影響しない
                    //--------------------------------------------------------------
                    if(auto* rimGlow = registry.try_get<RimGlowComponent>(entity); rimGlow && rimGlow->active) {
                        cbMat.rimColor  = hlslpp::float4(rimGlow->rimColor, rimGlow->rimIntensity);
                        cbMat.rimParams = hlslpp::float4(rimGlow->rimPower, rimGlow->glow, 0.0f, 0.0f);
                    }

                    cbMat.alphaCutoff = alphaCutoff;

                    //--------------------------------------------------------------
                    // 半透明フェード（ModelComponent::opacity）。1.0未満なら通常のディファード
                    // （GBuffer）ではなく半透明フォワード（TransparentDepth + Transparent）で描く。
                    // ディファードのライティング結果はTonemapパスがrgbしか読まないため、
                    // baseColorのアルファを下げるだけではディファード経路は一切フェードしない
                    //--------------------------------------------------------------
                    bool isFading = modelComp.opacity < 0.999f;
                    if(isFading) {
                        cbMat.baseColor.w *= std::max(modelComp.opacity, 0.0f);
                    }

                    // 実体はキューが所有する（Render() の Clear() まで有効）
                    Tsukino::Renderer::CBufferMaterial* pCbMat = &ctx->renderer->GetDrawQueue().AllocMaterialData();
                    *pCbMat                                    = cbMat;

                    // シェーダーアセットの取得
                    // VSはワールド座標・法線・UVを出力するだけなので、フォワード（半透明）/
                    // ディファード(GBuffer)いずれのPSでも共用できる
                    Tsukino::Asset::AssetHandle vsHandle = isSkeletal ? ctx->builtinAssets->shaders.modelVS : ctx->builtinAssets->shaders.staticModelVS;
                    Tsukino::Asset::AssetHandle psHandle = ctx->builtinAssets->shaders.gbufferPS;

                    Tsukino::Renderer::BlendMode blendMode = Tsukino::Renderer::BlendMode::Opaque;

                    // opacityフェード中は半透明フォワードへ切り替える。
                    // Model.ps.hlslはt0（アルベド）しかサンプルしないため、フェード中だけ
                    // 法線/MR/エミッシブ/AOマップは効かなくなる
                    if(isFading) {
                        psHandle  = ctx->builtinAssets->shaders.modelPS;
                        blendMode = Tsukino::Renderer::BlendMode::Alpha;
                    }

                    // UI の層に描くモデルはトーンマップの後に描かれるので、トーンマップまで掛けるシェーダーで描く
                    if(screenModel) {
                        psHandle  = ctx->builtinAssets->shaders.screenModelPS;
                        blendMode = Tsukino::Renderer::BlendMode::Alpha;
                    }

                    auto vsAsset = std::static_pointer_cast<Tsukino::Asset::ShaderAsset>(ctx->assetManager->Get(vsHandle));
                    auto psAsset = std::static_pointer_cast<Tsukino::Asset::ShaderAsset>(ctx->assetManager->Get(psHandle));

                    if(!vsAsset || !psAsset)
                        continue;

                    // 頂点フォーマット
                    Tsukino::GraphicsCommon::VertexFormat vertexFormat =
                        isSkeletal ? Tsukino::GraphicsCommon::VertexFormat::Skinned : Tsukino::GraphicsCommon::VertexFormat::PositionNormalUV;

                    //--------------------------------------------------------------
                    // t0〜t4 をバインド。未設定はデフォルトへフォールバックする。
                    //   白        : シェーダー側で cbuffer 定数との乗算になるため恒等元
                    //   フラット法線: 適用しても頂点法線がそのまま保たれる
                    //--------------------------------------------------------------
                    ID3D11ShaderResourceView* whiteSRV      = ctx->renderer->GetResources().GetWhiteTextureSRV();
                    ID3D11ShaderResourceView* flatNormalSRV = ctx->renderer->GetResources().GetFlatNormalTextureSRV();

                    // 指定パイプラインでMaterialを1つ組み立てて、安定した参照を返す
                    // （複数DrawCommandから同じテクスチャ設定を使い回すためのヘルパー）
                    auto buildMaterial = [&](const std::shared_ptr<Tsukino::Renderer::PipelineState>& pipeline) -> Tsukino::Renderer::Material* {
                        if(!pipeline)
                            return nullptr;

                        Tsukino::Renderer::Material& mat = ctx->renderer->GetDrawQueue().AllocMaterial();
                        mat.SetPipeline(pipeline.get());
                        mat.SetSampler(ctx->renderer->GetResources().GetSampler(Tsukino::GraphicsCommon::SamplerType::AnisotropicWrap));
                        mat.SetTexture(Tsukino::Renderer::SRVSlot::Albedo, albedoSRV ? albedoSRV : whiteSRV);
                        mat.SetTexture(Tsukino::Renderer::SRVSlot::Normal, normalSRV ? normalSRV : flatNormalSRV);
                        mat.SetTexture(Tsukino::Renderer::SRVSlot::MetallicRoughness, mrSRV ? mrSRV : whiteSRV);
                        mat.SetTexture(Tsukino::Renderer::SRVSlot::Emissive, emissiveSRV ? emissiveSRV : whiteSRV);
                        mat.SetTexture(Tsukino::Renderer::SRVSlot::AO, aoSRV ? aoSRV : whiteSRV);

                        return &mat;
                    };

                    // 指定Material・パスでDrawCommandを1つ積むヘルパー
                    // （フェード中は同じメッシュを深度パス・色パスの2回積むため関数化する）
                    auto pushDrawCommand = [&](Tsukino::Renderer::Material* material, Tsukino::Renderer::RenderPass pass) {
                        Tsukino::Renderer::DrawCommand cmd{};
                        cmd.mesh         = const_cast<Tsukino::Renderer::MeshBuffer*>(&targetMeshBuffer);
                        cmd.transform    = finalTransform;
                        cmd.material     = material;
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
                        cmd.pass = pass;

                        ctx->renderer->GetDrawQueue().Push(cmd);
                    };

                    //--------------------------------------------------------------
                    // 閉じたメッシュは裏面を捨てる。カメラが敵の中へめり込んでも内面のテクスチャが見えない。
                    // ただし変換に鏡像（拡縮がマイナス。例: ノードの拡縮が -20,-20,-20 の FBX）が入っていると
                    // 三角形の巡回順が画面上で逆になり、裏面を捨てると手前の外側の面が消えて奥の内面が見える。
                    // その場合は表面を捨てて同じ見え方にする。鏡像かどうかは行列式の符号で判定する
                    // （拡縮・回転・移動だけのアフィン変換なので、4x4 の行列式は左上 3x3 の行列式と同じ）
                    //--------------------------------------------------------------
                    Tsukino::Renderer::CullMode cullMode = Tsukino::Renderer::CullMode::None;
                    if(!modelComp.doubleSided) {
                        const bool isMirrored = float(hlslpp::determinant(static_cast<const hlslpp::float4x4&>(finalTransform))) < 0.0f;
                        cullMode              = isMirrored ? Tsukino::Renderer::CullMode::Front : Tsukino::Renderer::CullMode::Back;
                    }

                    if(screenModel) {
                        // UI の層：画面スプライト・文字と同じ Overlay パスへ、sortOrder 付きで積む。
                        // モデル同士の前後は深度で決める（Renderer が最初の1つの前に深度を消す）
                        auto pipeline = ctx->renderer->GetResources().GetPipelineFactory()->Create(*vsAsset, *psAsset, vertexFormat,
                                                                                    Tsukino::Renderer::DepthMode::ReadWrite, blendMode, cullMode);
                        if(auto* mat = buildMaterial(pipeline)) {
                            Tsukino::Renderer::DrawCommand cmd{};
                            cmd.mesh           = const_cast<Tsukino::Renderer::MeshBuffer*>(&targetMeshBuffer);
                            cmd.transform      = finalTransform;
                            cmd.material       = mat;
                            cmd.materialData   = pCbMat;
                            cmd.pass           = Tsukino::Renderer::RenderPass::Overlay;
                            cmd.sortOrder      = screenModel->sortOrder;
                            cmd.cameraOverride = screenCamera;
                            cmd.castsShadow    = false;
                            cmd.hasClipRect    = screenClipped;
                            cmd.clipRect       = screenClip;
                            if(isSkeletal) {
                                cmd.boneMatrices = skeletonOut->local_matrices;
                                cmd.boneCount    = skeletonOut->bone_count;
                            }
                            ctx->renderer->GetDrawQueue().Push(cmd);
                        }
                    } else if(isFading) {
                        // 半透明フォワード：先に深度だけ埋め（スキンメッシュの自己重なり対策）、
                        // 続けてその深度と一致する画素だけを1回シェーディングする。
                        // 影・モーションベクタはGBufferパス限定のため、フェード中は失われる
                        auto depthPipeline = ctx->renderer->GetResources().GetPipelineFactory()->Create(
                            *vsAsset, *psAsset, vertexFormat, Tsukino::Renderer::DepthMode::ReadWrite, Tsukino::Renderer::BlendMode::DepthOnly,
                            cullMode);
                        auto colorPipeline = ctx->renderer->GetResources().GetPipelineFactory()->Create(
                            *vsAsset, *psAsset, vertexFormat, Tsukino::Renderer::DepthMode::EqualReadOnly, blendMode, cullMode);

                        if(auto* depthMat = buildMaterial(depthPipeline))
                            pushDrawCommand(depthMat, Tsukino::Renderer::RenderPass::TransparentDepth);
                        if(auto* colorMat = buildMaterial(colorPipeline))
                            pushDrawCommand(colorMat, Tsukino::Renderer::RenderPass::Transparent);
                    } else {
                        auto pipeline = ctx->renderer->GetResources().GetPipelineFactory()->Create(*vsAsset, *psAsset, vertexFormat,
                                                                                    Tsukino::Renderer::DepthMode::ReadWrite, blendMode, cullMode);
                        if(auto* mat = buildMaterial(pipeline)) {
                            // 不透明（PBR/Unlit/Toon）はG-Bufferパスへ回す
                            pushDrawCommand(mat, Tsukino::Renderer::RenderPass::GBuffer);
                        }
                    }
                }
            }
        });
    }

}    // namespace Tsukino::BuiltIn::ECS
