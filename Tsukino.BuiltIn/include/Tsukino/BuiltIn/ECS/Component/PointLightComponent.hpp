//--------------------------------------------------------------
//! @file   PointLightComponent.hpp
//! @brief  点光源コンポーネント
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
#include <hlsl++.h>
// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {
    //--------------------------------------------------------------
    //! @struct PointLightComponent
    //! @brief  点光源の設定を持つコンポーネント
    //! @note   位置は同一エンティティの TransformComponent から取得する。
    //!         シャドウは落とさない（影付きライトは DirectionalLightComponent のみ）。
    //!         同時に有効化できる点光源・スポットライトの合計は
    //!         Lighting パスの都合上 TSUKINO_MAX_LIGHTS（64）まで。
    //--------------------------------------------------------------
    struct PointLightComponent {
        hlslpp::float3 color     = {1.0f, 1.0f, 1.0f};    //!< ライトの色
        float          intensity = 1.0f;                   //!< ライトの強度
        float          range     = 10.0f;                  //!< 影響半径（この距離でゼロへスムーズに減衰）
        bool           enabled   = true;                   //!< 無効時はLightingパスへ渡さない
    };
}    // namespace Tsukino::BuiltIn::ECS
