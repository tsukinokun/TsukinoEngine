//-------------------------------------------------------------
//! @file   PhysicsSystem.hpp
//! @brief  PhysicsSystemクラスの宣言
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once
#include <Tsukino/Core/ECS/System/ISystem.hpp>

namespace Tsukino::ECS {
    class EventBus;
}

// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {
    struct CollisionComponent;    // 前方宣言

    //-------------------------------------------------------------
    //! @class  PhysicsSystem
    //-------------------------------------------------------------
    class PhysicsSystem : public Tsukino::ECS::ISystem {
    public:
        //-------------------------------------------------------------
        //! @brief コンストラクタ
        //-------------------------------------------------------------
        explicit PhysicsSystem(Tsukino::ECS::EventBus& eventBus);

        //-------------------------------------------------------------
        //! @brief デストラクタ
        //-------------------------------------------------------------
        ~PhysicsSystem() override;

        //-------------------------------------------------------------
        // システムの更新
        //! @param  registry    [in] ECS レジストリ
        //! @param  deltaTime   [in] 前フレームからの経過時間
        //-------------------------------------------------------------
        void Update(Tsukino::ECS::Registry& registry, float deltaTime) override;

    private:
        struct Impl;
        Impl* m_impl;
    };

}    // namespace Tsukino::BuiltIn::ECS
