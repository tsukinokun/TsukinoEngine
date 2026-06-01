//--------------------------------------------------------------
//! @file   CameraDescSerialization.hpp
//! @brief  CameraDescに対するcerealシリアライズ定義
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
#include <Tsukino/Core/Math/Serialization/HlslppSerialization.hpp>    // 数学のシリアライズ
#include <Tsukino/BuiltIn/ECS/Desc/CameraDesc.hpp>      // 純粋な設計図

#include <cereal/cereal.hpp>
// 名前空間 : cereal
namespace cereal {
    //--------------------------------------------------------------
    //! @brief CameraDescのシリアライズ定義
    //! @param Archive シリアライズのアーカイブ
    //! @param desc シリアライズ対象のCameraDesc
    //--------------------------------------------------------------
    template <class Archive>
    void serialize(Archive& ar, Tsukino::BuiltIn::ECS::CameraDesc& desc) {
        ar(cereal::make_nvp("projectionType", desc.projectionType),
           cereal::make_nvp("orthoSize", desc.orthoSize),
           cereal::make_nvp("fov", desc.fov),
           cereal::make_nvp("aspectRatio", desc.aspectRatio),
           cereal::make_nvp("nearZ", desc.nearZ),
           cereal::make_nvp("farZ", desc.farZ),
           cereal::make_nvp("useLookAt", desc.useLookAt),
           cereal::make_nvp("lookAtTarget", desc.lookAtTarget),
           cereal::make_nvp("isPrimary", desc.isPrimary));
    }

}    // namespace cereal
