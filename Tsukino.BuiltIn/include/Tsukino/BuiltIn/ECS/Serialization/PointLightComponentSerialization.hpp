//--------------------------------------------------------------
//! @file   PointLightComponentSerialization.hpp
//! @brief  PointLightComponentのcerealシリアライズ定義
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
#include <Tsukino/BuiltIn/ECS/Component/PointLightComponent.hpp>

#include <Tsukino/Core/Math/Serialization/HlslppSerialization.hpp>

#include <cereal/cereal.hpp>
// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {

    //--------------------------------------------------------------
    //! @brief  PointLightComponentのcerealシリアライズ定義
    //--------------------------------------------------------------
    template <class Archive>
    void save(Archive& archive, const PointLightComponent& light) {
        archive(cereal::make_nvp("color", light.color),
                cereal::make_nvp("intensity", light.intensity),
                cereal::make_nvp("range", light.range),
                cereal::make_nvp("enabled", light.enabled));
    }

    //--------------------------------------------------------------
    //! @brief  PointLightComponentのcerealデシリアライズ定義
    //--------------------------------------------------------------
    template <class Archive>
    void load(Archive& archive, PointLightComponent& light) {
        archive(light.color, light.intensity, light.range, light.enabled);
    }

}    // namespace Tsukino::BuiltIn::ECS
