//----------------------------------------------------------------------------
//! @file   ShadowPass.cpp
//! @brief  シャドウマップパスの実装
//----------------------------------------------------------------------------
#include "ShadowPass.hpp"
#include <Tsukino/Renderer/DX11/GraphicsContext.hpp>
#include "DrawCommandExecutor.hpp"

#include <Tsukino/Renderer/ShaderSlots.hpp>

#include <Tsukino/Engine/Asset/Shader/ShaderAsset.hpp>

#include <Tsukino/Core/Log.hpp>

#include <string>

#include <algorithm>
#include <array>
#include <bit>
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
        // b0 の shadowParams へ配る値の仮置き。
        // 奥行きは最初の UpdateCascades() でカスケードの大きさから決め直すので、
        // ここでは0除算にならない値を入れておくだけ
        //--------------------------------------------------------------------
        m_frameConstants->SetShadowMapParams(m_settings.mapSize, m_settings.distance * 2.0f + m_settings.casterMargin);

        return CreateShadowMap(graphicsContext.GetDevice(), m_settings.mapSize);
    }

    //------------------------------------------------------------------------
    //! 影の調整値を設定します。
    //------------------------------------------------------------------------
    void ShadowPass::SetSettings(const ShadowSettings& settings) {
        m_requestedSettings = settings;
        ApplySettings();
    }

    //------------------------------------------------------------------------
    //! 描画品質による影の上限を設定します。
    //------------------------------------------------------------------------
    void ShadowPass::SetQuality(u32 mapSizeLimit, u32 cascadeCount) {
        m_mapSizeLimit       = mapSizeLimit;
        m_activeCascadeCount = std::clamp(cascadeCount, 1u, kCascadeCount);
        ApplySettings();
    }

    //------------------------------------------------------------------------
    //! シーンの調整値へ描画品質の上限を掛けて適用します。
    //------------------------------------------------------------------------
    void ShadowPass::ApplySettings() {
        ShadowSettings limited = m_requestedSettings;
        if(m_mapSizeLimit > 0)
            limited.mapSize = std::min(limited.mapSize, m_mapSizeLimit);

        const u32 previousMapSize = m_settings.mapSize;
        m_settings                = ShadowCascades::SanitizeSettings(limited);

        // Initialize() の前ならシャドウマップはまだ無く、Initialize() が新しい解像度で作る
        if(!m_mapTex || m_settings.mapSize == previousMapSize)
            return;

        //--------------------------------------------------------------------
        // 解像度が変わったのでシャドウマップを作り直す。
        // 失敗したときに影が丸ごと消えないよう、前の資源を握っておいて戻せるようにする
        //--------------------------------------------------------------------
        const ComPtr<ID3D11Texture2D>                                   oldTex     = m_mapTex;
        const ComPtr<ID3D11ShaderResourceView>                          oldSRV     = m_mapSRV;
        const ComPtr<ID3D11SamplerState>                                oldSampler = m_sampler;
        const std::array<ComPtr<ID3D11DepthStencilView>, kCascadeCount> oldDSV     = m_mapDSV;

        if(!CreateShadowMap(m_graphicsContext->GetDevice(), m_settings.mapSize)) {
            Tsukino::Core::Log::Error("Failed to recreate the shadow map at " + std::to_string(m_settings.mapSize)
                                      + " px; keeping the previous " + std::to_string(previousMapSize) + " px shadow map.");
            m_mapTex           = oldTex;
            m_mapSRV           = oldSRV;
            m_sampler          = oldSampler;
            m_mapDSV           = oldDSV;
            m_settings.mapSize = previousMapSize;
        }
    }

    //------------------------------------------------------------------------
    //! このフレームのカスケードを求めて、ワールドのシーン定数（b0）へ書き込みます。
    //------------------------------------------------------------------------
    void ShadowPass::UpdateCascades(const CBufferScene& worldScene) {
        // ディレクショナルライトが一度も設定されていなければ、向きが無いので投影を作れない
        const hlslpp::float3 lightDir = worldScene.lightDir.xyz;
        if(static_cast<float>(hlslpp::dot(lightDir, lightDir)) <= 0.0f)
            return;

        const ShadowCascadeSet cascadeSet = ShadowCascades::ComputeCascades(worldScene.view, worldScene.invViewProj, hlslpp::normalize(lightDir), m_settings, m_activeCascadeCount);
        m_cascades                        = cascadeSet.cascades;

        // 奥行きはカスケードの大きさから毎フレーム決まる。カメラ行列が未設定で求まらなかったフレームは前の値を残す
        const float depthRange = cascadeSet.depthTowardLight + cascadeSet.depthAwayFromLight;
        if(depthRange > 0.0f)
            m_frameConstants->SetShadowMapParams(m_settings.mapSize, depthRange);

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
        vp.Width    = static_cast<float>(m_settings.mapSize);
        vp.Height   = static_cast<float>(m_settings.mapSize);
        vp.MaxDepth = 1.0f;
        context->RSSetViewports(1, &vp);

        for(u32 cascade = 0; cascade < kCascadeCount; ++cascade) {
            // このスライスだけをクリアする
            context->ClearDepthStencilView(m_mapDSV[cascade].Get(), D3D11_CLEAR_DEPTH, 0.0f, 0);

            //----------------------------------------------------------------
            // 使わないカスケードは消すだけで描かない。
            // シェーダーが最後の有効なカスケードの境目で次のスライスを読むと
            // 「影なし」が返るので、影を出す奥行きの端で影が自然に消える
            //----------------------------------------------------------------
            if(cascade >= m_activeCascadeCount)
                continue;

            //----------------------------------------------------------------
            // 描画中のカスケード番号を b0 へ流す。
            // 頂点シェーダーはこれを見て cascadeViewProj[] のどれを使うかを決めるので、
            // シャドウ用シェーダーはカスケードの枚数を知らなくて済む
            //----------------------------------------------------------------
            m_frameConstants->SetShadowCascadeIndex(cascade);
            m_frameConstants->UploadWorld();

            // このスライスをバインドする（カラーRTはnull）
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

                if(m_cullingEnabled && !ShadowCascades::OverlapsCascade(cmd, m_cascades[cascade]))
                    continue;

                m_commandExecutor->ExecuteShadow(cmd, m_staticPipeline.get(), m_skeletalPipeline.get(), stats);
            }
        }

        return true;
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
    //! シャドウマップ（テクスチャ・DSV・SRV・比較サンプラー）を作成します。
    //------------------------------------------------------------------------
    bool ShadowPass::CreateShadowMap(ID3D11Device* device, u32 mapSize) {
        //--------------------------------------------------------------------
        // シャドウマップテクスチャの作成
        // R32_TYPELESS : DSV(D32_FLOAT)とSRV(R32_FLOAT)で共有するため。
        // 解像度を変えて作り直すときにも呼ばれるので、ビューは ReleaseAndGetAddressOf() で前のものを手放して受け取る
        //--------------------------------------------------------------------
        D3D11_TEXTURE2D_DESC texDesc{};
        texDesc.Width            = mapSize;
        texDesc.Height           = mapSize;
        texDesc.MipLevels        = 1;
        texDesc.ArraySize        = kCascadeCount;    // カスケード1枚＝配列の1スライス
        texDesc.Format           = DXGI_FORMAT_R32_TYPELESS;
        texDesc.SampleDesc.Count = 1;
        texDesc.Usage            = D3D11_USAGE_DEFAULT;
        texDesc.BindFlags        = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;

        if(FAILED(device->CreateTexture2D(&texDesc, nullptr, m_mapTex.ReleaseAndGetAddressOf()))) {
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

            if(FAILED(device->CreateDepthStencilView(m_mapTex.Get(), &dsvDesc, m_mapDSV[i].ReleaseAndGetAddressOf()))) {
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

        if(FAILED(device->CreateShaderResourceView(m_mapTex.Get(), &srvDesc, m_mapSRV.ReleaseAndGetAddressOf()))) {
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

        if(FAILED(device->CreateSamplerState(&samplerDesc, m_sampler.ReleaseAndGetAddressOf()))) {
            Tsukino::Core::Log::Error("Failed to create shadow sampler.");
            return false;
        }

        return true;
    }
}    // namespace Tsukino::Renderer
