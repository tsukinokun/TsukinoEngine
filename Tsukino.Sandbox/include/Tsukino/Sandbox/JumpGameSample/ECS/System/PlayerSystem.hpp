//-------------------------------------------------------------
//! @file   PlayerSystem.hpp
//! @brief  PlayerSystemクラスの宣言
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once
#include <Tsukino/Core/ECS/Event/ScopedConnection.hpp>
#include <Tsukino/Core/ECS/System/ISystem.hpp>
#include <Tsukino/BuiltIn/ECS/Event/CollisionEnterEvent.hpp>
namespace Tsukino::ECS {
    class EventBus;    // 前方宣言
}

// 名前空間 : JumpGameSample::ECS
namespace JumpGameSample::ECS {
    //-------------------------------------------------------------
    //! @class  PlayerSystem
    //! @brief  プレイヤーのシステム
    //-------------------------------------------------------------
    class PlayerSystem : public Tsukino::ECS::ISystem {
    public:
        //-------------------------------------------------------------
        //! @brief  コンストラクタ
        //! @param eventBus [in] イベントバスの参照
        //-------------------------------------------------------------
        explicit PlayerSystem(Tsukino::ECS::EventBus& eventBus);

        //-------------------------------------------------------------
        //! @brief 更新処理
        //! @param registry  [in] エンジンのECSレジストリのラッパー
        //! @param deltaTime [in] デルタタイム
        //-------------------------------------------------------------
        void Update(Tsukino::ECS::Registry& registry, float deltaTime) override;

    private:
        Tsukino::ECS::ScopedConnection                          m_collisionConnection;    // 衝突イベントの購読解除を自動で行うためのScopedConnection
        std::vector<Tsukino::BuiltIn::ECS::CollisionEnterEvent> m_pendingCollisions;      //
    };
}    // namespace JumpGameSample::ECS
