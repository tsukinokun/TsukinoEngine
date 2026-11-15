//----------------------------------------------------------------------------
//! @file   EffectRenderer.hpp
//! @brief  bgfx 版の Effekseer を描く部品の持ち主（Renderer の内部用）
//! @detail Effekseer のレンダラー（src/Bgfx/Effekseer）を作って持ち、毎フレーム描く前に、
//!         描く先のビュー・歪みの背景・ソフトパーティクルの深度を渡します。
//!         描き終わった絵をバックバッファへ写すのもここです。
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/Core/Math/Matrix.hpp>
#include <Tsukino/Core/typedef.hpp>

#include <bgfx/bgfx.h>

#include <memory>

// 前方宣言
namespace EffekseerRenderer {
    class Renderer;
}

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {
    class FullscreenPass;     // 前方宣言
    class GraphicsContext;    // 前方宣言

    //------------------------------------------------------------------------
    //! Effekseer を描く部品の持ち主です（bgfx 版）。
    //------------------------------------------------------------------------
    class EffectRenderer {
    public:
        //! 何も持たずに作ります。
        EffectRenderer();

        //! レンダラーとプログラムを破棄します。
        ~EffectRenderer();

        EffectRenderer(const EffectRenderer&)            = delete;
        EffectRenderer& operator=(const EffectRenderer&) = delete;

        //! Effekseer のレンダラーと、絵を写すプログラムを作ります。
        //! @param  [in] squareMaxCount 1回にまとめて描ける四角の数
        //! @param  [in] fullscreenPass フルスクリーン三角形の描画（借りる）
        //! @return 作れたら true
        [[nodiscard]]
        bool Initialize(int32_t squareMaxCount, const FullscreenPass& fullscreenPass);

        //! Effekseer のレンダラーを返します。
        //! @return レンダラー（作れていなければ nullptr）
        [[nodiscard]]
        ::EffekseerRenderer::Renderer* Get() const noexcept;

        //! 描く前の準備をします。描く先のビューを決め、歪みの背景と深度を写して Effekseer へ渡します。
        //! @param  [in] view            エフェクトを描くビュー（描く先はエフェクトを描く先のフレームバッファ）
        //! @param  [in] graphicsContext 描く先を持つコンテキスト
        //! @param  [in] projection      射影行列（ソフトパーティクルで深度を距離へ戻すのに使う）
        void Prepare(bgfx::ViewId view, const GraphicsContext& graphicsContext, const Tsukino::Core::Math::matrix& projection);

        //! エフェクトを重ねた絵を、バックバッファへ引き伸ばして写します。
        //! @param  [in] view            写すビュー（描く先はバックバッファ）
        //! @param  [in] graphicsContext 絵を持つコンテキスト
        void Present(bgfx::ViewId view, const GraphicsContext& graphicsContext) const;

    private:
        struct Impl;                      //!< Effekseer の型を持つ中身（EffectRenderer.cpp で定義）
        std::unique_ptr<Impl> m_impl;     // 中身

        const FullscreenPass* m_fullscreenPass = nullptr;                // フルスクリーン三角形の描画（借りている）
        bgfx::ProgramHandle   m_copyProgram    = BGFX_INVALID_HANDLE;    // 絵を写すプログラム（Tonemap.vs + Copy.ps）
        bgfx::UniformHandle   m_sourceSampler  = BGFX_INVALID_HANDLE;    // 写す絵のサンプラー
    };

}    // namespace Tsukino::Renderer
