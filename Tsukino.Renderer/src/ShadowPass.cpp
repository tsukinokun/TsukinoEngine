//----------------------------------------------------------------------------
//! @file   ShadowPass.cpp
//! @brief  シャドウマップパスの実装
//----------------------------------------------------------------------------
#include "ShadowPass.hpp"
#include "DrawCommandExecutor.hpp"

#include <Tsukino/Renderer/ShaderSlots.hpp>

#include <Tsukino/Engine/Asset/Shader/ShaderAsset.hpp>

#include <Tsukino/Core/Log.hpp>

#include <string>

#include <algorithm>
#include <array>
#include <cmath>

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {
    //------------------------------------------------------------------------
    //! シャドウマップと影用パイプラインを作成します。
    //------------------------------------------------------------------------
    bool ShadowPass::Initialize(GraphicsContext&                   graphicsContext,
                                RenderResources&                   resources,
                                FrameConstants&                    frameConstants,
                                DrawCommandExecutor&               commandExecutor,
                                const Tsukino::Asset::ShaderAsset* staticVS,
                                const Tsukino::Asset::ShaderAsset* skeletalVS,
                                const Tsukino::Asset::ShaderAsset* ps) {
        m_graphicsContext = &graphicsContext;
        m_frameConstants  = &frameConstants;
        m_commandExecutor = &commandExecutor;

        PipelineFactory* factory = resources.GetPipelineFactory();
        if(!factory)
            return false;

        // 静的メッシュ用シャドウパイプライン
        if(staticVS && ps) {
            m_staticPipeline = factory->Create(*staticVS, *ps, Tsukino::GraphicsCommon::VertexFormat::PositionNormalUV, DepthMode::ReadWrite);
            if(!m_staticPipeline) {
                Tsukino::Core::Log::Error("Renderer: Shadow Static Pipeline generation failed.");
                return false;
            }
        }

        // スキニングメッシュ用シャドウパイプライン
        if(skeletalVS && ps) {
            m_skeletalPipeline = factory->Create(*skeletalVS, *ps, Tsukino::GraphicsCommon::VertexFormat::Skinned, DepthMode::ReadWrite);
            if(!m_skeletalPipeline) {
                Tsukino::Core::Log::Error("Renderer: Shadow Skeletal Pipeline generation failed.");
                return false;
            }
        }

        return CreateShadowMap(graphicsContext.GetDevice());
    }

    //------------------------------------------------------------------------
    //! このフレームのカスケードを求めて、ワールドのシーン定数（b0）へ書き込みます。
    //------------------------------------------------------------------------
    void ShadowPass::UpdateCascades(const CBufferScene& worldScene) {
        // ディレクショナルライトが一度も設定されていなければ、向きが無いので投影を作れない
        const hlslpp::float3 lightDir = worldScene.lightDir.xyz;
        if(static_cast<float>(hlslpp::dot(lightDir, lightDir)) <= 0.0f)
            return;

        m_cascades = ComputeCascades(worldScene.view, worldScene.invViewProj, hlslpp::normalize(lightDir));

        //--------------------------------------------------------------------
        // シェーダーへ渡す形に並べ替える。
        // テクセル幅と分割距離は float4 1本ずつで運ぶので、カスケードは最大4枚まで
        //--------------------------------------------------------------------
        static_assert(kCascadeCount <= 4, "cascadeTexelWorld and cascadeSplitFar are single float4s; they hold at most 4 cascades.");

        std::array<Tsukino::Core::Math::matrix, kCascadeCount> cascadeViewProj{};
        float                                                  texelWorld[4]{};
        float                                                  splitFar[4]{};

        for(u32 i = 0; i < kCascadeCount; ++i) {
            cascadeViewProj[i] = m_cascades[i].lightViewProj;
            texelWorld[i]      = m_cascades[i].texelWorldSize;
            splitFar[i]        = m_cascades[i].splitFar;
        }

        m_frameConstants->SetShadowCascades(cascadeViewProj, hlslpp::float4(texelWorld[0], texelWorld[1], texelWorld[2], texelWorld[3]),
                                            hlslpp::float4(splitFar[0], splitFar[1], splitFar[2], splitFar[3]));
    }

    //------------------------------------------------------------------------
    //! GBuffer パスの描画コマンドのうち、影を落とすものをシャドウマップへ描きます。
    //------------------------------------------------------------------------
    bool ShadowPass::Execute(const std::vector<DrawCommand>& commands, Renderer::FrameStats& stats) {
        if(!m_staticPipeline && !m_skeletalPipeline)
            return false;

        ID3D11DeviceContext* context = m_graphicsContext->GetContext();

        // ビューポートは全カスケード共通（スライスの大きさは同じ）
        D3D11_VIEWPORT vp{};
        vp.Width    = static_cast<float>(kMapSize);
        vp.Height   = static_cast<float>(kMapSize);
        vp.MaxDepth = 1.0f;
        context->RSSetViewports(1, &vp);

        for(u32 cascade = 0; cascade < kCascadeCount; ++cascade) {
            //----------------------------------------------------------------
            // 描画中のカスケード番号を b0 へ流す。
            // 頂点シェーダーはこれを見て cascadeViewProj[] のどれを使うかを決めるので、
            // シャドウ用シェーダーはカスケードの枚数を知らなくて済む
            //----------------------------------------------------------------
            m_frameConstants->SetShadowCascadeIndex(cascade);
            m_frameConstants->UploadWorld();

            // このスライスだけをクリアしてバインドする（カラーRTはnull）
            context->ClearDepthStencilView(m_mapDSV[cascade].Get(), D3D11_CLEAR_DEPTH, 0.0f, 0);

            ID3D11RenderTargetView* nullRTV = nullptr;
            context->OMSetRenderTargets(1, &nullRTV, m_mapDSV[cascade].Get());

            for(const auto& cmd : commands) {
                if(cmd.pass != RenderPass::GBuffer)
                    continue;

                // 影を落とさない物は外す。頂点シェーダーが独自に頂点を組み立てる物は、
                // 同じ動きの影用パイプライン（DrawCommand::shadowPipeline）を渡せば影を落とせる。
                // 渡さないなら固定のシャドウ用シェーダーでは形を再現できないので、castsShadow を false にしておく
                if(!cmd.castsShadow)
                    continue;

                if(m_cullingEnabled && !OverlapsCascade(cmd, m_cascades[cascade]))
                    continue;

                m_commandExecutor->ExecuteShadow(cmd, m_staticPipeline.get(), m_skeletalPipeline.get(), stats);
            }
        }

        return true;
    }

    //------------------------------------------------------------------------
    //! 描画コマンドがカスケードの投影範囲に掛かっているかを判定します。
    //------------------------------------------------------------------------
    bool ShadowPass::OverlapsCascade(const DrawCommand& cmd, const ShadowCascade& cascade) {
        if(!cmd.mesh)
            return false;

        //--------------------------------------------------------------------
        // 判定しないケース。
        //
        // バウンド半径0はMeshDataがboundsを持っていない経路（プリミティブ等）。
        // インスタンス描画はメッシュ単体のバウンドが全インスタンスを覆わない
        // （草がその例。今はcastsShadow=falseだが、ここでも明示的に守る）。
        // どちらも「分からないものは消さない」側へ倒す
        //--------------------------------------------------------------------
        if(cmd.mesh->boundsRadius <= 0.0f || cmd.instanceCount > 1)
            return true;

        //--------------------------------------------------------------------
        // ローカルのバウンド球をワールドへ移す。
        // 半径はワールド行列の最大軸スケールで伸ばす（非一様スケールでも
        // 覆い漏らさない側に倒す）
        //--------------------------------------------------------------------
        const hlslpp::float4 centerWorld = hlslpp::mul(hlslpp::float4(cmd.mesh->boundsCenter, 1.0f), cmd.transform);

        const auto axisLength = [&cmd](int row) {
            const hlslpp::float4 axis = cmd.transform[row];
            return std::sqrt(static_cast<float>(axis.x) * static_cast<float>(axis.x) + static_cast<float>(axis.y) * static_cast<float>(axis.y)
                             + static_cast<float>(axis.z) * static_cast<float>(axis.z));
        };
        const float scale = std::max({axisLength(0), axisLength(1), axisLength(2)});

        float radiusWorld = cmd.mesh->boundsRadius * scale;

        // スキンメッシュはバインドポーズのAABBなので、アニメではみ出す分を見込む
        if(cmd.boneMatrices && cmd.boneCount > 0)
            radiusWorld *= kSkinnedBoundsInflate;

        //--------------------------------------------------------------------
        // カスケードのクリップ空間（平行投影なのでw=1、xyは-1〜1）で判定する。
        // ワールドの半径は平行投影の半径（＝カスケードを包む球の半径）で割ると
        // クリップ空間の半径になる。
        // 奥行き（z）は見ない。全カスケードで共通の広い範囲を取ってあり、
        // ここで切ると「範囲外の高い所にある投影者」を落としてしまうため
        //--------------------------------------------------------------------
        const hlslpp::float4 clip = hlslpp::mul(centerWorld, cascade.lightViewProj);

        const float boundsRadius = cascade.bounds.radius;
        const float clipRadius   = (boundsRadius > 0.0f) ? (radiusWorld / boundsRadius) : 0.0f;
        const float x          = static_cast<float>(clip.x);
        const float y          = static_cast<float>(clip.y);

        return (x + clipRadius >= -1.0f) && (x - clipRadius <= 1.0f) && (y + clipRadius >= -1.0f) && (y - clipRadius <= 1.0f);
    }

    //------------------------------------------------------------------------
    //! カメラの視錐台をカスケードに切り分け、1枚ずつのライト空間の投影を求めます。
    //------------------------------------------------------------------------
    std::array<ShadowCascade, ShadowPass::kCascadeCount> ShadowPass::ComputeCascades(const Tsukino::Core::Math::matrix& cameraView,
                                                                                     const Tsukino::Core::Math::matrix& cameraInvViewProj,
                                                                                     const hlslpp::float3&              lightDir) {
        std::array<ShadowCascade, kCascadeCount> cascades{};

        // 手順0: カメラの視錐台をワールド空間の8頂点として取り出す
        const FrustumCorners frustum = ComputeFrustumCorners(cameraView, cameraInvViewProj);

        // カメラ行列がまだ設定されていない（最初のフレームなど）と奥行きが求まらない。
        // NaN でもここで弾けるよう、否定の形で比べる
        if(!(frustum.farDepth > frustum.nearDepth))
            return cascades;

        // 手順1: 視錐台を奥行き方向に切る距離を決める
        const std::array<float, kCascadeCount + 1> splitDepths = ComputeSplitDepths(frustum.nearDepth, frustum.farDepth);

        for(u32 i = 0; i < kCascadeCount; ++i) {
            ShadowCascade& cascade = cascades[i];
            cascade.splitNear      = splitDepths[i];
            cascade.splitFar       = splitDepths[i + 1];

            // 手順2: 切れ端の8頂点を求め、それを包む球を作る
            const std::array<hlslpp::float3, 8> sliceCorners = ComputeSliceCorners(frustum, cascade.splitNear, cascade.splitFar);
            cascade.bounds                                   = ComputeBoundingSphere(sliceCorners);

            // 手順3で中心をテクセル幅へ丸めると、投影範囲が最大1テクセルずれて球の端がはみ出す。
            // 1テクセル分の余白を足しておく（半径は切り上げ済みの整数から決まるので、揺れない）
            cascade.bounds.radius += ComputeTexelWorldSize(cascade.bounds.radius);
            cascade.texelWorldSize = ComputeTexelWorldSize(cascade.bounds.radius);

            // 手順3: 球を覆う平行投影をライト空間で作る
            cascade.lightViewProj = ComputeLightViewProj(lightDir, cascade.bounds);
        }

        return cascades;
    }

    //------------------------------------------------------------------------
    //! カメラの視錐台の8頂点を、NDC の立方体の頂点を逆射影して求めます。
    //------------------------------------------------------------------------
    FrustumCorners ShadowPass::ComputeFrustumCorners(const Tsukino::Core::Math::matrix& cameraView,
                                                     const Tsukino::Core::Math::matrix& cameraInvViewProj) {
        //--------------------------------------------------------------------
        // 画面の四隅（NDC の xy）。near 面と far 面で同じ並びにしておくと、
        // nearPlane[i] と farPlane[i] がカメラから伸びる同じ稜線の両端になる
        //--------------------------------------------------------------------
        constexpr std::array<std::array<float, 2>, 4> kScreenCorners = {{
            {{-1.0f, -1.0f}},    // 左下
            {{+1.0f, -1.0f}},    // 右下
            {{+1.0f, +1.0f}},    // 右上
            {{-1.0f, +1.0f}},    // 左上
        }};

        // リバースZのため、NDC の z は near 面が1、far 面が0
        constexpr float kNearNdcZ = 1.0f;
        constexpr float kFarNdcZ  = 0.0f;

        // NDC の点をワールドへ戻す。透視投影ではwが1でないので、最後にwで割る
        const auto unproject = [&cameraInvViewProj](const std::array<float, 2>& screen, float ndcZ) {
            const hlslpp::float4 world = hlslpp::mul(hlslpp::float4(screen[0], screen[1], ndcZ, 1.0f), cameraInvViewProj);
            return hlslpp::float3(world.xyz) / hlslpp::float1(world.w);
        };

        FrustumCorners frustum{};
        for(u32 i = 0; i < 4; ++i) {
            frustum.nearPlane[i] = unproject(kScreenCorners[i], kNearNdcZ);
            frustum.farPlane[i]  = unproject(kScreenCorners[i], kFarNdcZ);
        }

        //--------------------------------------------------------------------
        // 奥行きは View 行列でビュー空間へ移した z。
        // near 面・far 面はビュー空間の z が一定の平面なので、どの頂点で測っても同じ
        //--------------------------------------------------------------------
        const auto viewDepth = [&cameraView](const hlslpp::float3& world) {
            return static_cast<float>(hlslpp::mul(hlslpp::float4(world, 1.0f), cameraView).z);
        };
        frustum.nearDepth = viewDepth(frustum.nearPlane[0]);
        frustum.farDepth  = viewDepth(frustum.farPlane[0]);

        return frustum;
    }

    //------------------------------------------------------------------------
    //! 視錐台を奥行き方向に切る距離を求めます。
    //------------------------------------------------------------------------
    std::array<float, ShadowPass::kCascadeCount + 1> ShadowPass::ComputeSplitDepths(float nearDepth, float farDepth) {
        std::array<float, kCascadeCount + 1> splitDepths{};

        // 影を出すのはカメラの far 面と kShadowDistance の近いほうまで
        const float shadowFar = std::min(kShadowDistance, farDepth);

        //--------------------------------------------------------------------
        // 式の起点。カメラの near から始めると、TPSカメラでは何も写っていない
        // 手前の空間にカスケードを使い切ってしまう（kShadowNear参照）。
        // 起点より手前はカスケード0がまとめて受け持つ（下の splitDepths.front()）
        //--------------------------------------------------------------------
        // clampにしないのは、near が kShadowDistance より奥のカメラで lo > hi になるため
        const float splitStart = std::min(std::max(kShadowNear, nearDepth), shadowFar);

        // 対数分割は起点が0だと計算できない（正射影のカメラで起こりうる）ので、下限を設ける
        const float logNear = std::max(splitStart, 1.0e-3f);

        for(u32 i = 0; i <= kCascadeCount; ++i) {
            const float ratio = static_cast<float>(i) / static_cast<float>(kCascadeCount);

            //----------------------------------------------------------------
            // 対数分割は、カメラから見た1テクセルの大きさがどの奥行きでも同じに
            // なる理想的な分け方だが、そのままだと第1カスケードが極端に薄くなる。
            // 等分割はその逆で近景が粗い。両者の重み付き平均を取るのが
            // practical split scheme（kSplitLambdaが対数側の重み）
            //----------------------------------------------------------------
            const float logSplit     = logNear * std::pow(shadowFar / logNear, ratio);
            const float uniformSplit = splitStart + (shadowFar - splitStart) * ratio;

            splitDepths[i] = kSplitLambda * logSplit + (1.0f - kSplitLambda) * uniformSplit;
        }

        // 両端は式の丸め誤差を残さず、ちょうど near と shadowFar にする。
        // 先頭を splitStart ではなく near にするので、カスケード0は起点より手前も受け持つ
        splitDepths.front() = nearDepth;
        splitDepths.back()  = shadowFar;

        return splitDepths;
    }

    //------------------------------------------------------------------------
    //! 視錐台を2つの奥行きで切った切れ端の8頂点を求めます。
    //------------------------------------------------------------------------
    std::array<hlslpp::float3, 8> ShadowPass::ComputeSliceCorners(const FrustumCorners& frustum, float splitNear, float splitFar) {
        //--------------------------------------------------------------------
        // 視錐台の稜線（near 面の頂点から far 面の頂点へ向かう辺）の上では、
        // ビュー空間の z は位置に比例して変わる。だから「奥行き splitNear の点」は
        // 稜線を (splitNear - nearDepth) / (farDepth - nearDepth) の割合で
        // 線形補間すれば、ちょうど求まる
        //--------------------------------------------------------------------
        const float depthRange = frustum.farDepth - frustum.nearDepth;
        const float nearRatio  = (splitNear - frustum.nearDepth) / depthRange;
        const float farRatio   = (splitFar - frustum.nearDepth) / depthRange;

        std::array<hlslpp::float3, 8> sliceCorners{};
        for(u32 i = 0; i < 4; ++i) {
            const hlslpp::float3 edge = frustum.farPlane[i] - frustum.nearPlane[i];

            sliceCorners[i]     = frustum.nearPlane[i] + edge * hlslpp::float1(nearRatio);    // 手前の4頂点
            sliceCorners[i + 4] = frustum.nearPlane[i] + edge * hlslpp::float1(farRatio);     // 奥の4頂点
        }

        return sliceCorners;
    }

    //------------------------------------------------------------------------
    //! 点の集まりを包む球を求めます。
    //------------------------------------------------------------------------
    BoundingSphere ShadowPass::ComputeBoundingSphere(const std::array<hlslpp::float3, 8>& corners) {
        //--------------------------------------------------------------------
        // 箱（AABB）ではなく球で包む理由。
        // 切れ端をライト空間の箱でぴったり包むと、カメラを回すたびに箱の大きさが
        // 変わり、1テクセルの大きさも変わって影の縁がちらつく。
        // 球の大きさはカメラの向きに依らないので、カメラを回しても投影範囲は
        // 平行移動するだけになり、テクセル幅に丸める手順3と組み合わせると影が止まって見える。
        // 球のほうが隙間が多く解像度は少し損をするが、揺れないことを優先する
        //--------------------------------------------------------------------
        hlslpp::float3 center = hlslpp::float3(0.0f, 0.0f, 0.0f);
        for(const hlslpp::float3& corner : corners)
            center += corner;
        // 複合代入（center /= float1）にしない。float1 が3成分へ広がらずxだけを割り、
        // yzを0で割って中心がinfになる（半径もinfになり、影が1つも描かれなかった）。
        // 二項の演算子なら広がるので、こちらで書く
        center = center / hlslpp::float1(static_cast<float>(corners.size()));

        float radius = 0.0f;
        for(const hlslpp::float3& corner : corners)
            radius = std::max(radius, static_cast<float>(hlslpp::length(corner - center)));

        // 数学的には向きに依らない半径も、浮動小数点では毎フレーム末尾の桁が揺れる。
        // 揺れるとテクセル幅も揺れるので、切り上げて値を固定する
        radius = std::ceil(radius);

        return BoundingSphere{center, radius};
    }

    //------------------------------------------------------------------------
    //! シャドウマップ（t8）と比較サンプラー（s8）をピクセルシェーダーへバインドします。
    //------------------------------------------------------------------------
    void ShadowPass::BindForSampling(ID3D11DeviceContext* context) const {
        constexpr UINT srvSlot     = static_cast<UINT>(SRVSlot::ShadowMap);
        constexpr UINT samplerSlot = static_cast<UINT>(SamplerSlot::ShadowMap);
        context->PSSetShaderResources(srvSlot, 1, m_mapSRV.GetAddressOf());
        context->PSSetSamplers(samplerSlot, 1, m_sampler.GetAddressOf());
    }

    //------------------------------------------------------------------------
    //! 球を覆うディレクショナルライトの ViewProjection 行列を、カスケード1枚分求めます。
    //------------------------------------------------------------------------
    Tsukino::Core::Math::matrix ShadowPass::ComputeLightViewProj(const hlslpp::float3& lightDir, const BoundingSphere& bounds) {
        //--------------------------------------------------------------------
        // ディレクショナルライトは平行投影を使う。
        // 投影範囲は、視錐台の切れ端を包む球を、光の向きから見てちょうど覆う正方形
        //--------------------------------------------------------------------
        hlslpp::float3 up = hlslpp::float3(0.0f, 1.0f, 0.0f);

        // ライト方向が真上/真下に近いときupベクトルが平行になるので回避
        float dotUp = std::abs(hlslpp::dot(lightDir, up));
        if(dotUp > 0.99f) {
            up = hlslpp::float3(0.0f, 0.0f, 1.0f);
        }

        //--------------------------------------------------------------------
        // View行列はワールド原点から光の向きへ見るものに固定し、球の中心ではなく
        // 投影範囲（off-center）の側を動かす。
        //
        // 球の中心に合わせてView行列ごと動かすと、中心が1テクセル未満動くたびに
        // シャドウマップ上の格子もずれ、影の縁がカメラの移動に合わせてちらつく
        // （プレイヤー追従のTPSでは常に動いている）。原点基準のView空間で
        // 中心の位置をテクセル幅の整数倍へ丸めてから範囲を置けば、格子は
        // ワールドに対して動かず、影の縁は止まって見える
        //--------------------------------------------------------------------
        const hlslpp::float3        origin    = hlslpp::float3(0.0f, 0.0f, 0.0f);
        Tsukino::Core::Math::matrix lightView = Tsukino::Core::Math::matrix::lookAtLH(origin, lightDir, up);

        const hlslpp::float4 centerInLight = hlslpp::mul(hlslpp::float4(bounds.center, 1.0f), lightView);

        const float radius         = bounds.radius;
        const float texelWorldSize = ComputeTexelWorldSize(radius);
        const float centerX        = std::floor(static_cast<float>(centerInLight.x) / texelWorldSize) * texelWorldSize;
        const float centerY        = std::floor(static_cast<float>(centerInLight.y) / texelWorldSize) * texelWorldSize;
        const float centerDepth    = static_cast<float>(centerInLight.z);

        // リバースZのため far → near の順に渡す
        Tsukino::Core::Math::matrix lightProj = Tsukino::Core::Math::matrix::orthographicOffCenterLH(centerX - radius,                     // left
                                                                                                     centerX + radius,                     // right
                                                                                                     centerY - radius,                     // bottom
                                                                                                     centerY + radius,                     // top
                                                                                                     centerDepth + kDepthAwayFromLight,    // far
                                                                                                     centerDepth - kDepthTowardLight       // near
        );

        return hlslpp::mul(lightView, lightProj);
    }

    //------------------------------------------------------------------------
    //! シャドウマップ（テクスチャ・DSV・SRV・比較サンプラー）を作成します。
    //------------------------------------------------------------------------
    bool ShadowPass::CreateShadowMap(ID3D11Device* device) {
        //--------------------------------------------------------------------
        // シャドウマップテクスチャの作成
        // R32_TYPELESS : DSV(D32_FLOAT)とSRV(R32_FLOAT)で共有するため
        //--------------------------------------------------------------------
        D3D11_TEXTURE2D_DESC texDesc{};
        texDesc.Width            = kMapSize;
        texDesc.Height           = kMapSize;
        texDesc.MipLevels        = 1;
        texDesc.ArraySize        = kCascadeCount;    // カスケード1枚＝配列の1スライス
        texDesc.Format           = DXGI_FORMAT_R32_TYPELESS;
        texDesc.SampleDesc.Count = 1;
        texDesc.Usage            = D3D11_USAGE_DEFAULT;
        texDesc.BindFlags        = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;

        if(FAILED(device->CreateTexture2D(&texDesc, nullptr, m_mapTex.GetAddressOf()))) {
            Tsukino::Core::Log::Error("Failed to create shadow map texture.");
            return false;
        }

        //--------------------------------------------------------------------
        // DSVはスライスごとに1本ずつ作る。
        // 書き込みは「1枚のカスケード」単位で、配列全体をまとめてバインドすることは無い
        //--------------------------------------------------------------------
        for(u32 i = 0; i < kCascadeCount; ++i) {
            D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
            dsvDesc.Format                         = DXGI_FORMAT_D32_FLOAT;
            dsvDesc.ViewDimension                  = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
            dsvDesc.Texture2DArray.MipSlice        = 0;
            dsvDesc.Texture2DArray.FirstArraySlice = i;
            dsvDesc.Texture2DArray.ArraySize       = 1;

            if(FAILED(device->CreateDepthStencilView(m_mapTex.Get(), &dsvDesc, m_mapDSV[i].GetAddressOf()))) {
                Tsukino::Core::Log::Error("Failed to create shadow map DSV for cascade " + std::to_string(i) + ".");
                return false;
            }
        }

        //--------------------------------------------------------------------
        // SRVは配列全体で1本。シェーダーはTexture2DArrayとして読み、
        // 第3成分でカスケードを選ぶ。スロット(t8)は1枚のときと変わらない
        //--------------------------------------------------------------------
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
        srvDesc.Format                         = DXGI_FORMAT_R32_FLOAT;
        srvDesc.ViewDimension                  = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
        srvDesc.Texture2DArray.MostDetailedMip = 0;
        srvDesc.Texture2DArray.MipLevels       = 1;
        srvDesc.Texture2DArray.FirstArraySlice = 0;
        srvDesc.Texture2DArray.ArraySize       = kCascadeCount;

        if(FAILED(device->CreateShaderResourceView(m_mapTex.Get(), &srvDesc, m_mapSRV.GetAddressOf()))) {
            Tsukino::Core::Log::Error("Failed to create shadow map SRV.");
            return false;
        }

        //--------------------------------------------------------------------
        // PCF用比較サンプラーの作成
        // SampleCmpLevelZero で使用する
        //--------------------------------------------------------------------
        D3D11_SAMPLER_DESC samplerDesc{};
        samplerDesc.Filter         = D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
        samplerDesc.AddressU       = D3D11_TEXTURE_ADDRESS_BORDER;
        samplerDesc.AddressV       = D3D11_TEXTURE_ADDRESS_BORDER;
        samplerDesc.AddressW       = D3D11_TEXTURE_ADDRESS_BORDER;
        samplerDesc.BorderColor[0] = 1.0f;    // 範囲外は「影なし」にする
        samplerDesc.BorderColor[1] = 1.0f;
        samplerDesc.BorderColor[2] = 1.0f;
        samplerDesc.BorderColor[3] = 1.0f;
        samplerDesc.ComparisonFunc = D3D11_COMPARISON_GREATER_EQUAL;
        samplerDesc.MinLOD         = 0;
        samplerDesc.MaxLOD         = D3D11_FLOAT32_MAX;

        if(FAILED(device->CreateSamplerState(&samplerDesc, m_sampler.GetAddressOf()))) {
            Tsukino::Core::Log::Error("Failed to create shadow sampler.");
            return false;
        }

        return true;
    }
}    // namespace Tsukino::Renderer
