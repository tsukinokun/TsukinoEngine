//-------------------------------------------------------------
//! @file   PickupPromptComponent.hpp
//! @brief  PickupPromptComponent構造体の宣言
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once
// 名前空間 : ActionGame::ECS
namespace ActionGame::ECS {
    //-------------------------------------------------------------
    //! @struct PickupPromptComponent
    //! @brief  「Fキーで拾う」を促すUIラベル用エンティティの目印タグ。
    //!         TransformComponent + FontComponentとセットで付与し、
    //!         PickupSystemが毎フレームテキストとスクリーン座標を書き換える
    //-------------------------------------------------------------
    struct PickupPromptComponent {
        float screenOffsetY = 0.0f;    //!< 変換後のスクリーン座標からの微調整
    };
}    // namespace ActionGame::ECS
