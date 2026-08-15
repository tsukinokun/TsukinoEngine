//-------------------------------------------------------------
//! @file   CombatSystem.hpp
//! @brief  CombatSystemクラスの宣言
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once
#include <Tsukino/Core/ECS/System/ISystem.hpp>
// 名前空間 : ActionGame::ECS
namespace ActionGame::ECS {
    //-------------------------------------------------------------
    //! @class  CombatSystem
    //! @brief  武器の当たり判定の有効化・追従、ダメージ処理、死亡判定を行うシステム。
    //!         当たり判定はJolt物理を使わず、Transform間の距離判定で簡易的に行う
    //!         （プレイヤーはCharacterVirtualで駆動されており、Jolt標準の
    //!           ContactListenerがCharacterVirtualの接触をイベント化しないため）
    //-------------------------------------------------------------
    class CombatSystem : public Tsukino::ECS::ISystem {
    public:
        //-------------------------------------------------------------
        //! @brief 更新処理
        //! @param registry  [in] エンジンのECSレジストリのラッパー
        //! @param deltaTime [in] デルタタイム
        //-------------------------------------------------------------
        void Update(Tsukino::ECS::Registry& registry, float deltaTime) override;
    };
}    // namespace ActionGame::ECS
