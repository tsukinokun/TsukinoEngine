//--------------------------------------------------------------
//! @file   EffectComponentSerialization.hpp
//! @brief  EffectComponent に対する cereal シリアライズ定義
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
#include <Tsukino/BuiltIn/ECS/Component/EffectComponent.hpp>

#include <Tsukino/Engine/Asset/Serialization/AssetHandleSerialization.hpp>

#include <cereal/cereal.hpp>
// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {

    //--------------------------------------------------------------
    //! @brief  EffectComponent のセーブ処理
    //--------------------------------------------------------------
    template <class Archive>
    void save(Archive& archive, const EffectComponent& effect) {
        archive(cereal::make_nvp("effectAsset", effect.effectAsset),
                cereal::make_nvp("playSpeed",  effect.playSpeed),
                cereal::make_nvp("looping",    effect.looping));
    }

    //--------------------------------------------------------------
    //! @brief  EffectComponent のロード処理
    //--------------------------------------------------------------
    template <class Archive>
    void load(Archive& archive, EffectComponent& effect) {
        archive(effect.effectAsset, effect.playSpeed, effect.looping);
        effect.handle  = -1;
        effect.stopped = false;
        effect.active  = false;
    }

}    // namespace Tsukino::BuiltIn::ECS
