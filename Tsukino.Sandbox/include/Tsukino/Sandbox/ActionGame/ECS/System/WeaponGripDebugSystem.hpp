//-------------------------------------------------------------
//! @file   WeaponGripDebugSystem.hpp
//! @brief  WeaponGripDebugSystemクラスの宣言
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once
#include <Tsukino/Core/ECS/System/ISystem.hpp>
// 名前空間 : ActionGame::ECS
namespace ActionGame::ECS {
    //-------------------------------------------------------------
    //! @class  WeaponGripDebugSystem
    //! @brief  武器の握り位置・角度（WeaponComponent::gripPointLocal / attackLocalOffset /
    //!         attackGripRotationOffset）を実機で調整するためのデバッグ専用システム。
    //!         F6で調整モードをON/OFFし、F10で攻撃ポーズを固定した状態でIJKLUOキーにより
    //!         現在の編集対象を微調整、F9で貼り付け可能なC++コードとしてログへ出力する。
    //!         _DEBUGビルドでのみScene/システムへ登録される
    //-------------------------------------------------------------
    class WeaponGripDebugSystem : public Tsukino::ECS::ISystem {
    public:
        //-------------------------------------------------------------
        //! @brief 更新処理
        //! @param registry  [in] エンジンのECSレジストリのラッパー
        //! @param deltaTime [in] デルタタイム
        //-------------------------------------------------------------
        void Update(Tsukino::ECS::Registry& registry, float deltaTime) override;
    };
}    // namespace ActionGame::ECS
