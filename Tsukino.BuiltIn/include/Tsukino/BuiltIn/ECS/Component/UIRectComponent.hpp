//-------------------------------------------------------------
//! @file   UIRectComponent.hpp
//! @brief  UIRectComponentクラスの宣言
//-------------------------------------------------------------
#pragma once
#include <hlsl++.h>

// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {
    //-------------------------------------------------------------
    //! @struct UIRectComponent
    //! @brief  画面UIの部品が占める四角（幅・高さと基準点）
    //! @note   部品の大きさを Transform の scale と分けて持つ。scale は 3D と同じく子へ受け継がれる
    //!         「拡大率」（押したときの縮み・ポップなどの動き）にだけ使い、大きさはここで決める。
    //!         大きさは子へ受け継がれないので、板やボタンの子にアイコン・文字をそのまま置ける。
    //!
    //!         四角はこのエンティティのローカルの座標で、Transform の位置が四角の pivot の点になる。
    //!         ワールド行列（親まで合成した拡大・回転・移動）で写したものが画面上の四角になる。
    //!
    //!         同じエンティティの次のものがこの四角を使う（無ければ今まで通りの大きさ）。
    //!         - SpriteComponent（画面空間）: 画像を四角いっぱいに描く（9 スライスも四角の大きさで分ける）。
    //!           無ければ画像のピクセルの大きさ
    //!         - マウスの当たり判定（PointerTargetComponent）: 四角の中で当たる（回転も考慮する）。
    //!           無ければ画像のピクセルの大きさ
    //!         - FontComponent: horizontalAlign・verticalAlign を四角の中の揃えにし、maxWidth が 0 なら四角の幅に収める。
    //!           無ければ Transform の位置が揃えの基準点
    //!         - UIClipComponent: size が 0 なら、この四角を切り取り枠にする
    //-------------------------------------------------------------
    struct UIRectComponent {
        hlslpp::float2 size  = hlslpp::float2(0.0f, 0.0f);    // 幅・高さ（UI の座標）
        hlslpp::float2 pivot = hlslpp::float2(0.5f, 0.5f);    // Transform の位置が四角のどこか（0,0 で左上、1,1 で右下）
    };
}    // namespace Tsukino::BuiltIn::ECS
