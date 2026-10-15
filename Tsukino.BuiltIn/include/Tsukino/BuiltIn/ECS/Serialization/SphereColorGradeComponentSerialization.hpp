//--------------------------------------------------------------
//! @file   SphereColorGradeComponentSerialization.hpp
//! @brief  SphereColorGradeComponentのcerealシリアライズ定義
//--------------------------------------------------------------
#pragma once
#include <Tsukino/BuiltIn/ECS/Component/SphereColorGradeComponent.hpp>

#include <Tsukino/Core/Math/Serialization/HlslppSerialization.hpp>

#include <cereal/cereal.hpp>
// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {

    //--------------------------------------------------------------
    //! @brief  SphereColorGradeComponentのcerealシリアライズ定義
    //--------------------------------------------------------------
    template <class Archive>
    void save(Archive& archive, const SphereColorGradeComponent& grade) {
        archive(cereal::make_nvp("enabled", grade.enabled),
                cereal::make_nvp("center", grade.center),
                cereal::make_nvp("radius", grade.radius),
                cereal::make_nvp("edgeSoftness", grade.edgeSoftness),
                cereal::make_nvp("tint", grade.tint),
                cereal::make_nvp("strength", grade.strength),
                cereal::make_nvp("fogColor", grade.fogColor),
                cereal::make_nvp("fogDensity", grade.fogDensity),
                cereal::make_nvp("edgeColor", grade.edgeColor),
                cereal::make_nvp("edgeWidth", grade.edgeWidth));
    }

    //--------------------------------------------------------------
    //! @brief  SphereColorGradeComponentのcerealデシリアライズ定義
    //--------------------------------------------------------------
    template <class Archive>
    void load(Archive& archive, SphereColorGradeComponent& grade) {
        archive(grade.enabled, grade.center, grade.radius, grade.edgeSoftness, grade.tint, grade.strength, grade.fogColor, grade.fogDensity,
                grade.edgeColor, grade.edgeWidth);
    }

}    // namespace Tsukino::BuiltIn::ECS
