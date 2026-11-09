//----------------------------------------------------------------------------
//! @file   TextureImportRules.cpp
//! @brief  テクスチャのインポートと読み込みの規則の実装
//----------------------------------------------------------------------------
#include <Tsukino/Engine/Asset/Texture/TextureImportRules.hpp>

#include <algorithm>

// 名前空間 : Tsukino::Asset
namespace Tsukino::Asset {

    namespace {
        //--------------------------------------------------------------
        //! パスの区切りを / に揃えます。
        //! @param  [in] text パス
        //! @return 区切りを揃えたパス
        //--------------------------------------------------------------
        std::string NormalizeSeparators(std::string text) {
            std::replace(text.begin(), text.end(), '\\', '/');
            return text;
        }
    }    // namespace

    //------------------------------------------------------------------------
    //! 規則を追加します。
    //------------------------------------------------------------------------
    void TextureImportRules::Add(const std::string& pathPrefix, const TextureImportRule& rule) {
        m_rules.emplace_back(NormalizeSeparators(pathPrefix), rule);
    }

    //------------------------------------------------------------------------
    //! パスに当てはまる規則を探します。
    //------------------------------------------------------------------------
    const TextureImportRule* TextureImportRules::Find(const Tsukino::Core::Path& path) const {
        if(m_rules.empty())
            return nullptr;

        // ソースは相対パス、キャッシュは Cache/ の下の絶対パスで来るので、前方一致ではなく含むかどうかで見る
        const std::string normalized = NormalizeSeparators(path.string());
        for(const auto& [prefix, rule] : m_rules) {
            if(normalized.find(prefix) != std::string::npos)
                return &rule;
        }
        return nullptr;
    }

}    // namespace Tsukino::Asset
