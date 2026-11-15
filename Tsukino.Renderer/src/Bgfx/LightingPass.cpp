//----------------------------------------------------------------------------
//! @file   LightingPass.cpp
//! @brief  bgfx 版のディファードライティングパスの実装
//! @detail G-Buffer を読み、太陽（影つき）・点光源・スポットライト・環境光で照らして HDR バッファへ書きます。
//----------------------------------------------------------------------------
#include <Tsukino/Renderer/LightingPass.hpp>
#include <Tsukino/Renderer/Bgfx/FrameConstantsGpu.hpp>
#include <Tsukino/Renderer/Bgfx/GraphicsContext.hpp>
#include <Tsukino/Renderer/Bgfx/IBLBakerGpu.hpp>
#include <Tsukino/Renderer/Bgfx/LightingPassGpu.hpp>
#include <Tsukino/Renderer/Bgfx/TextureStages.hpp>
#include <Tsukino/Renderer/Bgfx/Views.hpp>

#include "FullscreenPass.hpp"
#include "ShadowPass.hpp"

#include <Tsukino/Core/Log.hpp>

#include <algorithm>
#include <cstring>
#include <string>

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {

    //------------------------------------------------------------------------
    //! プログラムとサンプラーを破棄します。
    //------------------------------------------------------------------------
    LightingPass::Gpu::~Gpu() {
        if(!GraphicsContext::IsBgfxAlive())
            return;
        if(bgfx::isValid(program))
            bgfx::destroy(program);
        for(bgfx::UniformHandle sampler : gbufferSamplers) {
            if(bgfx::isValid(sampler))
                bgfx::destroy(sampler);
        }
    }

    //------------------------------------------------------------------------
    //! 資源を持たない状態で作ります。
    //------------------------------------------------------------------------
    LightingPass::LightingPass()
        : m_gpu(std::make_unique<Gpu>()) {}

    //------------------------------------------------------------------------
    //! 資源を破棄します。
    //------------------------------------------------------------------------
    LightingPass::~LightingPass() = default;

    //------------------------------------------------------------------------
    //! プログラムとユニフォームを作成します。
    //------------------------------------------------------------------------
    bool LightingPass::Initialize(GraphicsContext&                   graphicsContext,
                                  RenderResources&                   resources,
                                  FrameConstants&                    frameConstants,
                                  const FullscreenPass&              fullscreenPass,
                                  const ShadowPass&                  shadowPass,
                                  IBLBaker&                          iblBaker,
                                  const Tsukino::Asset::ShaderAsset* ps) {
        m_graphicsContext = &graphicsContext;
        m_resources       = &resources;
        m_frameConstants  = &frameConstants;
        m_fullscreenPass  = &fullscreenPass;
        m_shadowPass      = &shadowPass;
        m_iblBaker        = &iblBaker;

        m_gpu->program = fullscreenPass.CreateProgram(ps);
        if(!bgfx::isValid(m_gpu->program)) {
            Tsukino::Core::Log::Error("LightingPass: The lighting shader is not a bgfx shader; opaque models are not lit.");
            return false;
        }
        if(!m_gpu->lights.Create("u_lights")) {
            Tsukino::Core::Log::Error("LightingPass: Failed to create the lights uniform.");
            return false;
        }

        constexpr const char* kSamplerNames[GraphicsContext::kGBufferCount] = {
            "s_gbufferAlbedo", "s_gbufferNormal", "s_gbufferMaterial", "s_gbufferEmissive", "s_gbufferDepth", "s_gbufferWorldPos",
        };
        for(u32 i = 0; i < GraphicsContext::kGBufferCount; ++i)
            m_gpu->gbufferSamplers[i] = bgfx::createUniform(kSamplerNames[i], bgfx::UniformType::Sampler);
        return true;
    }

    //------------------------------------------------------------------------
    //! 太陽の光を設定します（向きと色はシーン定数へ入る）。
    //------------------------------------------------------------------------
    void LightingPass::SetDirectionalLight(const hlslpp::float3& direction, const hlslpp::float3& color, float intensity) {
        if(!m_frameConstants)
            return;
        const hlslpp::float3 normalizedDir = hlslpp::normalize(direction);
        m_frameConstants->SetDirectionalLight(hlslpp::float4(normalizedDir.x, normalizedDir.y, normalizedDir.z, 0.0f),
                                              hlslpp::float4(color.x, color.y, color.z, intensity));
    }

    //------------------------------------------------------------------------
    //! 点光源・スポットライトを設定します。
    //------------------------------------------------------------------------
    void LightingPass::SetLights(const GPULight* lights, u32 count) {
        const u32 copyCount = std::min(count, MAX_LIGHTS);
        if(count > MAX_LIGHTS && !m_lightOverflowWarned) {
            Tsukino::Core::Log::Error("LightingPass::SetLights - light count (" + std::to_string(count) + ") exceeds MAX_LIGHTS ("
                                      + std::to_string(MAX_LIGHTS) + "). Extra lights are dropped.");
            m_lightOverflowWarned = true;
        }

        m_lightsData.lightCount = copyCount;
        if(copyCount > 0)
            std::memcpy(m_lightsData.lights, lights, sizeof(GPULight) * copyCount);
    }

    //------------------------------------------------------------------------
    //! G-Buffer を照らして HDR バッファへ書きます。
    //------------------------------------------------------------------------
    void LightingPass::Execute() {
        if(!m_fullscreenPass || !bgfx::isValid(m_gpu->program))
            return;

        // G-Buffer は画面と同じ大きさなので点で読む
        constexpr u32 kPointClamp = BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP | BGFX_SAMPLER_MIN_POINT | BGFX_SAMPLER_MAG_POINT
                                    | BGFX_SAMPLER_MIP_POINT;
        const u8 stages[GraphicsContext::kGBufferCount] = {
            BgfxStage::GBufferAlbedo, BgfxStage::GBufferNormal, BgfxStage::GBufferMaterial,
            BgfxStage::GBufferEmissive, BgfxStage::GBufferDepth, BgfxStage::GBufferWorldPos,
        };
        const bgfx::TextureHandle textures[GraphicsContext::kGBufferCount] = {
            m_graphicsContext->GetGBufferTexture(0), m_graphicsContext->GetGBufferTexture(1), m_graphicsContext->GetGBufferTexture(2),
            m_graphicsContext->GetGBufferTexture(3), m_graphicsContext->GetDepthTexture(),     m_graphicsContext->GetGBufferTexture(4),
        };
        for(u32 i = 0; i < GraphicsContext::kGBufferCount; ++i)
            bgfx::setTexture(stages[i], m_gpu->gbufferSamplers[i], textures[i], kPointClamp);

        m_frameConstants->GetGpu().Apply();
        m_gpu->lights.Set(m_lightsData);
        m_iblBaker->GetGpu().Bind();
        m_shadowPass->BindForSampling();

        // 深度テストなし・ブレンドなし（何も描かれていない画素は discard で背景色のまま残す）
        m_fullscreenPass->Submit(BgfxView::Lighting, m_gpu->program, BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A);
    }

}    // namespace Tsukino::Renderer
