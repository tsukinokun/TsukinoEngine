//--------------------------------------------------------------
//! @file   DebugCameraComponentSerialization.hpp
//! @brief  DebugCameraComponentのcerealシリアライズ定義
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
#include <Tsukino/BuiltIn/ECS/Component/DebugCameraComponent.hpp>

#include <cereal/cereal.hpp>
// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {

    //--------------------------------------------------------------
    //! @brief  DebugCameraComponentのcerealシリアライズ定義
    //--------------------------------------------------------------
    template <class Archive>
    void save(Archive& archive, const DebugCameraComponent& debugCamera) {
        archive(cereal::make_nvp("moveSpeed", debugCamera.moveSpeed),
                cereal::make_nvp("sprintSpeed", debugCamera.sprintSpeed),
                cereal::make_nvp("mouseSens", debugCamera.mouseSens));
        // yaw/pitch/isActiveは実行時の状態のため保存しない
    }

    //--------------------------------------------------------------
    //! @brief  DebugCameraComponentのcerealデシリアライズ定義
    //--------------------------------------------------------------
    template <class Archive>
    void load(Archive& archive, DebugCameraComponent& debugCamera) {
        archive(debugCamera.moveSpeed, debugCamera.sprintSpeed, debugCamera.mouseSens);
        debugCamera.yaw      = 0.0f;
        debugCamera.pitch    = 0.0f;
        debugCamera.isActive = false;
    }

}    // namespace Tsukino::BuiltIn::ECS
