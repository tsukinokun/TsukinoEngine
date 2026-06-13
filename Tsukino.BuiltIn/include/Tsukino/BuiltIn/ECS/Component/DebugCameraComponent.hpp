//-------------------------------------------------------------
//! @file   DebugCameraComponent.hpp
//! @brief  DebugCameraComponentクラスの宣言
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once
// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {
    //-------------------------------------------------------------
    //! @class  DebugCameraComponent
    //! @brief  デバッグカメラの設定を管理するクラス
    //-------------------------------------------------------------
    struct DebugCameraComponent {
        float moveSpeed   = 10.0f;
        float sprintSpeed = 30.0f;
        float mouseSens   = 0.15f;

        float yaw   = 0.0f;
        float pitch = 0.0f;

        bool isActive = false;    // F5でトグル
    };

}    // namespace Tsukino::BuiltIn::ECS
