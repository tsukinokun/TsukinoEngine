//--------------------------------------------------------------
//! @file   SpotLightComponent.hpp
//! @brief  スポットライトコンポーネント
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
#include <hlsl++.h>
// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {
    //--------------------------------------------------------------
    //! @struct SpotLightComponent
    //! @brief  スポットライトの設定を持つコンポーネント
    //! @note   位置・向きは同一エンティティの TransformComponent から取得する
    //!         （向きはローカル+Z軸: hlslpp::mul(rotation, float3(0,0,1))）。
    //!         シャドウは落とさない（影付きライトは DirectionalLightComponent のみ）。
    //--------------------------------------------------------------
    struct SpotLightComponent {
        hlslpp::float3 color        = {1.0f, 1.0f, 1.0f};    //!< ライトの色
        float          intensity    = 1.0f;                   //!< ライトの強度
        float          range        = 10.0f;                  //!< 影響半径
        float          innerConeDeg = 20.0f;                  //!< 内側円錐角度（度）。この角度内は減衰なし
        float          outerConeDeg = 30.0f;                  //!< 外側円錐角度（度）。この角度を超えると寄与ゼロ
        bool           enabled      = true;                   //!< 無効時はLightingパスへ渡さない
    };
}    // namespace Tsukino::BuiltIn::ECS
