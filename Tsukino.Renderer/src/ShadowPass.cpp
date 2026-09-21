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

        //--------------------------------------------------------------------
        // 分割の結果をログへ出す。分割式は定数から機械的に決まるので、
        // 「近景カスケードが想定どおりの細かさになっているか」を
        // 起動ログだけで判断できるようにしておく
        //--------------------------------------------------------------------
        const std::array<float, kCascadeCount> extents = ComputeCascadeExtents();
        for(u32 i = 0; i < kCascadeCount; ++i) {
            Tsukino::Core::Log::Info("ShadowPass: cascade " + std::to_string(i) + " halfExtent=" + std::to_string(extents[i])
                                     + " texelWorld=" + std::to_string(2.0f * extents[i] / static_cast<float>(kMapSize)));
        }

        return CreateShadowMap(graphicsContext.GetDevice());
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

        const std::array<float, kCascadeCount> extents = ComputeCascadeExtents();

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

            const Tsukino::Core::Math::matrix& cascadeViewProj = m_frameConstants->GetWorldSceneData().cascadeViewProj[cascade];

            for(const auto& cmd : commands) {
                if(cmd.pass != RenderPass::GBuffer)
                    continue;

                // 頂点シェーダーが独自に頂点を組み立てるオブジェクトは、
                // 固定のシャドウ用シェーダーでは形を再現できないので外す
                if(!cmd.castsShadow)
                    continue;

                if(m_cullingEnabled && !OverlapsCascade(cmd, cascadeViewProj, extents[cascade]))
                    continue;

                m_commandExecutor->ExecuteShadow(cmd, m_staticPipeline.get(), m_skeletalPipeline.get(), stats);
            }
        }

        return true;
    }

    //------------------------------------------------------------------------
    //! 描画コマンドがカスケードの投影範囲に掛かっているかを判定します。
    //------------------------------------------------------------------------
    bool ShadowPass::OverlapsCascade(const DrawCommand& cmd, const Tsukino::Core::Math::matrix& cascadeViewProj, float halfExtent) {
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
        // ワールドの半径はhalfExtentで割るとクリップ空間の半径になる。
        // 奥行き（z）は見ない。全カスケードで共通の広い範囲を取ってあり、
        // ここで切ると「範囲外の高い所にある投影者」を落としてしまうため
        //--------------------------------------------------------------------
        const hlslpp::float4 clip = hlslpp::mul(centerWorld, cascadeViewProj);

        const float clipRadius = (halfExtent > 0.0f) ? (radiusWorld / halfExtent) : 0.0f;
        const float x          = static_cast<float>(clip.x);
        const float y          = static_cast<float>(clip.y);

        return (x + clipRadius >= -1.0f) && (x - clipRadius <= 1.0f) && (y + clipRadius >= -1.0f) && (y - clipRadius <= 1.0f);
    }

    //------------------------------------------------------------------------
    //! カスケードごとの平行投影の半径を求めます。
    //------------------------------------------------------------------------
    std::array<float, ShadowPass::kCascadeCount> ShadowPass::ComputeCascadeExtents() {
        std::array<float, kCascadeCount> extents{};

        for(u32 i = 0; i < kCascadeCount; ++i) {
            const float ratio = static_cast<float>(i + 1) / static_cast<float>(kCascadeCount);

            //----------------------------------------------------------------
            // 対数分割は視野に対して一定の角分解能を保つ理想的な分け方だが、
            // そのままだと第1カスケードが極端に狭くなる。
            // 等分割はその逆で近景が粗い。両者の重み付き平均を取るのが
            // practical split scheme（kSplitLambdaが対数側の重み）
            //----------------------------------------------------------------
            const float logSplit     = kSplitNear * std::pow(kShadowDistance / kSplitNear, ratio);
            const float uniformSplit = kSplitNear + (kShadowDistance - kSplitNear) * ratio;

            extents[i] = kSplitLambda * logSplit + (1.0f - kSplitLambda) * uniformSplit;
        }

        return extents;
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
    //! ディレクショナルライトの ViewProjection 行列を求めます。
    //------------------------------------------------------------------------
    Tsukino::Core::Math::matrix ShadowPass::ComputeLightViewProj(const hlslpp::float3& normalizedDir, const hlslpp::float3& focusPoint,
                                                                 float halfExtent) {
        //--------------------------------------------------------------------
        // ディレクショナルライトは平行投影を使う。
        //
        // シャドウの投影範囲は呼び出し側から渡されたfocusPoint（通常はメインカメラの
        // 注視点）を中心にする（ワールド原点固定だと、カメラが原点から離れる
        // プレイヤー追従型のシーンで影が一切映らなくなる）。
        // カメラ位置そのものを中心にしないのは、TPSカメラのように注視点から
        // 離れた位置にカメラを置く構成だと、画面に映る注視点付近がシャドウ範囲の
        // 端に寄ってしまい、キャラクターのすぐ近くで影が途切れて見えるため
        //--------------------------------------------------------------------
        hlslpp::float3 up = hlslpp::float3(0.0f, 1.0f, 0.0f);

        // ライト方向が真上/真下に近いときupベクトルが平行になるので回避
        float dotUp = std::abs(hlslpp::dot(normalizedDir, up));
        if(dotUp > 0.99f) {
            up = hlslpp::float3(0.0f, 0.0f, 1.0f);
        }

        //--------------------------------------------------------------------
        // View行列はワールド原点から光の向きへ見るものに固定し、注視点ではなく
        // 投影範囲（off-center）の側を動かす。
        //
        // 注視点に合わせてView行列ごと動かすと、注視点が1テクセル未満動くたびに
        // シャドウマップ上の格子もずれ、影の縁がカメラの移動に合わせてちらつく
        // （プレイヤー追従のTPSでは常に動いている）。原点基準のView空間で
        // 注視点の位置をテクセル幅の整数倍へ丸めてから範囲を置けば、格子は
        // ワールドに対して動かず、影の縁は止まって見える
        //--------------------------------------------------------------------
        const hlslpp::float3        origin    = hlslpp::float3(0.0f, 0.0f, 0.0f);
        Tsukino::Core::Math::matrix lightView = Tsukino::Core::Math::matrix::lookAtLH(origin, normalizedDir, up);

        const hlslpp::float4 focusInLight = hlslpp::mul(hlslpp::float4(focusPoint, 1.0f), lightView);

        const float texelWorldSize = 2.0f * halfExtent / static_cast<float>(kMapSize);
        const float centerX        = std::floor(static_cast<float>(focusInLight.x) / texelWorldSize) * texelWorldSize;
        const float centerY        = std::floor(static_cast<float>(focusInLight.y) / texelWorldSize) * texelWorldSize;
        const float focusDepth     = static_cast<float>(focusInLight.z);

        // リバースZのため far → near の順に渡す（以前の「注視点から500後ろにライト、near1 / far2000」と同じ奥行き）
        Tsukino::Core::Math::matrix lightProj = Tsukino::Core::Math::matrix::orthographicOffCenterLH(centerX - halfExtent,    // left
                                                                                                     centerX + halfExtent,    // right
                                                                                                     centerY - halfExtent,    // bottom
                                                                                                     centerY + halfExtent,    // top
                                                                                                     focusDepth + kDepthAwayFromLight,    // far
                                                                                                     focusDepth - kDepthTowardLight       // near
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
