//-------------------------------------------------------------
//! @file   DebugCameraSystem.hpp
//! @brief  DebugCameraSystemクラスの宣言
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once
#ifdef _DEBUG
#include <Tsukino/Core/ECS/System/ISystem.hpp>
// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {
    //-------------------------------------------------------------
    //! @class  DebugCameraSystem
    //! @brief  デバッグカメラの操作を管理するシステム
    //-------------------------------------------------------------
    class DebugCameraSystem final : public Tsukino::ECS::ISystem {
    public:
        //-------------------------------------------------------------
        // システムの更新
        //! @param  registry    [in] ECS レジストリ
        //! @param  deltaTime   [in] 前フレームからの経過時間
        //-------------------------------------------------------------
        void Update(Tsukino::ECS::Registry& registry, float deltaTime) override;
    };

}    // namespace Tsukino::BuiltIn::ECS
#endif    // _DEBUG
