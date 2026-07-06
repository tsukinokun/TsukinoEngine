//-------------------------------------------------------------
//! @file   GameCameraSystem.hpp
//! @brief  GameCameraSystemクラスの宣言
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once
#include <Tsukino/Core/ECS/System/ISystem.hpp>
// 名前空間 : WaterGame::ECS
namespace WaterGame::ECS {

    //-------------------------------------------------------------
    //! @class  GameCameraSystem
    //! @brief  WASD入力によってターゲット周囲を回転するゲームカメラの更新システム
    //-------------------------------------------------------------
    class GameCameraSystem : public Tsukino::ECS::ISystem {
    public:
        //-------------------------------------------------------------
        //! @brief  システムの更新処理
        //! @param  registry  [in] ECSレジストリ
        //! @param  deltaTime [in] 前フレームからの経過時間（秒）
        //-------------------------------------------------------------
        void Update(Tsukino::ECS::Registry& registry, float deltaTime) override;
    };

}    // namespace WaterGame::ECS
