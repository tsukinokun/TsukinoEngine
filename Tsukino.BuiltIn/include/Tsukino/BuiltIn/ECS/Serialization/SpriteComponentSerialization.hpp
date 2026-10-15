//-------------------------------------------------------------
//! @file   SpriteComponentSerialization.hpp
//! @brief  SpriteComponentのcerealシリアライズ定義
//-------------------------------------------------------------
#pragma once
#include <Tsukino/BuiltIn/ECS/Component/SpriteComponent.hpp>

#include <Tsukino/Core/Math/Serialization/HlslppSerialization.hpp>

#include <cereal/cereal.hpp>

// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {

    //--------------------------------------------------------------
    //! @brief  SpriteComponentのcerealシリアライズ定義
    //--------------------------------------------------------------
    template <class Archive>
    void save(Archive& archive, const SpriteComponent& sprite) {
        archive(cereal::make_nvp("textureHandle", sprite.textureHandle),
                cereal::make_nvp("blendMode", sprite.blendMode),
                cereal::make_nvp("space", sprite.space),
                cereal::make_nvp("tintColor", sprite.tintColor),
                cereal::make_nvp("uvRect", sprite.uvRect),
                cereal::make_nvp("sortOrder", sprite.sortOrder),
                cereal::make_nvp("fillMode", sprite.fillMode),
                cereal::make_nvp("fillAmount", sprite.fillAmount),
                cereal::make_nvp("fillStartAngle", sprite.fillStartAngle),
                cereal::make_nvp("fillClockwise", sprite.fillClockwise),
                cereal::make_nvp("sliceBorder", sprite.sliceBorder),
                cereal::make_nvp("sliceScale", sprite.sliceScale),
                cereal::make_nvp("raycastTarget", sprite.raycastTarget));
    }

    //--------------------------------------------------------------
    //! @brief  SpriteComponentのcerealデシリアライズ定義
    //--------------------------------------------------------------
    template <class Archive>
    void load(Archive& archive, SpriteComponent& sprite) {
        archive(sprite.textureHandle, sprite.blendMode, sprite.space, sprite.tintColor, sprite.uvRect, sprite.sortOrder);

        // fill*・slice*・raycastTarget は後から足した項目。古いPrefab JSONには無いので、無ければ既定値（画像全体を描く・当たりを受ける）のまま
        auto loadOptional = [&archive](const char* name, auto& value) {
            try {
                archive(cereal::make_nvp(name, value));
            } catch(const cereal::Exception&) {
            }
        };
        loadOptional("fillMode", sprite.fillMode);
        loadOptional("fillAmount", sprite.fillAmount);
        loadOptional("fillStartAngle", sprite.fillStartAngle);
        loadOptional("fillClockwise", sprite.fillClockwise);
        loadOptional("sliceBorder", sprite.sliceBorder);
        loadOptional("sliceScale", sprite.sliceScale);
        loadOptional("raycastTarget", sprite.raycastTarget);
    }

}    // namespace Tsukino::BuiltIn::ECS
