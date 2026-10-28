//----------------------------------------------------------------------------
//! @file   BurstParamsSerialization.hpp
//! @brief  BurstParams（粒子の撒き方）の cereal シリアライズ定義
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/BuiltIn/ECS/Particle/BurstEmitter.hpp>

#include <Tsukino/Core/IO/CerealOptional.hpp>
#include <Tsukino/Core/Math/Serialization/HlslppSerialization.hpp>

#include <cereal/cereal.hpp>

// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {

    //! BurstParams を書き出します。
    //! @param  [in] archive 書き込み先のアーカイブ
    //! @param  [in] params  書き出す撒き方
    template <class Archive>
    void save(Archive& archive, const BurstParams& params) {
        archive(cereal::make_nvp("count", params.count), cereal::make_nvp("speedMin", params.speedMin), cereal::make_nvp("speedMax", params.speedMax),
                cereal::make_nvp("upwardBias", params.upwardBias), cereal::make_nvp("spread", params.spread),
                cereal::make_nvp("lifeMin", params.lifeMin), cereal::make_nvp("lifeMax", params.lifeMax), cereal::make_nvp("sizeMin", params.sizeMin),
                cereal::make_nvp("sizeMax", params.sizeMax), cereal::make_nvp("endSizeScale", params.endSizeScale),
                cereal::make_nvp("gravity", params.gravity), cereal::make_nvp("drag", params.drag), cereal::make_nvp("radius", params.radius),
                cereal::make_nvp("delayMax", params.delayMax), cereal::make_nvp("colorA", params.colorA), cereal::make_nvp("colorB", params.colorB),
                cereal::make_nvp("endAlpha", params.endAlpha), cereal::make_nvp("additive", params.additive),
                cereal::make_nvp("texturePixels", params.texturePixels));
    }

    //! BurstParams を読み込みます。書かれていない項目は既定値のまま残します。
    //! @param  [in]     archive 読み込み元のアーカイブ
    //! @param  [in,out] params  読み込み先の撒き方
    template <class Archive>
    void load(Archive& archive, BurstParams& params) {
        using Tsukino::IO::LoadOptional;
        LoadOptional(archive, "count", params.count);
        LoadOptional(archive, "speedMin", params.speedMin);
        LoadOptional(archive, "speedMax", params.speedMax);
        LoadOptional(archive, "upwardBias", params.upwardBias);
        LoadOptional(archive, "spread", params.spread);
        LoadOptional(archive, "lifeMin", params.lifeMin);
        LoadOptional(archive, "lifeMax", params.lifeMax);
        LoadOptional(archive, "sizeMin", params.sizeMin);
        LoadOptional(archive, "sizeMax", params.sizeMax);
        LoadOptional(archive, "endSizeScale", params.endSizeScale);
        LoadOptional(archive, "gravity", params.gravity);
        LoadOptional(archive, "drag", params.drag);
        LoadOptional(archive, "radius", params.radius);
        LoadOptional(archive, "delayMax", params.delayMax);
        LoadOptional(archive, "colorA", params.colorA);
        LoadOptional(archive, "colorB", params.colorB);
        LoadOptional(archive, "endAlpha", params.endAlpha);
        LoadOptional(archive, "additive", params.additive);
        LoadOptional(archive, "texturePixels", params.texturePixels);
    }

}    // namespace Tsukino::BuiltIn::ECS
