//-------------------------------------------------------------
//! @file   FontComponentSerialization.hpp
//! @brief  FontComponentのcerealシリアライズ定義
//! @note   text は UTF-8 の文字列で、任意キー（無ければ空のまま。呼び出し側が Instantiate 後にコードで設定してもよい）。
//!         fontHandleはAssetRefなので、SpriteComponentのtextureHandleと同じくパス文字列で読み書きできる
//!         （任意キー。無ければ未設定＝既定フォント）。
//-------------------------------------------------------------
#pragma once
#include <Tsukino/BuiltIn/ECS/Component/FontComponent.hpp>

#include <Tsukino/Core/IO/CerealOptional.hpp>
#include <Tsukino/Core/Math/Serialization/HlslppSerialization.hpp>

#include <cereal/cereal.hpp>
#include <cereal/types/string.hpp>

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
                cereal::make_nvp("sortOrder", font.sortOrder),
                cereal::make_nvp("maxWidth", font.maxWidth),
                cereal::make_nvp("fontHandle", font.fontHandle),
                cereal::make_nvp("scale", font.scale),
                cereal::make_nvp("text", font.text));
    }

    //--------------------------------------------------------------
    //! @brief  FontComponentのcerealデシリアライズ定義
    //--------------------------------------------------------------
    template <class Archive>
    void load(Archive& archive, FontComponent& font) {
        archive(font.color, font.origin, font.horizontalAlign, font.verticalAlign, font.outlineColor, font.outlineWidth, font.sortOrder);

        // maxWidth・fontHandle・scale・text は後から足した項目。古いPrefab JSONには無いので、無ければ既定値のまま
        // （maxWidth＝制限なし、fontHandle＝未設定で既定フォント、scale＝1、text＝空）
        Tsukino::IO::LoadOptional(archive, "maxWidth", font.maxWidth);
        Tsukino::IO::LoadOptional(archive, "fontHandle", font.fontHandle);
        Tsukino::IO::LoadOptional(archive, "scale", font.scale);
        Tsukino::IO::LoadOptional(archive, "text", font.text);
    }

}    // namespace Tsukino::BuiltIn::ECS
