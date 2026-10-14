//--------------------------------------------------------------
//! @file   OcclusionCutoutComponentSerialization.hpp
//! @brief  OcclusionCutoutComponentのcerealシリアライズ定義
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
#include <Tsukino/BuiltIn/ECS/Component/OcclusionCutoutComponent.hpp>

#include <cereal/cereal.hpp>
// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {
    //--------------------------------------------------------------
    //! @brief  OcclusionCutoutComponentのcerealシリアライズ定義
    //--------------------------------------------------------------
    template <class Archive>
    void save(Archive& archive, const OcclusionCutoutComponent& cutout) {
        archive(cereal::make_nvp("enabled", cutout.enabled));
    }

    //--------------------------------------------------------------
    //! @brief  OcclusionCutoutComponentのcerealデシリアライズ定義
    //--------------------------------------------------------------
    template <class Archive>
    void load(Archive& archive, OcclusionCutoutComponent& cutout) {
        archive(cutout.enabled);
    }

}    // namespace Tsukino::BuiltIn::ECS
