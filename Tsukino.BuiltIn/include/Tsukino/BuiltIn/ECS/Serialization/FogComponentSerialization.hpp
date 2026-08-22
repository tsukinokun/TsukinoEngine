//--------------------------------------------------------------
//! @file   FogComponentSerialization.hpp
//! @brief  FogComponentのcerealシリアライズ定義
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
#include <Tsukino/BuiltIn/ECS/Component/FogComponent.hpp>

#include <Tsukino/Core/Math/Serialization/HlslppSerialization.hpp>

#include <cereal/cereal.hpp>
// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {

    //--------------------------------------------------------------
    //! @brief  FogComponentのcerealシリアライズ定義
    //--------------------------------------------------------------
    template <class Archive>
    void save(Archive& archive, const FogComponent& fog) {
        archive(cereal::make_nvp("enabled", fog.enabled),
                cereal::make_nvp("color", fog.color),
                cereal::make_nvp("density", fog.density),
                cereal::make_nvp("startDistance", fog.startDistance),
                cereal::make_nvp("maxOpacity", fog.maxOpacity),
                cereal::make_nvp("heightFogEnabled", fog.heightFogEnabled),
                cereal::make_nvp("height", fog.height),
                cereal::make_nvp("heightFalloff", fog.heightFalloff),
                cereal::make_nvp("heightDensity", fog.heightDensity),
                cereal::make_nvp("sunColor", fog.sunColor),
                cereal::make_nvp("sunScatterPower", fog.sunScatterPower),
                cereal::make_nvp("noiseEnabled", fog.noiseEnabled),
                cereal::make_nvp("noiseScale", fog.noiseScale),
                cereal::make_nvp("noiseIntensity", fog.noiseIntensity),
                cereal::make_nvp("windDirection", fog.windDirection),
                cereal::make_nvp("windSpeed", fog.windSpeed));
    }

    //--------------------------------------------------------------
    //! @brief  FogComponentのcerealデシリアライズ定義
    //--------------------------------------------------------------
    template <class Archive>
    void load(Archive& archive, FogComponent& fog) {
        archive(fog.enabled, fog.color, fog.density, fog.startDistance, fog.maxOpacity, fog.heightFogEnabled, fog.height, fog.heightFalloff,
                fog.heightDensity, fog.sunColor, fog.sunScatterPower, fog.noiseEnabled, fog.noiseScale, fog.noiseIntensity, fog.windDirection,
                fog.windSpeed);
    }

}    // namespace Tsukino::BuiltIn::ECS
