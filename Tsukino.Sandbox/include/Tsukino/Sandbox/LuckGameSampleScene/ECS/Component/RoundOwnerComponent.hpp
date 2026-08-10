//-------------------------------------------------------------
//! @file   RoundOwnerComponent.hpp
//! @brief  RoundOwnerComponentクラスの宣言
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once
#include <hlsl++.h>

// 名前空間 : LuckGameSampleScene::ECS
namespace LuckGameSampleScene::ECS {

    //-------------------------------------------------------------
    //! @struct RoundOwnerComponent
    //! @brief  このサイコロがどちらのお椀に属するかを示す（場外判定の基準座標として使用）
    //-------------------------------------------------------------
    struct RoundOwnerComponent {
        hlslpp::float3 bowlCenter = hlslpp::float3(0.0f, 0.0f, 0.0f);    //!< 所属するお椀のワールド座標
    };

}    // namespace LuckGameSampleScene::ECS
