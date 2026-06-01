//-------------------------------------------------------------
//! @file   SceneEvent.hpp
//! @brief  シーンのライフサイクルに関するBuilt-inイベント
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once
#include <string>
// 名前空間 : Tsukino::ECS::EngineEvent
// エンジンのシーン管理に関するイベントを定義する名前空間、特別にエンジン層に置く
namespace Tsukino::ECS::EngineEvent {
    //-------------------------------------------------------------
    //! @struct SceneInitializedEvent
    //! @brief  Scene::Initialize() が完了したとき
    //! @note   Scene::Initialize() が自動で Publish する
    //-------------------------------------------------------------
    struct SceneInitializedEvent {};

    //-------------------------------------------------------------
    //! @struct SceneChangeRequestedEvent
    //! @brief  シーン遷移が要求されたとき
    //! @note   ユーザーが任意のタイミングで Publish して使う
    //-------------------------------------------------------------
    struct SceneChangeRequestedEvent {
        std::string nextSceneName;    //!< 遷移先シーン名
    };

}    // namespace Tsukino::ECS::EngineEvent
