//-------------------------------------------------------------
//! @file   FontComponentSerialization.hpp
//! @brief  FontComponentのcerealシリアライズ定義
//! @note   textはstd::wstringでありcerealのJSONバックエンドでの対応が未実証なため対象外とする。
//!         アタッチ時のデフォルト値（空）のまま残り、必要なら呼び出し側がInstantiate後にコードで設定する。
//!         fontHandleはAssetRefなので、SpriteComponentのtextureHandleと同じくパス文字列で読み書きできる
//!         （任意キー。無ければ未設定＝既定フォント）。
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
                cereal::make_nvp("sortOrder", font.sortOrder),
                cereal::make_nvp("maxWidth", font.maxWidth),
                cereal::make_nvp("fontHandle", font.fontHandle));
    }

    //--------------------------------------------------------------
    //! @brief  FontComponentのcerealデシリアライズ定義
    //--------------------------------------------------------------
    template <class Archive>
    void load(Archive& archive, FontComponent& font) {
        archive(font.color, font.origin, font.horizontalAlign, font.verticalAlign, font.outlineColor, font.outlineWidth, font.sortOrder);

        // maxWidth・fontHandle は後から足した項目。古いPrefab JSONには無いので、無ければ既定値のまま
        // （maxWidth＝制限なし、fontHandle＝未設定で既定フォント）
        auto loadOptional = [&archive](const char* name, auto& value) {
            try {
                archive(cereal::make_nvp(name, value));
            } catch(const cereal::Exception&) {
            }
        };
        loadOptional("maxWidth", font.maxWidth);
        loadOptional("fontHandle", font.fontHandle);
    }

}    // namespace Tsukino::BuiltIn::ECS
