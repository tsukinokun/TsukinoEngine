//-------------------------------------------------------------
//! @file   ScoreUISystem.hpp
//! @brief  ScoreUISystemクラスの宣言
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
    //! @class  ScoreUISystem
    //! @brief  スコアUIのシステム
    //-------------------------------------------------------------
    class ScoreUISystem : public Tsukino::ECS::ISystem {
    public:
        //-------------------------------------------------------------
        //! @brief 更新処理
        //! @param registry  [in] エンジンのECSレジストリのラッパー
        //! @param deltaTime [in] デルタタイム
        //-------------------------------------------------------------
        void Update(Tsukino::ECS::Registry& registry, float deltaTime) override;

    private:
    };
}    // namespace JumpGameSample::ECS
