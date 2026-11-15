//----------------------------------------------------------------------------
//! @file   Views.hpp
//! @brief  bgfx 版のビューの番号（Renderer の内部用）
//! @detail bgfx は番号の小さいビューから順に描きます。描く順に並べています。
//!         パスを bgfx へ移すたびに、間へ足していきます。
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/Renderer/ConstantBuffer.hpp>

#include <bgfx/bgfx.h>

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer::BgfxView {

    // 環境光（IBL）のベイク。描く先（キューブの面とミップ）ごとにビューが1つ要る。焼くフレームでだけ使う
    inline constexpr bgfx::ViewId IblBrdfLut    = 0;     // BRDF の積分表（起動時に1回）
    inline constexpr bgfx::ViewId IblCapture    = 1;     // スカイのキャプチャ（面ごとに1つ。1〜6）
    inline constexpr bgfx::ViewId IblIrradiance = 7;     // irradiance の畳み込み（面ごとに1つ。7〜12）
    inline constexpr bgfx::ViewId IblSpecular   = 13;    // スペキュラのプレフィルタ（ミップ × 6 + 面。13〜48）

    inline constexpr bgfx::ViewId Shadow      = 49;    // シャドウマップ（カスケード1枚ごとに1つ。49〜52）
    inline constexpr bgfx::ViewId GBuffer     = 53;    // 不透明なモデルを G-Buffer へ
    inline constexpr bgfx::ViewId Sky         = 54;    // HDR を背景色で消して、スカイを描く
    inline constexpr bgfx::ViewId Lighting    = 55;    // G-Buffer を照らして HDR へ（物の無い画素は背景のまま残す）
    inline constexpr bgfx::ViewId World       = 56;    // 深度を書くフォワード（デバッグ線など）を HDR へ
    inline constexpr bgfx::ViewId AmbientParticle = 57;    // 環境パーティクルを HDR へ加算（フォグの前。粒子にもフォグが乗る）
    inline constexpr bgfx::ViewId Fog         = 58;    // 深度を読んで、フォグを HDR へ over 合成
    inline constexpr bgfx::ViewId ColorGrade  = 59;    // 球形の色調補正（フォグの上から掛ける）
    inline constexpr bgfx::ViewId Transparent = 60;    // 半透明（TransparentDepth・Transparent）を HDR へ

    inline constexpr bgfx::ViewId MotionBlur  = 61;    // HDR を速度の向きへぼかして、ポストプロセス用の中間バッファへ

    // ブルーム。描く先（縮小段）ごとにビューが1つ要る。縮小段は最大6枚（BloomPass::kMaxMipCount）
    inline constexpr bgfx::ViewId BloomPrefilter  = 62;    // 明部抽出 + 1/2 へ縮小（シーン → 段0）
    inline constexpr bgfx::ViewId BloomDownsample = 63;    // 縮小（段 i-1 → 段 i。63〜67）
    inline constexpr bgfx::ViewId BloomUpsample   = 68;    // 拡大しながら足し戻す（小さい段から順に。68〜72）
    inline constexpr bgfx::ViewId BloomComposite  = 73;    // 段0 をシーンへ加算

    inline constexpr bgfx::ViewId Tonemap     = 74;    // シーン（HDR か中間バッファ）をトーンマップしてバックバッファへ（エフェクトを描くフレームはエフェクトの描く先へ）
    inline constexpr bgfx::ViewId Effects     = 75;    // エフェクト（Effekseer）をトーンマップした絵の上へ。3D の深度で隠れる
    inline constexpr bgfx::ViewId EffectsPresent = 76;    // エフェクトを重ねた絵をバックバッファへ写す
    inline constexpr bgfx::ViewId Overlay     = 77;    // UI と文字（UI の層の 3D モデルが深度を消し直すたびに、1つずつ後ろの番号を使う）

    static_assert(IblSpecular + 6 * 6 <= Shadow, "The IBL bake views (one per cube face and mip) must fit before the shadow views.");
    static_assert(Shadow + kShadowCascadeCount <= GBuffer, "The shadow views (one per cascade) must fit before the G-Buffer view.");

    //! Overlay が使ってよい最後の番号
    inline constexpr bgfx::ViewId OverlayLast = 250;

}    // namespace Tsukino::Renderer::BgfxView
