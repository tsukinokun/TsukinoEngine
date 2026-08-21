//-------------------------------------------------------------
//! @file   HighlightComponent.hpp
//! @brief  HighlightComponent構造体の宣言
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once
#include <hlsl++.h>
// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {
    //-------------------------------------------------------------
    //! @struct HighlightComponent
    //! @brief  モデルの発光（ネオン風リム + 白発光）をエンティティ単位で上書きするコンポーネント
    //! @note   拾えるアイテムの強調表示に限らず、敵のロックオン表示など汎用的な用途を想定した
    //!         描画用コンポーネント。ModelSystemがCBufferMaterialへ反映する
    //-------------------------------------------------------------
    struct HighlightComponent {
        bool           active       = false;                             // falseの間は一切描画に影響しない
        hlslpp::float3 rimColor     = hlslpp::float3(0.2f, 1.0f, 1.0f);   // ふちの色（ネオンなので1.0超も可）
        float          rimIntensity = 3.0f;                               // ふちの強さ。HDRなので1.0超で白飛び方向に光る
        float          rimPower     = 3.0f;                               // ふちの鋭さ。大きいほど輪郭に集中する
        float          glow         = 0.0f;                               // 全体を白く持ち上げる量（0=なし）
    };
}    // namespace Tsukino::BuiltIn::ECS
