//----------------------------------------------------------------------------
//! @file   UIMotionComponent.hpp
//! @brief  画面UIの見た目だけのずれ（ずらす・中心を決めて拡大・回す）のコンポーネント
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/Core/ECS/EntityRef/EntityRef.hpp>

#include <hlsl++.h>

// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {

    //! 画面UIの見た目だけのずれです。
    //! @note   TransformSystem が、TransformComponent から作ったローカル行列のあとに
    //!         「pivot を中心に scale・rotation をかけ、offset だけずらす」行列を重ねて worldMatrix を作る。
    //!         TransformComponent の値は書き換えないので、位置や大きさを毎フレーム書くシステムとぶつからない。
    //!         - 子にもそのまま効く（子の worldMatrix は親の worldMatrix から作られるため）
    //!         - 文字の大きさは worldMatrix のスケールから決まるので、文字も一緒に拡大する
    //!         - マウスの当たり判定（InteractionSystem）も worldMatrix を使うので、見た目と一致する
    //!         - offset と pivot は親の空間の座標（親が無い画面UIなら画面ピクセル）
    //!         - source を指定すると、UITweenSystem がそのエンティティの動き（と UICanvasGroupComponent の透明度）をこちらへ写す
    //!           （ボタンとは別エンティティのラベルや、パネルの部品をまとめて動かす。同じ親の空間にあること）。
    //!           自分も UITweenComponent か UIButtonFeedbackComponent を持つなら、自分の動きのあとに写し元の動きを重ねる
    //!           （パネルの上のボタン）。写しの連鎖は2段まで（パネル → ボタン → ラベル）
    //!         - UITweenComponent か UIButtonFeedbackComponent を付けたエンティティと、source を指定したエンティティの値は、
    //!           UITweenSystem が毎フレーム作り直す（手で書いた値は残らない）
    struct UIMotionComponent {
        hlslpp::float2          offset      = hlslpp::float2(0.0f, 0.0f);    // ずらす量（親の空間の座標）
        hlslpp::float2          scale       = hlslpp::float2(1.0f, 1.0f);    // 拡大率（pivot を中心にかける）
        float                   rotation    = 0.0f;                          // 回転（ラジアン。画面の奥を軸に、pivot を中心に回す）
        hlslpp::float2          pivot       = hlslpp::float2(0.0f, 0.0f);    // 拡大・回転の中心（親の空間の座標。pivotIsSelf が false のとき使う）
        bool                    pivotIsSelf = true;                          // true なら自分の位置（TransformComponent::position）を中心にする
        Tsukino::ECS::EntityRef source;                                      // 動きを写す元のエンティティ（無ければ entt::null）
    };
}    // namespace Tsukino::BuiltIn::ECS
