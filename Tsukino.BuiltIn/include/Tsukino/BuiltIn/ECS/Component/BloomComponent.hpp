//--------------------------------------------------------------
//! @file   BloomComponent.hpp
//! @brief  ブルームコンポーネント
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
#include <hlsl++.h>
// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {
    //--------------------------------------------------------------
    //! @struct BloomComponent
    //! @brief  ブルーム（明るい所の光が周りへ滲む）の設定
    //! @note   カメラエンティティにアタッチして使う。
    //!         このコンポーネントが存在しなければブルームは無効
    //!         （MotionBlurComponent と同じく「付ける／付けない」で ON/OFF を表す）。
    //!
    //!         滲むのは HDR で threshold を超えた画素だけ。普通に照らされた物は
    //!         1.0 未満に収まるので、光らせたい物（RimGlowComponent・エミッシブ）を
    //!         1.0 超えで光らせると、それだけが滲む。
    //!         Effekseer のエフェクトと UI はトーンマップ後に描かれるので滲まない
    //--------------------------------------------------------------
    struct BloomComponent {
        bool           enabled   = true;                                // false ならブルームパスをスキップする
        float          threshold = 1.0f;                                // これを超えた明るさだけが滲む（HDR。1.0 = 白）
        float          softKnee  = 0.5f;                                // 閾値付近をなだらかにする幅（0〜1。threshold に対する割合）
        float          intensity = 0.6f;                                // 滲みを元の絵へ足す強さ
        float          radius    = 1.0f;                                // 滲みの広がり（拡大時のぼかし半径の倍率）
        hlslpp::float3 tint      = hlslpp::float3(1.0f, 1.0f, 1.0f);    // 滲みの色味
    };
}    // namespace Tsukino::BuiltIn::ECS
