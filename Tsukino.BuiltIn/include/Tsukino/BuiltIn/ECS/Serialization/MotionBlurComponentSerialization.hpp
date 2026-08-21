//--------------------------------------------------------------
//! @file   MotionBlurComponentSerialization.hpp
//! @brief  MotionBlurComponentに対するcerealシリアライズ定義
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
#include <Tsukino/BuiltIn/ECS/Component/MotionBlurComponent.hpp>

#include <cereal/cereal.hpp>

// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {
    //--------------------------------------------------------------
    //! @brief  MotionBlurComponentのcerealシリアライズ定義
    //--------------------------------------------------------------
    template <class Archive>
    void save(Archive& archive, const MotionBlurComponent& blur) {
        // 保存したい初期パラメータだけをアーカイブに流し込む
        archive(cereal::make_nvp("enabled", blur.enabled),
                cereal::make_nvp("strength", blur.strength),
                cereal::make_nvp("maxBlurRadius", blur.maxBlurRadius),
                cereal::make_nvp("sampleCount", blur.sampleCount),
                cereal::make_nvp("targetFps", blur.targetFps));
    }

    //--------------------------------------------------------------
    //! @brief  MotionBlurComponentのcerealデシリアライズ定義
    //--------------------------------------------------------------
    template <class Archive>
    void load(Archive& archive, MotionBlurComponent& blur) {
        archive(blur.enabled,
                blur.strength,
                blur.maxBlurRadius,
                blur.sampleCount,
                blur.targetFps);
    }

}    // namespace Tsukino::BuiltIn::ECS
