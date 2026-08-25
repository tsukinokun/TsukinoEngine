//-------------------------------------------------------------
//! @file   FontComponent.hpp
//! @brief  FontComponentクラスの宣言
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once
#include <Tsukino/Engine/Asset/AssetHandle.hpp>

#include <hlsl++.h>
#include <cstdint>
#include <string>
// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {
    //-------------------------------------------------------------
    //! @enum   HorizontalAlign
    //! @brief  テキストの水平方向の基準位置
    //-------------------------------------------------------------
    enum class HorizontalAlign : std::uint8_t {
        Left = 0,    // 描画位置が文字列の左端
        Center,      // 描画位置が文字列の中央
        Right,       // 描画位置が文字列の右端
    };

    //-------------------------------------------------------------
    //! @enum   VerticalAlign
    //! @brief  テキストの垂直方向の基準位置
    //-------------------------------------------------------------
    enum class VerticalAlign : std::uint8_t {
        Top = 0,    // 描画位置が文字列の上端
        Middle,     // 描画位置が文字列の中央
        Bottom,     // 描画位置が文字列の下端
    };

    //-------------------------------------------------------------
    //! @struct FontComponent
    //! @brief  フォント描画に必要な情報を管理するコンポ
    //! @note   文字サイズはこのコンポでは持たず、TransformComponent::scale.xが
    //!         そのままフォントの拡大率になる（FontRendererSystemがworldMatrixから読む）
    //-------------------------------------------------------------
    struct FontComponent {
        std::wstring                text;                     // 描画するテキスト
        Tsukino::Asset::AssetHandle fontHandle;               // フォントアセットのハンドル
        hlslpp::float4              color  = {1, 1, 1, 1};    // フォントの色
        hlslpp::float2              origin = {0, 0};          // 回転の中心点などの微調整用

        // 揃え位置。既定値(Left/Top)は従来どおり「描画位置＝文字列の左上」になる。
        // 実際の反映はFontRendererSystemが文字列を計測してoriginへ加算する形で行う
        HorizontalAlign horizontalAlign = HorizontalAlign::Left;    // 水平方向の基準位置
        VerticalAlign   verticalAlign   = VerticalAlign::Top;       // 垂直方向の基準位置

        // 縁取り。明るい背景の上でも文字が読めるようにするためのもの。
        // outlineWidthが0より大きいときだけ、本体の下に8方向へずらした文字が描かれる
        hlslpp::float4 outlineColor = {0, 0, 0, 1};    // 縁取りの色
        float          outlineWidth = 0.0f;            // 縁取りの太さ（ピクセル単位。0で無効）

        // 描画順（小さいほど先に描かれる）。RenderPass::Overlayへ積まれる文字は
        // SpriteComponent::sortOrderと同じ1本の軸として比較されるため、
        // 画面固定UIの重なりはスプライトと文字の種類を問わずここの値だけで決まる
        int sortOrder = 0;
    };
}    // namespace Tsukino::BuiltIn::ECS
