//--------------------------------------------------------------
//! @file   DirectionalLightComponentSerialization.hpp
//! @brief  DirectionalLightComponentのcerealシリアライズ定義
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
#include <Tsukino/BuiltIn/ECS/Component/DirectionalLightComponent.hpp>

#include <Tsukino/Core/Math/Serialization/HlslppSerialization.hpp>
#include <Tsukino/Core/IO/CerealOptional.hpp>

#include <Tsukino/Renderer/ShadowSettings.hpp>

#include <cereal/cereal.hpp>

// 名前空間 : Tsukino::Renderer
// cereal は引数の型の名前空間から save / load を探すので、ShadowSettings と同じ名前空間に置く
namespace Tsukino::Renderer {

    //--------------------------------------------------------------
    //! @brief  ShadowSettingsのcerealシリアライズ定義
    //--------------------------------------------------------------
    template <class Archive>
    void save(Archive& archive, const ShadowSettings& shadow) {
        archive(cereal::make_nvp("distance", shadow.distance),
                cereal::make_nvp("splitStart", shadow.splitStart),
                cereal::make_nvp("splitLambda", shadow.splitLambda),
                cereal::make_nvp("mapSize", shadow.mapSize),
                cereal::make_nvp("casterMargin", shadow.casterMargin));
    }

    //--------------------------------------------------------------
    //! @brief  ShadowSettingsのcerealデシリアライズ定義
    //! @note   どの項目も省略でき、省いた項目は ShadowSettings の既定値のまま残る
    //--------------------------------------------------------------
    template <class Archive>
    void load(Archive& archive, ShadowSettings& shadow) {
        using Tsukino::IO::LoadOptional;
        LoadOptional(archive, "distance", shadow.distance);
        LoadOptional(archive, "splitStart", shadow.splitStart);
        LoadOptional(archive, "splitLambda", shadow.splitLambda);
        LoadOptional(archive, "mapSize", shadow.mapSize);
        LoadOptional(archive, "casterMargin", shadow.casterMargin);
    }

}    // namespace Tsukino::Renderer

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
                cereal::make_nvp("castShadow", light.castShadow),
                cereal::make_nvp("shadow", light.shadow));
    }

    //--------------------------------------------------------------
    //! @brief  DirectionalLightComponentのcerealデシリアライズ定義
    //! @note   shadow は後から足した項目なので省略できる（無ければ既定値）
    //--------------------------------------------------------------
    template <class Archive>
    void load(Archive& archive, DirectionalLightComponent& light) {
        archive(light.direction, light.color, light.intensity, light.castShadow);
        Tsukino::IO::LoadOptional(archive, "shadow", light.shadow);
    }

}    // namespace Tsukino::BuiltIn::ECS
