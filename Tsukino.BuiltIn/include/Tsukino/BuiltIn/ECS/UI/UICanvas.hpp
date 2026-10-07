//--------------------------------------------------------------
//! @file   UICanvas.hpp
//! @brief  画面UIの座標と、画面（バックバッファ）のピクセルの対応
//! @detail 画面スプライト・文字・UI の層のモデル・切り取り枠・マウスの当たり判定は、すべて「UI の座標」で置きます。
//!         UI のカメラ（正射影の CameraComponent）に referenceResolution を指定すると、その解像度で作った UI を
//!         縦横比を保ったままウィンドウに収まる大きさに拡大し、真ん中に寄せます（Unity の CanvasScaler の
//!         Scale With Screen Size に当たる）。指定が無ければ UI の座標 ＝ ピクセルです。
//!         対応は CameraSystem が毎フレーム Registry のコンテキストに書き、UI を扱うシステムはここを通して変換します。
//--------------------------------------------------------------
#pragma once
#include <Tsukino/BuiltIn/ECS/UI/UIClipUtility.hpp>

#include <Tsukino/Core/ECS/Registry/Registry.hpp>

#include <hlsl++.h>

#include <algorithm>

// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {

    //--------------------------------------------------------------
    //! @struct UICanvas
    //! @brief  UI の座標と画面のピクセルの対応（pixel = ui * scale + offset）
    //--------------------------------------------------------------
    struct UICanvas {
        float          scale  = 1.0f;                            // UI の 1 単位が何ピクセルか
        hlslpp::float2 offset = hlslpp::float2(0.0f, 0.0f);      // UI の原点 (0, 0) が来る画面のピクセル（真ん中に寄せたときの余白）
        hlslpp::float2 size   = hlslpp::float2(0.0f, 0.0f);      // UI の基準の大きさ（UI の座標。基準が無ければ画面のピクセル数）

        //! @brief  UI の座標を画面のピクセルにする
        [[nodiscard]] hlslpp::float2 ToPixel(const hlslpp::float2& ui) const noexcept { return ui * scale + offset; }

        //! @brief  画面のピクセルを UI の座標にする（マウスの位置など）
        [[nodiscard]] hlslpp::float2 ToUI(const hlslpp::float2& pixel) const noexcept { return (pixel - offset) / scale; }

        //! @brief  UI の座標の切り取り枠を、画面のピクセルの枠にする（シザー矩形を作るとき）
        [[nodiscard]] UIClipUtility::ClipBounds ToPixel(const UIClipUtility::ClipBounds& ui) const noexcept {
            UIClipUtility::ClipBounds pixel;
            pixel.left   = ui.left * scale + float(offset.x);
            pixel.top    = ui.top * scale + float(offset.y);
            pixel.right  = ui.right * scale + float(offset.x);
            pixel.bottom = ui.bottom * scale + float(offset.y);
            return pixel;
        }

        //--------------------------------------------------------------
        //! @brief  基準の解像度で作った UI を、縦横比を保って画面に収め、真ん中に寄せる対応を作る
        //! @param  reference [in] 基準の解像度（UI の座標の幅と高さ）。どちらかが 0 以下なら UI の座標 ＝ ピクセル
        //! @param  screen    [in] 画面の大きさ（ピクセル）
        //! @return 対応
        //--------------------------------------------------------------
        [[nodiscard]] static UICanvas Fit(const hlslpp::float2& reference, const hlslpp::float2& screen) noexcept {
            UICanvas canvas;
            if(float(reference.x) <= 0.0f || float(reference.y) <= 0.0f || float(screen.x) <= 0.0f || float(screen.y) <= 0.0f) {
                canvas.size = screen;
                return canvas;
            }
            canvas.scale  = std::min(float(screen.x / reference.x), float(screen.y / reference.y));
            canvas.offset = (screen - reference * canvas.scale) * 0.5f;
            canvas.size   = reference;
            return canvas;
        }
    };

    //--------------------------------------------------------------
    //! @brief  レジストリに置いた UI の対応を返す
    //! @param  registry [in] ECS レジストリ
    //! @return 対応（コンテキストに無ければ UI の座標 ＝ ピクセル）
    //--------------------------------------------------------------
    [[nodiscard]] inline const UICanvas& GetUICanvas(Tsukino::ECS::Registry& registry) {
        static const UICanvas kIdentity;
        return registry.HasContext<UICanvas>() ? registry.GetContext<UICanvas>() : kIdentity;
    }
}    // namespace Tsukino::BuiltIn::ECS
