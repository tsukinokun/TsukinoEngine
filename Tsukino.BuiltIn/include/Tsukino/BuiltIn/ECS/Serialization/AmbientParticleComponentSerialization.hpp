//--------------------------------------------------------------
//! @file   AmbientParticleComponentSerialization.hpp
//! @brief  AmbientParticleComponentのcerealシリアライズ定義
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
#include <Tsukino/BuiltIn/ECS/Component/AmbientParticleComponent.hpp>

#include <Tsukino/Core/Math/Serialization/HlslppSerialization.hpp>

#include <cereal/cereal.hpp>
// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {

    //--------------------------------------------------------------
    //! AmbientParticleComponentのcerealシリアライズ定義
    //--------------------------------------------------------------
    template <class Archive>
    void save(Archive& archive, const AmbientParticleComponent& particle) {
        archive(cereal::make_nvp("enabled", particle.enabled),
                cereal::make_nvp("count", particle.count),
                cereal::make_nvp("volumeSize", particle.volumeSize),
                cereal::make_nvp("seed", particle.seed),
                cereal::make_nvp("color", particle.color),
                cereal::make_nvp("intensity", particle.intensity),
                cereal::make_nvp("minSize", particle.minSize),
                cereal::make_nvp("maxSize", particle.maxSize),
                cereal::make_nvp("minBrightness", particle.minBrightness),
                cereal::make_nvp("maxBrightness", particle.maxBrightness),
                cereal::make_nvp("twinkle", particle.twinkle),
                cereal::make_nvp("driftVelocity", particle.driftVelocity),
                cereal::make_nvp("swayAmplitude", particle.swayAmplitude),
                cereal::make_nvp("swayFrequency", particle.swayFrequency),
                cereal::make_nvp("minSpeedScale", particle.minSpeedScale),
                cereal::make_nvp("maxSpeedScale", particle.maxSpeedScale),
                cereal::make_nvp("edgeFadeStart", particle.edgeFadeStart),
                cereal::make_nvp("nearFadeDistance", particle.nearFadeDistance));
    }

    //--------------------------------------------------------------
    //! AmbientParticleComponentのcerealデシリアライズ定義
    //--------------------------------------------------------------
    template <class Archive>
    void load(Archive& archive, AmbientParticleComponent& particle) {
        archive(particle.enabled, particle.count, particle.volumeSize, particle.seed, particle.color, particle.intensity, particle.minSize,
                particle.maxSize, particle.minBrightness, particle.maxBrightness, particle.twinkle, particle.driftVelocity, particle.swayAmplitude,
                particle.swayFrequency, particle.minSpeedScale, particle.maxSpeedScale, particle.edgeFadeStart, particle.nearFadeDistance);
    }

}    // namespace Tsukino::BuiltIn::ECS
