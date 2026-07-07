//-------------------------------------------------------------
//! @file   TimerSystem.hpp
//! @brief  TimerSystemクラスの宣言
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
    //! @class  TimerSystem
    //! @brief  TimerComponentを持つエンティティの残り時間を毎フレーム更新する
    //-------------------------------------------------------------
    class TimerSystem : public Tsukino::ECS::ISystem {
    public:
        //------------------------------------------------------------
        //! @brief  コンストラクタ
        //! @param  eventBus [in] イベントバスへの参照（TimeUpEventを発行するため）
        //------------------------------------------------------------
        explicit TimerSystem(Tsukino::ECS::EventBus& eventBus);

        //------------------------------------------------------------
        //! @brief  更新処理
        //! @param  eventBus [in] イベントバスへの参照（TimeUpEventを発行するため）
        //! @param  priority [in] システムの優先度（低いほど先に更新される）
        //-------------------------------------------------------------
        void Update(Tsukino::ECS::Registry& registry, float deltaTime) override;

    private:
        Tsukino::ECS::EventBus& m_eventBus;    //!< イベントバスへの参照（TimeUpEventを発行するため）
    };

}    // namespace WaterGame::ECS
