//-------------------------------------------------------------
//! @file   ScrollBarComponent.hpp
//! @brief  ScrollBarComponentクラスの宣言
//-------------------------------------------------------------
#pragma once
#include <Tsukino/Core/ECS/Entity/Entity.hpp>

#include <entt/entt.hpp>
#include <hlsl++.h>

// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {
    //-------------------------------------------------------------
    //! @struct ScrollBarComponent
    //! @brief  ScrollViewComponent のスクロールバー
    //! @note   「溝」の画面スプライトに付け、ScrollViewComponent::scrollBar から指す。
    //!         つまみ（thumb）は別の画面スプライトで、溝の子にはしない（溝の拡大率を受け継がないように）。
    //!         ScrollViewSystem が毎フレーム、溝とつまみの拡大率・つまみの位置を決める：
    //!         中身が枠に収まるとき・スクロールが enabled でないときは両方とも拡大率 0 で隠す。
    //!         色や幅（size.x）は作る側が決める
    //-------------------------------------------------------------
    struct ScrollBarComponent {
        Tsukino::ECS::Entity thumb          = entt::null;                      // つまみのエンティティ
        hlslpp::float2       size           = hlslpp::float2(0.0f, 0.0f);    // 溝の大きさ（画面ピクセル。中心は溝のエンティティの位置）
        float                minThumbLength = 24.0f;                         // つまみの最短の長さ（ピクセル）
    };
}    // namespace Tsukino::BuiltIn::ECS
