//--------------------------------------------------------------
//! @file   DirectionalLightComponentSerialization.hpp
//! @brief  DirectionalLightComponentのcerealシリアライズ定義
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
#include <Tsukino/BuiltIn/ECS/Component/DirectionalLightComponent.hpp>

#include <Tsukino/Core/Math/Serialization/HlslppSerialization.hpp>

#include <cereal/cereal.hpp>
// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {

    //--------------------------------------------------------------
    //! @brief  DirectionalLightComponentのcerealシリアライズ定義
    //--------------------------------------------------------------
    template <class Archive>
    void save(Archive& archive, const DirectionalLightComponent& light) {
        archive(cereal::make_nvp("direction", light.direction),
                cereal::make_nvp("color", light.color),
                cereal::make_nvp("intensity", light.intensity),
                cereal::make_nvp("castShadow", light.castShadow));
    }

    //--------------------------------------------------------------
    //! @brief  DirectionalLightComponentのcerealデシリアライズ定義
    //--------------------------------------------------------------
    template <class Archive>
    void load(Archive& archive, DirectionalLightComponent& light) {
        archive(light.direction, light.color, light.intensity, light.castShadow);
    }

}    // namespace Tsukino::BuiltIn::ECS
