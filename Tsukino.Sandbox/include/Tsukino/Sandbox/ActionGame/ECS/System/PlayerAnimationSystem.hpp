//-------------------------------------------------------------
//! @file   PlayerAnimationSystem.hpp
//! @brief  PlayerAnimationSystemクラスの宣言
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once
#include <Tsukino/Core/ECS/System/ISystem.hpp>
#include <Tsukino/Sandbox/ActionGame/ECS/Utility/StateMachine.hpp>
#include <Tsukino/Sandbox/ActionGame/ECS/Component/PlayerAnimationSetComponent.hpp>
// 名前空間 : ActionGame::ECS
namespace ActionGame::ECS {
    //-------------------------------------------------------------
    //! @class  PlayerAnimationSystem
    //! @brief  プレイヤーの状態（接地/移動/スプリント）を見て、
    //!         Idle/Run/FastRun/Jumpのアニメーションステートマシンを進行させるシステム
    //-------------------------------------------------------------
    class PlayerAnimationSystem : public Tsukino::ECS::ISystem {
    public:
        //-------------------------------------------------------------
        //! @brief コンストラクタ。各ステートのOnEnterコールバック（クリップ切り替え）を登録する
        //-------------------------------------------------------------
        PlayerAnimationSystem();

        //-------------------------------------------------------------
        //! @brief 更新処理
        //! @param registry  [in] エンジンのECSレジストリのラッパー
        //! @param deltaTime [in] デルタタイム
        //-------------------------------------------------------------
        void Update(Tsukino::ECS::Registry& registry, float deltaTime) override;

    private:
        StateMachine<PlayerAnimState> m_stateMachine;    //!< Idle/Run/FastRun/Jumpの遷移とクリップ切り替えを管理する
    };
}    // namespace ActionGame::ECS
