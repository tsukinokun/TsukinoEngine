//--------------------------------------------------------------
//! @file   BloomComponentSerialization.hpp
//! @brief  BloomComponentのcerealシリアライズ定義
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
#include <Tsukino/BuiltIn/ECS/Component/BloomComponent.hpp>

#include <Tsukino/Core/Math/Serialization/HlslppSerialization.hpp>

#include <cereal/cereal.hpp>
// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {
    //--------------------------------------------------------------
    //! @brief  BloomComponentのcerealシリアライズ定義
    //--------------------------------------------------------------
    template <class Archive>
    void save(Archive& archive, const BloomComponent& bloom) {
        archive(cereal::make_nvp("enabled", bloom.enabled),
                cereal::make_nvp("threshold", bloom.threshold),
                cereal::make_nvp("softKnee", bloom.softKnee),
                cereal::make_nvp("intensity", bloom.intensity),
                cereal::make_nvp("radius", bloom.radius),
                cereal::make_nvp("tint", bloom.tint));
    }

    //--------------------------------------------------------------
    //! @brief  BloomComponentのcerealデシリアライズ定義
    //--------------------------------------------------------------
    template <class Archive>
    void load(Archive& archive, BloomComponent& bloom) {
        archive(bloom.enabled, bloom.threshold, bloom.softKnee, bloom.intensity, bloom.radius, bloom.tint);
    }

}    // namespace Tsukino::BuiltIn::ECS
