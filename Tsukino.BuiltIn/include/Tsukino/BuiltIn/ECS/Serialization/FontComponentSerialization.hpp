//-------------------------------------------------------------
//! @file   FontComponentSerialization.hpp
//! @brief  FontComponentのcerealシリアライズ定義
//! @note   textはstd::wstringでありcerealのJSONバックエンドでの対応が未実証なため、
//!         fontHandle（AssetHandle、プロセス内限定でシリアライズ不可）とあわせて対象外とする。
//!         どちらもアタッチ時のデフォルト値のまま残り、必要なら呼び出し側が
//!         Instantiate後にコードで設定する。
//-------------------------------------------------------------
#pragma once
#include <Tsukino/BuiltIn/ECS/Component/FontComponent.hpp>

#include <Tsukino/Core/Math/Serialization/HlslppSerialization.hpp>

#include <cereal/cereal.hpp>

// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {

    //--------------------------------------------------------------
    //! @brief  FontComponentのcerealシリアライズ定義
    //--------------------------------------------------------------
    template <class Archive>
    void save(Archive& archive, const FontComponent& font) {
        archive(cereal::make_nvp("color", font.color),
                cereal::make_nvp("origin", font.origin),
                cereal::make_nvp("horizontalAlign", font.horizontalAlign),
                cereal::make_nvp("verticalAlign", font.verticalAlign),
                cereal::make_nvp("outlineColor", font.outlineColor),
                cereal::make_nvp("outlineWidth", font.outlineWidth),
                cereal::make_nvp("sortOrder", font.sortOrder));
    }

    //--------------------------------------------------------------
    //! @brief  FontComponentのcerealデシリアライズ定義
    //--------------------------------------------------------------
    template <class Archive>
    void load(Archive& archive, FontComponent& font) {
        archive(font.color, font.origin, font.horizontalAlign, font.verticalAlign, font.outlineColor, font.outlineWidth, font.sortOrder);
    }

}    // namespace Tsukino::BuiltIn::ECS
