//-------------------------------------------------------------
//! @file   DiceBoundsSystem.hpp
//! @brief  DiceBoundsSystemクラスの宣言
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once
#include <Tsukino/Core/ECS/System/ISystem.hpp>

// 名前空間 : LuckGameSampleScene::ECS
namespace LuckGameSampleScene::ECS {

    //-------------------------------------------------------------
    //! @class  DiceBoundsSystem
    //! @brief  お椀中心から一定半径を超えて転がり出たサイコロを検知し、
    //!         お椀中心上空へ戻すシステム（ノーカウント・rollCountは増やさない）。
    //!         優先度は PhysicsSystem の直後に登録すること。
    //-------------------------------------------------------------
    class DiceBoundsSystem : public Tsukino::ECS::ISystem {
    public:
        void Update(Tsukino::ECS::Registry& registry, float deltaTime) override;
    };

}    // namespace LuckGameSampleScene::ECS
