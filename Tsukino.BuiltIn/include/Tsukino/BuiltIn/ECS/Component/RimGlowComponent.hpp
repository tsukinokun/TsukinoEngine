//--------------------------------------------------------------
//! @file   RimGlowComponent.hpp
//! @brief  エンティティ単位の発光上乗せコンポーネント
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
#include <hlsl++.h>
// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {
    //--------------------------------------------------------------
    //! エンティティ単位で発光を上乗せする設定
    //! @note ModelSystemがCBufferMaterialのrimColor / rimParamsへ書き、
    //!       シェーダーがマテリアル本来のエミッシブへ「加算」する。
    //!       上書きではないので、モデルが元々持つエミッシブは残る。
    //!       加算するのは不透明のGBuffer.ps.hlslと半透明のModel.ps.hlslの
    //!       両方で、どちらもMaterial.hlsliのEvaluateEmissiveBoost()を呼ぶ。
    //!       ModelComponent::opacityでフェード中のエンティティも同じように光る。
    //!
    //!       持っている効果は2つ。輪郭が光るリムと、面全体を一律に持ち上げるglow。
    //!       リムはフレネル反射の近似ではない。光源方向を見ず、視線と法線の角度
    //!       だけで決めているので、光源が無くても光る。物理的な正しさではなく、
    //!       プレイヤーに気づかせたいものを目立たせるための演出として使う。
    //!
    //!       色と強さの既定値は無色・ゼロにしてある。何色でどう光らせるかは
    //!       ゲーム側の演出の判断であって、エンジンが決めることではないため。
    //!       activeを立てただけでは何も起きず、指定したぶんだけ光る
    //--------------------------------------------------------------
    struct RimGlowComponent {
        bool           active       = false;                              // falseの間は一切描画に影響しない
        hlslpp::float3 rimColor     = hlslpp::float3(1.0f, 1.0f, 1.0f);   // ふちの色（HDRなので1.0超も可）
        float          rimIntensity = 0.0f;                               // ふちの強さ。1.0超で白飛び方向に光る
        float          rimPower     = 3.0f;                               // ふちの鋭さ。大きいほど輪郭に集中する
        float          glow         = 0.0f;                               // 面全体を一律に持ち上げる量（0=なし）
    };
}    // namespace Tsukino::BuiltIn::ECS
