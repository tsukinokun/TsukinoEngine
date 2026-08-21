//-------------------------------------------------------------
//! @file   TpsCameraSystem.hpp
//! @brief  TpsCameraSystemクラスの宣言
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once
#include <Tsukino/Core/ECS/System/ISystem.hpp>
// 名前空間 : ActionGame::ECS
namespace ActionGame::ECS {
    //-------------------------------------------------------------
    //! @class  TpsCameraSystem
    //! @brief  プレイヤーの向いている方向の後方に追従する三人称視点カメラの更新システム
    //-------------------------------------------------------------
    class TpsCameraSystem : public Tsukino::ECS::ISystem {
    public:
        //-------------------------------------------------------------
        //! @brief 更新処理
        //! @param registry  [in] エンジンのECSレジストリのラッパー
        //! @param deltaTime [in] デルタタイム
        //-------------------------------------------------------------
        void Update(Tsukino::ECS::Registry& registry, float deltaTime) override;
    };
}    // namespace ActionGame::ECS
