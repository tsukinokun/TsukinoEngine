//----------------------------------------------------------------------------
//! @file   EffectRenderer.cpp
//! @brief  bgfx 版の Effekseer を描く部品の持ち主の実装
//----------------------------------------------------------------------------
#include "EffectRenderer.hpp"

#include "Effekseer/EfkRenderer.hpp"
#include "FullscreenPass.hpp"

#include <Tsukino/Renderer/Bgfx/GraphicsContext.hpp>
#include <Tsukino/Renderer/Bgfx/ShaderPrograms.hpp>

#include <Tsukino/Core/Log.hpp>

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {

    //------------------------------------------------------------------------
    //! Effekseer の型を持つ中身です。
    //------------------------------------------------------------------------
    struct EffectRenderer::Impl {
        EffekseerBgfx::RendererImplementedRef     renderer;      // Effekseer のレンダラー
        Effekseer::RefPtr<EffekseerBgfx::Texture> background;    // 歪みの背景（借りたテクスチャ）
        Effekseer::RefPtr<EffekseerBgfx::Texture> depth;         // ソフトパーティクルの深度（借りたテクスチャ）
    };

    //------------------------------------------------------------------------
    //! 何も持たずに作ります。
    //------------------------------------------------------------------------
    EffectRenderer::EffectRenderer()
        : m_impl(std::make_unique<Impl>()) {
    }

    //------------------------------------------------------------------------
    //! レンダラーとプログラムを破棄します。
    //! @note  Effekseer のレンダラーは参照の数で消える。使う側（EffectSystem）はシーンと一緒に先に手放している
    //------------------------------------------------------------------------
    EffectRenderer::~EffectRenderer() {
        m_impl.reset();
        if(!GraphicsContext::IsBgfxAlive())
            return;
        if(bgfx::isValid(m_copyProgram))
            bgfx::destroy(m_copyProgram);
        if(bgfx::isValid(m_sourceSampler))
            bgfx::destroy(m_sourceSampler);
    }

    //------------------------------------------------------------------------
    //! Effekseer のレンダラーと、絵を写すプログラムを作ります。
    //------------------------------------------------------------------------
    bool EffectRenderer::Initialize(int32_t squareMaxCount, const FullscreenPass& fullscreenPass) {
        m_fullscreenPass = &fullscreenPass;
        m_copyProgram    = LoadBuiltInBgfxProgram("Tonemap.vs", "Copy.ps");
        m_sourceSampler  = bgfx::createUniform("s_source", bgfx::UniformType::Sampler);
        if(!bgfx::isValid(m_copyProgram))
            return false;

        // エンジンの深度はリバース Z（手前ほど大きい）
        auto renderer = Effekseer::MakeRefPtr<EffekseerBgfx::RendererImplemented>(squareMaxCount);
        if(!renderer->Initialize(true))
            return false;

        m_impl->renderer   = renderer;
        m_impl->background = renderer->GetBgfxGraphicsDevice()->CreateBorrowedTexture(BGFX_INVALID_HANDLE, 1, 1);
        m_impl->depth      = renderer->GetBgfxGraphicsDevice()->CreateBorrowedTexture(BGFX_INVALID_HANDLE, 1, 1);
        return true;
    }

    //------------------------------------------------------------------------
    //! Effekseer のレンダラーを返します。
    //------------------------------------------------------------------------
    ::EffekseerRenderer::Renderer* EffectRenderer::Get() const noexcept {
        return m_impl->renderer.Get();
    }

    //------------------------------------------------------------------------
    //! 描く前の準備をします。
    //------------------------------------------------------------------------
    void EffectRenderer::Prepare(bgfx::ViewId view, const GraphicsContext& graphicsContext, const Tsukino::Core::Math::matrix& projection) {
        EffekseerBgfx::RendererImplemented* renderer = m_impl->renderer.Get();
        if(!renderer)
            return;
        renderer->SetView(view);

        //--------------------------------------------------------------------
        // 歪みの背景（エフェクトを描く前の絵）とソフトパーティクルの深度を写す。
        // bgfx の写し（blit）は、そのビューの描画より先に、前のビューの後で行われる
        //--------------------------------------------------------------------
        const u32 width     = graphicsContext.GetSceneWidth();
        const u32 height    = graphicsContext.GetSceneHeight();
        auto      copyWhole = [view](bgfx::TextureHandle dst, bgfx::TextureHandle src) {
            bgfx::TextureRegion dstRegion;
            bgfx::TextureRegion srcRegion;
            dstRegion.init(dst);
            srcRegion.init(src);
            bgfx::blit(view, dstRegion, srcRegion);
        };
        copyWhole(graphicsContext.GetEffectBackgroundTexture(), graphicsContext.GetEffectColorTexture());
        copyWhole(graphicsContext.GetEffectDepthTexture(), graphicsContext.GetDepthTexture());

        // 描く先は作り直されるとハンドルが変わるので、毎フレーム差し替える
        m_impl->background->SetBorrowedHandle(graphicsContext.GetEffectBackgroundTexture(), width, height);
        m_impl->depth->SetBorrowedHandle(graphicsContext.GetEffectDepthTexture(), width, height);
        renderer->SetBackground(m_impl->background);

        //--------------------------------------------------------------------
        // 深度を距離へ戻すための射影行列の要素（Effekseer の決まりの名前。34 と 43 は行と列が逆の位置）
        //--------------------------------------------------------------------
        float values[4][4];
        hlslpp::store(&values[0][0], projection);

        EffekseerRenderer::DepthReconstructionParameter reconstruction;
        reconstruction.DepthBufferScale   = 1.0f;
        reconstruction.DepthBufferOffset  = 0.0f;
        reconstruction.ProjectionMatrix33 = values[2][2];
        reconstruction.ProjectionMatrix34 = values[3][2];
        reconstruction.ProjectionMatrix43 = values[2][3];
        reconstruction.ProjectionMatrix44 = values[3][3];
        renderer->SetDepth(m_impl->depth, reconstruction);
    }

    //------------------------------------------------------------------------
    //! エフェクトを重ねた絵を、バックバッファへ引き伸ばして写します。
    //------------------------------------------------------------------------
    void EffectRenderer::Present(bgfx::ViewId view, const GraphicsContext& graphicsContext) const {
        if(!m_fullscreenPass || !bgfx::isValid(m_copyProgram))
            return;
        // 内部解像度を下げているときは、ここで画面の大きさへ引き伸ばす（トーンマップと同じく線形で読む）
        bgfx::setTexture(0, m_sourceSampler, graphicsContext.GetEffectColorTexture(), BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP);
        m_fullscreenPass->Submit(view, m_copyProgram, BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A);
    }

}    // namespace Tsukino::Renderer
