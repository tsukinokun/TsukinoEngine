//--------------------------------------------------------------
//! @file   ModelComponentSerialization.hpp
//! @brief  ModelComponentのcerealシリアライズ定義
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
#include <Tsukino/BuiltIn/ECS/Component/ModelComponent.hpp>

#include <cereal/cereal.hpp>
#include <cereal/types/vector.hpp>
// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {

    //--------------------------------------------------------------
    //! @brief  ModelComponentのcerealシリアライズ定義
    //--------------------------------------------------------------
    template <class Archive>
    void save(Archive& archive, const ModelComponent& model) {
        archive(cereal::make_nvp("modelHandle", model.modelHandle), cereal::make_nvp("visible", model.visible),
                cereal::make_nvp("opacity", model.opacity), cereal::make_nvp("doubleSided", model.doubleSided),
                cereal::make_nvp("materials", model.materials));
    }

    //--------------------------------------------------------------
    //! @brief  ModelComponentのcerealデシリアライズ定義
    //--------------------------------------------------------------
    template <class Archive>
    void load(Archive& archive, ModelComponent& model) {
        archive(model.modelHandle, model.visible, model.opacity);

        // doubleSided は後から足した項目。古いPrefab JSONには無いので、無ければ既定値（片面）のまま
        try {
            archive(cereal::make_nvp("doubleSided", model.doubleSided));
        } catch(const cereal::Exception&) {
        }

        // materials も後から足した項目（JSON ではマテリアルのパスの配列）。無ければ差し替え無し
        try {
            archive(cereal::make_nvp("materials", model.materials));
        } catch(const cereal::Exception&) {
        }
    }

}    // namespace Tsukino::BuiltIn::ECS
