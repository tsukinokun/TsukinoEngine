//-------------------------------------------------------------
//! @file   DotEatSystem.hpp
//! @brief  DotEatSystemクラスの宣言
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once

#include <Tsukino/Core/ECS/System/ISystem.hpp>

namespace Tsukino::ECS {
    class EventBus;    // 前方宣言
}

// 名前空間 : WaterGame::ECS
namespace WaterGame::ECS {

    //-------------------------------------------------------------
    //! @class  DotEatSystem
    //! @brief  プレイヤー（ボール）とドットの距離を判定し、近づいたドットを消費する
    //-------------------------------------------------------------
    class DotEatSystem : public Tsukino::ECS::ISystem {
    public:
        //-------------------------------------------------------------
        //! @brief  コンストラクタ
        //! @param eventBus [in] イベントバスの参照
        //-------------------------------------------------------------
        explicit DotEatSystem(Tsukino::ECS::EventBus& eventBus);

        //-------------------------------------------------------------
        //! @brief  システムの更新
        //-------------------------------------------------------------
        void Update(Tsukino::ECS::Registry& registry, float deltaTime) override;

    private:
        Tsukino::ECS::EventBus& m_eventBus;    //!< イベントバスの参照
    };
}    // namespace WaterGame::ECS
