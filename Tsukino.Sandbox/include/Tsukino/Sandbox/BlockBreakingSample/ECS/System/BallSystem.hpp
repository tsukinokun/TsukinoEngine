//-------------------------------------------------------------
//! @file   BallSystem.hpp
//! @brief  BallSystemクラスの宣言
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once
#include <Tsukino/Core/ECS/System/ISystem.hpp>
#include <Tsukino/Core/Math/Matrix.hpp>
// 名前空間 : BlockBreakingSample::ECS
namespace BlockBreakingSample::ECS {
    //-------------------------------------------------------------
    //! @class  BallSystem
    //! @brief  CameraComponentを持つエンティティのビュー行列と
    //-------------------------------------------------------------
    class BallSystem : public Tsukino::ECS::ISystem {
    public:
        //-------------------------------------------------------------
        //! @brief 更新処理
        //! @param registry  [in] エンジンのECSレジストリのラッパー
        //! @param deltaTime [in] デルタタイム
        //-------------------------------------------------------------
        void Update(Tsukino::ECS::Registry& registry, float deltaTime) override;
    };
}    // namespace BlockBreakingSample::ECS
