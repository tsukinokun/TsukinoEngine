//----------------------------------------------------------------------------
//! @file   Views.hpp
//! @brief  bgfx 版のビューの番号（Renderer の内部用）
//! @detail bgfx は番号の小さいビューから順に描きます。DX11 版の Render() のパスの順に並べています。
//!         パスを bgfx へ移すたびに、間へ足していきます。
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/Renderer/ConstantBuffer.hpp>

#include <bgfx/bgfx.h>

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer::BgfxView {

    inline constexpr bgfx::ViewId Shadow      = 0;    // シャドウマップ（カスケード1枚ごとに1つ。0〜3 を使う）
    inline constexpr bgfx::ViewId GBuffer     = 4;    // 不透明なモデルを G-Buffer へ
    inline constexpr bgfx::ViewId Lighting    = 5;    // G-Buffer を照らして HDR へ（背景は背景色）
    inline constexpr bgfx::ViewId World       = 6;    // 深度を書くフォワード（デバッグ線など）を HDR へ
    inline constexpr bgfx::ViewId Transparent = 7;    // 半透明（TransparentDepth・Transparent）を HDR へ
    inline constexpr bgfx::ViewId Tonemap     = 8;    // HDR をトーンマップしてバックバッファへ
    inline constexpr bgfx::ViewId Overlay     = 9;    // UI と文字（UI の層の 3D モデルが深度を消し直すたびに、1つずつ後ろの番号を使う）

    static_assert(Shadow + kShadowCascadeCount <= GBuffer, "The shadow views (one per cascade) must fit before the G-Buffer view.");

    //! Overlay が使ってよい最後の番号
    inline constexpr bgfx::ViewId OverlayLast = 250;

}    // namespace Tsukino::Renderer::BgfxView
