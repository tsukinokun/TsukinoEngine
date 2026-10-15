//-------------------------------------------------------------
//! @file   UIRectComponentSerialization.hpp
//! @brief  UIRectComponentのcerealシリアライズ定義
//-------------------------------------------------------------
#pragma once
#include <Tsukino/BuiltIn/ECS/Component/UIRectComponent.hpp>

#include <Tsukino/Core/Math/Serialization/HlslppSerialization.hpp>

#include <cereal/cereal.hpp>

// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {

    //--------------------------------------------------------------
    //! @brief  UIRectComponentのcerealシリアライズ定義
    //--------------------------------------------------------------
    template <class Archive>
    void save(Archive& archive, const UIRectComponent& rect) {
        archive(cereal::make_nvp("size", rect.size), cereal::make_nvp("pivot", rect.pivot));
    }

    //--------------------------------------------------------------
    //! @brief  UIRectComponentのcerealデシリアライズ定義
    //--------------------------------------------------------------
    template <class Archive>
    void load(Archive& archive, UIRectComponent& rect) {
        archive(cereal::make_nvp("size", rect.size));

        // pivot は省略できる（無ければ中心）
        try {
            archive(cereal::make_nvp("pivot", rect.pivot));
        } catch(const cereal::Exception&) {
        }
    }

}    // namespace Tsukino::BuiltIn::ECS
