//--------------------------------------------------------------
//! @file   DirectionalLightComponent.hpp
//! @brief  ディレクショナルライトコンポーネント
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
#include <hlsl++.h>
// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {
    //--------------------------------------------------------------
    //! @struct DirectionalLightComponent
    //! @brief  平行光源の設定を持つコンポーネント
    //! @note   エンティティにアタッチして使用する
    //!         シーン内に複数存在する場合は最初に見つかったものを使用する
    //--------------------------------------------------------------
    struct DirectionalLightComponent {
        hlslpp::float3 direction  = {0.0f, -1.0f, 0.0f};    //!< ライトの方向（正規化推奨）
        hlslpp::float3 color      = {1.0f, 1.0f, 1.0f};     //!< ライトの色
        float          intensity  = 1.0f;                   //!< ライトの強度
        bool           castShadow = true;                   //!< シャドウを落とすか
    };
}    // namespace Tsukino::BuiltIn::ECS
