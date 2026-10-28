//----------------------------------------------------------------------------
//! @file   UIButtonSoundComponentSerialization.hpp
//! @brief  UIButtonSoundComponent の cereal シリアライズ定義
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/BuiltIn/ECS/Component/UIButtonSoundComponent.hpp>

#include <Tsukino/Core/IO/CerealOptional.hpp>

#include <cereal/cereal.hpp>

// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {

    //! UIButtonSoundComponent を書き出します。
    //! @param  [in] archive 書き込み先のアーカイブ
    //! @param  [in] sound   書き出すコンポーネント
    template <class Archive>
    void save(Archive& archive, const UIButtonSoundComponent& sound) {
        archive(cereal::make_nvp("hoverSound", sound.hoverSound),
                cereal::make_nvp("clickSound", sound.clickSound),
                cereal::make_nvp("hoverVolume", sound.hoverVolume),
                cereal::make_nvp("clickVolume", sound.clickVolume),
                cereal::make_nvp("bus", sound.bus));
    }

    //! UIButtonSoundComponent を読み込みます。書かれていない項目は既定値のまま残します。
    //! @param  [in]     archive 読み込み元のアーカイブ
    //! @param  [in,out] sound   読み込み先のコンポーネント
    template <class Archive>
    void load(Archive& archive, UIButtonSoundComponent& sound) {
        Tsukino::IO::LoadOptional(archive, "hoverSound", sound.hoverSound);
        Tsukino::IO::LoadOptional(archive, "clickSound", sound.clickSound);
        Tsukino::IO::LoadOptional(archive, "hoverVolume", sound.hoverVolume);
        Tsukino::IO::LoadOptional(archive, "clickVolume", sound.clickVolume);
        Tsukino::IO::LoadOptional(archive, "bus", sound.bus);
    }

}    // namespace Tsukino::BuiltIn::ECS
