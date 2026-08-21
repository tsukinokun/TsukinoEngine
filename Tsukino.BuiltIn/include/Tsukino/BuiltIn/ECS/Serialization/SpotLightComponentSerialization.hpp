//--------------------------------------------------------------
//! @file   SpotLightComponentSerialization.hpp
//! @brief  SpotLightComponentのcerealシリアライズ定義
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
#include <Tsukino/BuiltIn/ECS/Component/SpotLightComponent.hpp>

#include <Tsukino/Core/Math/Serialization/HlslppSerialization.hpp>

#include <cereal/cereal.hpp>
// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {

    //--------------------------------------------------------------
    //! @brief  SpotLightComponentのcerealシリアライズ定義
    //--------------------------------------------------------------
    template <class Archive>
    void save(Archive& archive, const SpotLightComponent& light) {
        archive(cereal::make_nvp("color", light.color),
                cereal::make_nvp("intensity", light.intensity),
                cereal::make_nvp("range", light.range),
                cereal::make_nvp("innerConeDeg", light.innerConeDeg),
                cereal::make_nvp("outerConeDeg", light.outerConeDeg),
                cereal::make_nvp("enabled", light.enabled));
    }

    //--------------------------------------------------------------
    //! @brief  SpotLightComponentのcerealデシリアライズ定義
    //--------------------------------------------------------------
    template <class Archive>
    void load(Archive& archive, SpotLightComponent& light) {
        archive(light.color, light.intensity, light.range, light.innerConeDeg, light.outerConeDeg, light.enabled);
    }

}    // namespace Tsukino::BuiltIn::ECS
