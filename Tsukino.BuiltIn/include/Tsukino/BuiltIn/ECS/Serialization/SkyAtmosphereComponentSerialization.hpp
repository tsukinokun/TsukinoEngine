//--------------------------------------------------------------
//! @file   SkyAtmosphereComponentSerialization.hpp
//! @brief  SkyAtmosphereComponentのcerealシリアライズ定義
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
#include <Tsukino/BuiltIn/ECS/Component/SkyAtmosphereComponent.hpp>

#include <Tsukino/Core/Math/Serialization/HlslppSerialization.hpp>

#include <cereal/cereal.hpp>
// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {

    //--------------------------------------------------------------
    //! @brief  SkyAtmosphereComponentのcerealシリアライズ定義
    //--------------------------------------------------------------
    template <class Archive>
    void save(Archive& archive, const SkyAtmosphereComponent& sky) {
        archive(cereal::make_nvp("rayleighScattering", sky.rayleighScattering),
                cereal::make_nvp("mieScattering", sky.mieScattering),
                cereal::make_nvp("mieAnisotropy", sky.mieAnisotropy),
                cereal::make_nvp("atmosphereHeight", sky.atmosphereHeight),
                cereal::make_nvp("planetRadius", sky.planetRadius),
                cereal::make_nvp("sunIntensity", sky.sunIntensity),
                cereal::make_nvp("sunDiskSize", sky.sunDiskSize),
                cereal::make_nvp("groundColor", sky.groundColor));
    }

    //--------------------------------------------------------------
    //! @brief  SkyAtmosphereComponentのcerealデシリアライズ定義
    //--------------------------------------------------------------
    template <class Archive>
    void load(Archive& archive, SkyAtmosphereComponent& sky) {
        archive(sky.rayleighScattering, sky.mieScattering, sky.mieAnisotropy, sky.atmosphereHeight, sky.planetRadius, sky.sunIntensity, sky.sunDiskSize,
                sky.groundColor);
    }

}    // namespace Tsukino::BuiltIn::ECS
