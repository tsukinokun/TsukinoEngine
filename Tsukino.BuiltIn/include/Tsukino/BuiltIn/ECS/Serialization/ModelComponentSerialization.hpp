//--------------------------------------------------------------
//! @file   ModelComponentSerialization.hpp
//! @brief  ModelComponentのcerealシリアライズ定義
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
#include <Tsukino/BuiltIn/ECS/Component/ModelComponent.hpp>

#include <cereal/cereal.hpp>
// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {

    //--------------------------------------------------------------
    //! @brief  ModelComponentのcerealシリアライズ定義
    //--------------------------------------------------------------
    template <class Archive>
    void save(Archive& archive, const ModelComponent& model) {
        archive(cereal::make_nvp("modelHandle", model.modelHandle), cereal::make_nvp("visible", model.visible));
    }

    //--------------------------------------------------------------
    //! @brief  ModelComponentのcerealデシリアライズ定義
    //--------------------------------------------------------------
    template <class Archive>
    void load(Archive& archive, ModelComponent& model) {
        archive(model.modelHandle, model.visible);
    }

}    // namespace Tsukino::BuiltIn::ECS
