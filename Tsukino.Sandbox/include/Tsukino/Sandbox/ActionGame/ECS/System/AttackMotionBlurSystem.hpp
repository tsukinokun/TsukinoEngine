//-------------------------------------------------------------
//! @file   AttackMotionBlurSystem.hpp
//! @brief  攻撃中にモーションブラーを強めるシステムの宣言
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once
#include <Tsukino/Core/ECS/System/ISystem.hpp>
// 名前空間 : ActionGame::ECS
namespace ActionGame::ECS {
    //-------------------------------------------------------------
    //! @class  AttackMotionBlurSystem
    //! @brief  攻撃の進行度に応じて MotionBlurComponent::strength を書き換えるシステム
    //!
    //! @note   エンジンは「強度パラメータを受け取って描く」だけに徹しており、
    //!         いつブラーを強めるかというゲーム側の演出判断はここが持つ。
    //!         エンジン層の型（MotionBlurComponent）へアプリ層が書き込む
    //!         一方向依存なので、エンジンは ActionGame を一切知らない。
    //!
    //!         実行順序は CombatSystem（attackBlendを更新）より後、
    //!         MotionBlurSystem（Rendererへ転送）より前であること。
    //-------------------------------------------------------------
    class AttackMotionBlurSystem : public Tsukino::ECS::ISystem {
    public:
        //-------------------------------------------------------------
        //! @brief 更新処理
        //-------------------------------------------------------------
        void Update(Tsukino::ECS::Registry& registry, float deltaTime) override;
    };
}    // namespace ActionGame::ECS
