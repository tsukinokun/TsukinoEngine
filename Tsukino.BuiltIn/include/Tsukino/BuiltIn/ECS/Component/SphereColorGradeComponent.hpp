//--------------------------------------------------------------
//! @file   SphereColorGradeComponent.hpp
//! @brief  球形の色調補正コンポーネント
//--------------------------------------------------------------
#pragma once
#include <hlsl++.h>
// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {
    //--------------------------------------------------------------
    //! @struct SphereColorGradeComponent
    //! @brief  ワールド空間の球の内側だけ、画面の色を補正する設定
    //! @note   Unreal の Post Process Volume＋SphereMask、Unity の Local Volume に当たる。
    //!         半径を毎フレーム書き換えれば「ある地点から世界が塗り替わっていく」演出になる。
    //!         不透明物と空に掛かり、半透明物・エフェクト・UI には掛からない（フォグパスの直後に描くため）。
    //!         シーンに複数あっても最初の1つだけが使われる（FogComponent と同じ）
    //--------------------------------------------------------------
    struct SphereColorGradeComponent {
        bool enabled = true;    // false ならパスをスキップする

        //----------------------------------------------------------
        // 球
        //----------------------------------------------------------
        hlslpp::float3 center         = hlslpp::float3(0.0f, 0.0f, 0.0f);    // 中心（ワールド）
        float          radius         = 0.0f;                                // 半径。0なら何も変わらない
        float          edgeSoftness   = 1.0f;                                // 境界をなめらかに切り替える幅（内側へ向かって）

        //----------------------------------------------------------
        // 内側の色
        //----------------------------------------------------------
        hlslpp::float3 tint       = hlslpp::float3(1.0f, 1.0f, 1.0f);    // 掛ける色（1未満で暗くなる）
        float          strength   = 1.0f;                                // 全体の効き（0で無効、1で設定どおり）
        hlslpp::float3 fogColor   = hlslpp::float3(0.0f, 0.0f, 0.0f);    // カメラからの距離で寄せる色（linear）
        float          fogDensity = 0.0f;                                // 距離あたりの濃さ（0で寄せない）

        //----------------------------------------------------------
        // 境界の光
        //----------------------------------------------------------
        hlslpp::float3 edgeColor = hlslpp::float3(0.0f, 0.0f, 0.0f);    // 境界に足す色（linear, HDR。0で光らない）
        float          edgeWidth = 1.0f;                                // 光の帯の幅
    };
}    // namespace Tsukino::BuiltIn::ECS
