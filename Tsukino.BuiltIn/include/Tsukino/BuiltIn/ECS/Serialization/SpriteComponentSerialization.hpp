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
                cereal::make_nvp("sortOrder", sprite.sortOrder));
    }

    //--------------------------------------------------------------
    //! @brief  SpriteComponentのcerealデシリアライズ定義
    //--------------------------------------------------------------
    template <class Archive>
    void load(Archive& archive, SpriteComponent& sprite) {
        archive(sprite.textureHandle, sprite.blendMode, sprite.space, sprite.tintColor, sprite.uvRect, sprite.sortOrder);
    }

}    // namespace Tsukino::BuiltIn::ECS
