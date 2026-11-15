//----------------------------------------------------------------------------
//! @file   BloomPass.cpp
//! @brief  bgfx 版のブルームパスの実装
//! @detail HDR の絵から閾値を超えた明るさを取り出し、縮小しながらぼかして、拡大しながら足し戻し、
//!         最後に HDR の絵へ加算します。bgfx は描く先ごとにビューが1つ要るので、
//!         明部抽出・縮小の各段・拡大の各段・合成にビューを1つずつ使います（Views.hpp の Bloom〜）。
//----------------------------------------------------------------------------
#include <Tsukino/Renderer/BloomPass.hpp>
#include <Tsukino/Renderer/Bgfx/BloomPassGpu.hpp>
#include <Tsukino/Renderer/Bgfx/GraphicsContext.hpp>
#include <Tsukino/Renderer/Bgfx/Views.hpp>

#include "FullscreenPass.hpp"

#include <Tsukino/Core/Log.hpp>

#include <string>

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {

    namespace {
        //! これより小さい縮小段は作らない（2px を切ると 13 タップが同じテクセルを読むだけになる）
        constexpr u32 kMinMipSize = 2;

        //! 読む側のサンプラー（DX11 版の LinearClamp）
        constexpr u32 kLinearClamp = BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP;

        //! 上書きで描くステート（深度テストなし・ブレンドなし）
        constexpr u64 kOpaqueState = BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A;

        //! 足して描くステート（DX11 版の DirectXTK の Additive と同じ SrcAlpha / One）
        constexpr u64 kAdditiveState = BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A
                                       | BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_SRC_ALPHA, BGFX_STATE_BLEND_ONE);
    }    // namespace

    //------------------------------------------------------------------------
    //! プログラム・ユニフォーム・縮小段を破棄します。
    //------------------------------------------------------------------------
    BloomPass::Gpu::~Gpu() {
        if(!GraphicsContext::IsBgfxAlive())
            return;
        DestroyMips();
        for(bgfx::ProgramHandle program : {prefilterProgram, downsampleProgram, upsampleProgram}) {
            if(bgfx::isValid(program))
                bgfx::destroy(program);
        }
        if(bgfx::isValid(sourceSampler))
            bgfx::destroy(sourceSampler);
    }

    //------------------------------------------------------------------------
    //! 縮小段のテクスチャとフレームバッファを破棄します。
    //------------------------------------------------------------------------
    void BloomPass::Gpu::DestroyMips() {
        for(MipTarget& mip : mips) {
            if(bgfx::isValid(mip.frameBuffer))
                bgfx::destroy(mip.frameBuffer);
            if(bgfx::isValid(mip.texture))
                bgfx::destroy(mip.texture);
            mip = MipTarget{};
        }
    }

    //------------------------------------------------------------------------
    //! 資源を持たない状態で作ります。
    //------------------------------------------------------------------------
    BloomPass::BloomPass()
        : m_gpu(std::make_unique<Gpu>()) {}

    //------------------------------------------------------------------------
    //! 資源を破棄します。
    //------------------------------------------------------------------------
    BloomPass::~BloomPass() = default;

    //------------------------------------------------------------------------
    //! ユニフォームとプログラムを作成します。
    //------------------------------------------------------------------------
    bool BloomPass::Initialize(GraphicsContext& graphicsContext, RenderResources& resources, const FullscreenPass& fullscreenPass,
                               const Tsukino::Asset::ShaderAsset* prefilterPS, const Tsukino::Asset::ShaderAsset* downsamplePS,
                               const Tsukino::Asset::ShaderAsset* upsamplePS) {
        m_graphicsContext = &graphicsContext;
        m_resources       = &resources;
        m_fullscreenPass  = &fullscreenPass;

        if(!m_gpu->params.Create("u_bloom")) {
            Tsukino::Core::Log::Error("Failed to create bloom uniform.");
            return false;
        }
        m_gpu->sourceSampler = bgfx::createUniform("s_bloomSource", bgfx::UniformType::Sampler);

        // 頂点シェーダーはフルスクリーン三角形用を共用する
        m_gpu->prefilterProgram  = fullscreenPass.CreateProgram(prefilterPS);
        m_gpu->downsampleProgram = fullscreenPass.CreateProgram(downsamplePS);
        m_gpu->upsampleProgram   = fullscreenPass.CreateProgram(upsamplePS);
        if(!bgfx::isValid(m_gpu->prefilterProgram) || !bgfx::isValid(m_gpu->downsampleProgram) || !bgfx::isValid(m_gpu->upsampleProgram)) {
            Tsukino::Core::Log::Error("BloomPass: The bloom shaders are not bgfx shaders; bloom is not drawn.");
            return false;
        }
        return true;
    }

    //------------------------------------------------------------------------
    //! 縮小段のテクスチャを画面サイズに合わせて作り直します。
    //------------------------------------------------------------------------
    bool BloomPass::EnsureMipTargets(u32 width, u32 height) {
        if(width == m_sourceWidth && height == m_sourceHeight && m_builtMipLimit == m_mipLimit && m_mipCount > 0)
            return true;

        m_gpu->DestroyMips();
        m_mipCount      = 0;
        m_sourceWidth   = width;
        m_sourceHeight  = height;
        m_builtMipLimit = m_mipLimit;

        u32 mipWidth  = width / 2;
        u32 mipHeight = height / 2;
        for(u32 i = 0; i < m_mipLimit; ++i) {
            if(mipWidth < kMinMipSize || mipHeight < kMinMipSize)
                break;

            //--------------------------------------------------------------
            // HDR と同じ 16bit 浮動小数。閾値を超えた明るさ（1.0 超）を
            // そのまま持ち運ぶので、8bit では足りない
            //--------------------------------------------------------------
            Gpu::MipTarget& mip = m_gpu->mips[i];
            mip.texture         = bgfx::createTexture2D(static_cast<u16>(mipWidth), static_cast<u16>(mipHeight), false, 1,
                                                        bgfx::TextureFormat::RGBA16F, BGFX_TEXTURE_RT | kLinearClamp);
            if(bgfx::isValid(mip.texture))
                mip.frameBuffer = bgfx::createFrameBuffer(1, &mip.texture, false);
            if(!bgfx::isValid(mip.texture) || !bgfx::isValid(mip.frameBuffer)) {
                Tsukino::Core::Log::Error("BloomPass: Failed to create bloom mip " + std::to_string(i) + ".");
                if(bgfx::isValid(mip.texture))
                    bgfx::destroy(mip.texture);
                mip = Gpu::MipTarget{};
                break;
            }
            mip.width  = static_cast<u16>(mipWidth);
            mip.height = static_cast<u16>(mipHeight);
            ++m_mipCount;

            mipWidth /= 2;
            mipHeight /= 2;
        }

        return m_mipCount > 0;
    }

    //------------------------------------------------------------------------
    //! ブルームを掛けて、トーンマップの入力になるバッファへ加算します。
    //------------------------------------------------------------------------
    void BloomPass::Execute(bool sourceIsPostProcess) {
        if(!m_enabled || m_mipLimit == 0 || !m_fullscreenPass || !m_fullscreenPass->IsValid() || !bgfx::isValid(m_gpu->prefilterProgram)
           || !bgfx::isValid(m_gpu->downsampleProgram) || !bgfx::isValid(m_gpu->upsampleProgram))
            return;

        const u32 width  = m_graphicsContext->GetSceneWidth();
        const u32 height = m_graphicsContext->GetSceneHeight();
        if(!EnsureMipTargets(width, height))
            return;

        // 定数を送ってフルスクリーン三角形を1枚描く。読む絵の大きさから1テクセルの幅を決める
        const auto draw = [this](bgfx::ViewId view, bgfx::FrameBufferHandle target, u32 targetWidth, u32 targetHeight, bgfx::TextureHandle source,
                                 u32 sourceWidth, u32 sourceHeight, bgfx::ProgramHandle program, u64 state, CBufferBloom data) {
            data.texelSizeX = 1.0f / static_cast<float>(sourceWidth);
            data.texelSizeY = 1.0f / static_cast<float>(sourceHeight);

            bgfx::setViewFrameBuffer(view, target);
            bgfx::setViewRect(view, 0, 0, static_cast<u16>(targetWidth), static_cast<u16>(targetHeight));

            m_gpu->params.Set(data);
            bgfx::setTexture(0, m_gpu->sourceSampler, source, kLinearClamp);
            m_fullscreenPass->Submit(view, program, state);
        };

        const auto& mips = m_gpu->mips;

        CBufferBloom data{};
        data.threshold = m_params.threshold;
        data.softKnee  = m_params.softKnee;
        data.radius    = m_params.radius;

        // モーションブラーが中間バッファへ書いた後なら、読むのも足し戻すのも中間バッファ。そうでなければ HDR バッファ
        const bgfx::TextureHandle     scene       = sourceIsPostProcess ? m_graphicsContext->GetPostProcessTexture() : m_graphicsContext->GetHdrTexture();
        const bgfx::FrameBufferHandle sceneTarget = sourceIsPostProcess ? m_graphicsContext->GetPostProcessFrameBuffer()
                                                                        : m_graphicsContext->GetHdrColorFrameBuffer();

        //----------------------------------------------------------
        // 1) 明部抽出 + 1/2 へ縮小（シーン → mip0）
        //----------------------------------------------------------
        draw(BgfxView::BloomPrefilter, mips[0].frameBuffer, mips[0].width, mips[0].height, scene, width, height, m_gpu->prefilterProgram,
             kOpaqueState, data);

        //----------------------------------------------------------
        // 2) 縮小（mip[i-1] → mip[i]）
        //----------------------------------------------------------
        for(u32 i = 1; i < m_mipCount; ++i) {
            const bgfx::ViewId view = static_cast<bgfx::ViewId>(BgfxView::BloomDownsample + (i - 1));
            draw(view, mips[i].frameBuffer, mips[i].width, mips[i].height, mips[i - 1].texture, mips[i - 1].width, mips[i - 1].height,
                 m_gpu->downsampleProgram, kOpaqueState, data);
        }

        //----------------------------------------------------------
        // 3) 拡大しながら足し戻す（mip[i] → mip[i-1] へ加算）
        //    小さい段ほど広く滲んでいるので、全段を足すと「芯は強く、裾は広い」光になる。
        //    ビューは描く順（小さい段から）に並べる
        //----------------------------------------------------------
        for(u32 i = m_mipCount - 1; i > 0; --i) {
            const bgfx::ViewId view = static_cast<bgfx::ViewId>(BgfxView::BloomUpsample + (m_mipCount - 1 - i));
            draw(view, mips[i - 1].frameBuffer, mips[i - 1].width, mips[i - 1].height, mips[i].texture, mips[i].width, mips[i].height,
                 m_gpu->upsampleProgram, kAdditiveState, data);
        }

        //----------------------------------------------------------
        // 4) 元の絵へ加算（mip0 → シーン）
        //    書き込み先は読み出し元と同じバッファ。ここでは mip0 しか読まない
        //----------------------------------------------------------
        data.intensity = m_params.intensity;
        data.tintR     = m_params.tintR;
        data.tintG     = m_params.tintG;
        data.tintB     = m_params.tintB;
        draw(BgfxView::BloomComposite, sceneTarget, width, height, mips[0].texture, mips[0].width, mips[0].height,
             m_gpu->upsampleProgram, kAdditiveState, data);
    }

}    // namespace Tsukino::Renderer
