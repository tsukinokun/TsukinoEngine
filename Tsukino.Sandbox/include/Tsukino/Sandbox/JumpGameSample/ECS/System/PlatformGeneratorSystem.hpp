//-------------------------------------------------------------
//! @file   PlatformGeneratorSystem.hpp
//! @brief  PlatformGeneratorSystemクラスの宣言
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once
#include <Tsukino/Core/ECS/System/ISystem.hpp>
#include <Tsukino/Core/Math/Matrix.hpp>
// 名前空間 : JumpGameSample::ECS
namespace JumpGameSample::ECS {
    //-------------------------------------------------------------
    //! @class  PlatformGeneratorSystem
    //! @brief  土台生成器械のシステム
    //-------------------------------------------------------------
    class PlatformGeneratorSystem : public Tsukino::ECS::ISystem {
    public:
        //-------------------------------------------------------------
        //! @brief 更新処理
        //! @param registry  [in] エンジンのECSレジストリのラッパー
        //! @param deltaTime [in] デルタタイム
        //-------------------------------------------------------------
        void Update(Tsukino::ECS::Registry& registry, float deltaTime) override;

        //-------------------------------------------------------------
        //! @brief  新しい土台の生成
        //! @param  registry    [in] エンジンのECSレジ
        //! @param  offsetY     [in] 土台の生成位置のYオフセット
        //-------------------------------------------------------------
        void SpawnNewPlatform(Tsukino::ECS::Registry& registry, float offsetY);
    };
}    // namespace JumpGameSample::ECS
