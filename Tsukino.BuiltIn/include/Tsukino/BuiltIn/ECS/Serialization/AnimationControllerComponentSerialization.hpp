//--------------------------------------------------------------
//! @file   CameraComponentSerialization.hpp
//! @brief  CameraComponentに対するcerealシリアライズ定義
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
#include <Tsukino/BuiltIn/ECS/Component/CameraComponent.hpp>

#include <Tsukino/Core/Math/Serialization/HlslppSerialization.hpp>   

#include <cereal/cereal.hpp>

// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {
    //--------------------------------------------------------------
    //! @brief  CameraComponentのcerealシリアライズ定義
    //--------------------------------------------------------------
    template <class Archive>
    void save(Archive& archive, const CameraComponent& camera) {
        // 保存したい初期パラメータだけをアーカイブに流し込む
        archive(cereal::make_nvp("projectionType", camera.projectionType),
                cereal::make_nvp("orthoSize", camera.orthoSize),
                cereal::make_nvp("fov", camera.fov),
                cereal::make_nvp("aspectRatio", camera.aspectRatio),
                cereal::make_nvp("nearZ", camera.nearZ),
                cereal::make_nvp("farZ", camera.farZ),
                cereal::make_nvp("useLookAt", camera.useLookAt),
                cereal::make_nvp("lookAtTarget", camera.lookAtTarget),
                cereal::make_nvp("isPrimary", camera.isPrimary));
        // viewMatrix などのキャッシュは保存しない！
    }

    //--------------------------------------------------------------
    //! @brief  CameraComponentのcerealデシリアライズ定義
    //--------------------------------------------------------------
    template <class Archive>
    void load(Archive& archive, CameraComponent& camera) {
        // ロード時も同様に変数を復元
        archive(camera.projectionType,
                camera.orthoSize,
                camera.fov,
                camera.aspectRatio,
                camera.nearZ,
                camera.farZ,
                camera.useLookAt,
                camera.lookAtTarget,
                camera.isPrimary);
        // ロード直後は行列の再計算が必要なので、dirtyフラグを強制的に立てる！
        camera.dirty = true;
    }

}    // namespace Tsukino::BuiltIn::ECS
