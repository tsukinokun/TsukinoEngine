//--------------------------------------------------------------
//! @file   TerrainGenerationRequestComponentSerialization.hpp
//! @brief  TerrainGenerationRequestComponentのcerealシリアライズ定義
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
#include <Tsukino/BuiltIn/ECS/Component/TerrainGenerationRequestComponent.hpp>

#include <cereal/cereal.hpp>
// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {

    //--------------------------------------------------------------
    //! @brief  TerrainGenerationRequestComponentのcerealシリアライズ定義
    //--------------------------------------------------------------
    template <class Archive>
    void save(Archive& archive, const TerrainGenerationRequestComponent& req) {
        archive(cereal::make_nvp("amplitude", req.amplitude),
                cereal::make_nvp("noiseFrequency", req.noiseFrequency),
                cereal::make_nvp("seed", req.seed),
                cereal::make_nvp("noiseType", req.noiseType),
                cereal::make_nvp("collisionModelHandle", req.collisionModelHandle));
    }

    //--------------------------------------------------------------
    //! @brief  TerrainGenerationRequestComponentのcerealデシリアライズ定義
    //--------------------------------------------------------------
    template <class Archive>
    void load(Archive& archive, TerrainGenerationRequestComponent& req) {
        archive(req.amplitude, req.noiseFrequency, req.seed, req.noiseType, req.collisionModelHandle);
    }

}    // namespace Tsukino::BuiltIn::ECS
