//-------------------------------------------------------------
//! @file   InGameCameraSystem.hpp
//! @brief  InGameCameraSystemクラスの宣言
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once
#include <Tsukino/Core/ECS/System/ISystem.hpp>
#include <Tsukino/Core/Math/Matrix.hpp>
// 名前空間 : JumpGameSample::ECS
namespace JumpGameSample::ECS {
    //-------------------------------------------------------------
    //! @class  InGameCameraSystem
    //! @brief  プレイヤーのシステム
    //-------------------------------------------------------------
    class InGameCameraSystem : public Tsukino::ECS::ISystem {
    public:
        //-------------------------------------------------------------
        //! @brief 更新処理
        //! @param registry  [in] エンジンのECSレジストリのラッパー
        //! @param deltaTime [in] デルタタイム
        //-------------------------------------------------------------
        void Update(Tsukino::ECS::Registry& registry, float deltaTime) override;

    private:
        Tsukino::ECS::Entity playerEntity = entt::null;    // プレイヤーのエンティティをキャッシュ
    };
}    // namespace JumpGameSample::ECS
