//----------------------------------------------------------------------------
//! @file   TextureImportRules.hpp
//! @brief  テクスチャのインポートと読み込みの規則
//! @detail パスごとに、ブロック圧縮するか・ミップマップを先に作るか・読み込み時に上位ミップを
//!         捨ててよいかを決めます。規則に当てはまらないテクスチャの扱いは変わりません。
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/Core/Path.hpp>
#include <Tsukino/Core/typedef.hpp>

#include <Tsukino/GraphicsCommon/Texture/TextureFormat.hpp>

#include <string>
#include <utility>
#include <vector>

// 名前空間 : Tsukino::Asset
namespace Tsukino::Asset {

    //------------------------------------------------------------------------
    //! テクスチャ1種類ぶんのインポートの規則です。
    //------------------------------------------------------------------------
    struct TextureImportRule {
        //! 保存するブロック圧縮フォーマット（TextureFormat::Unknown で圧縮しない）。
        //! 色空間（sRGB / 線形）はファイル名の慣習からインポーターが決めるので、どちらで書いてもよい
        Tsukino::GraphicsCommon::TextureFormat compressedFormat = Tsukino::GraphicsCommon::TextureFormat::Unknown;
        bool                                   generateMips     = false;    //!< インポート時にミップマップを作って保存するか
        bool                                   allowMipSkip     = false;    //!< 読み込み時に上位ミップを捨ててよいか（TextureImportRules::SetMipSkip）
    };

    //------------------------------------------------------------------------
    //! テクスチャのインポートと読み込みの規則の一覧です。
    //! @note  AssetManager が1つだけ持つ。読み込みのスレッドが読むので、
    //!        変更はスレッドを立てる前（最初のアセットを読む前）に済ませること
    //------------------------------------------------------------------------
    class TextureImportRules {
    public:

        //! 規則を追加します。
        //! @param  [in] pathPrefix 対象のフォルダ（パスの中にこの文字列を含むテクスチャに効く）
        //! @param  [in] rule       規則
        void Add(const std::string& pathPrefix, const TextureImportRule& rule);

        //! パスに当てはまる規則を探します。
        //! @param  [in] path テクスチャのパス（ソースでもキャッシュでもよい）
        //! @return 規則へのポインタ。当てはまらなければ nullptr
        [[nodiscard]]
        const TextureImportRule* Find(const Tsukino::Core::Path& path) const;

        //! 読み込み時に捨てる上位ミップの段数を設定します。
        //! @param  [in] mipSkip 段数（allowMipSkip の規則に当てはまるテクスチャにだけ効く）
        void SetMipSkip(u32 mipSkip) noexcept {
            m_mipSkip = mipSkip;
        }

        //! 読み込み時に捨てる上位ミップの段数を取得します。
        //! @return 段数
        [[nodiscard]]
        u32 GetMipSkip() const noexcept {
            return m_mipSkip;
        }

    private:
        std::vector<std::pair<std::string, TextureImportRule>> m_rules;          // 対象のフォルダと規則の組
        u32                                                    m_mipSkip = 0;    // 読み込み時に捨てる上位ミップの段数
    };

}    // namespace Tsukino::Asset
