//--------------------------------------------------------------
//! @file   AssetHandleGenerator.hpp
//! @brief  アセットハンドル生成ヘルパー
//! @detail アセットの識別キー（パス）から決定的なハンドルを生成します。
//--------------------------------------------------------------
#pragma once
#include <Tsukino/Engine/Asset/AssetHandle.hpp>
#include <Tsukino/Core/typedef.hpp>

#include <algorithm>
#include <cctype>
#include <string>
#include <string_view>
// 名前空間 : Tsukino::Asset
namespace Tsukino::Asset {
    //--------------------------------------------------------------
    //! @class  AssetHandleGenerator
    //! @brief  アセットハンドル生成クラス
    //--------------------------------------------------------------
    class AssetHandleGenerator {
    public:
        //--------------------------------------------------------------
        // 以前はstd::mt19937_64で乱数のハンドルを払い出していた。
        // 同じアセットでも起動のたびに値が変わるため、
        //   ・同じパスを二重にロードしても別アセットとして登録されてしまう
        //   ・ハンドルをJSONへ書き出しても次回起動時に解決できない
        // という2つの問題があった。識別キーのハッシュにすることで、
        // 同じキーからは常に同じハンドルが得られるようにしている。
        //
        // なおPrefabがアセットを参照する経路はAssetRef（パス文字列）であり、
        // ハンドルの値そのものをJSONへ書き出すことは無い
        //--------------------------------------------------------------

        //! アセットの識別キーを正規化します。
        //! @param  [in] rawKey 正規化する前のキー（アセットパスなど）
        //! @return 区切り文字と大文字小文字を揃えたキー
        [[nodiscard]]
        static std::string NormalizeKey(std::string_view rawKey) {
            std::string key(rawKey);
            // 区切り文字を'/'へ統一する
            std::replace(key.begin(), key.end(), '\\', '/');
            // Windowsのファイルシステムは大文字小文字を区別しないため小文字へ揃える
            std::transform(key.begin(), key.end(), key.begin(),
                           [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            return key;
        }

        //! 識別キーからアセットハンドルを生成します。
        //! @param  [in] rawKey ハンドルの元にする識別キー（アセットパスなど）
        //! @return 生成されたアセットハンドル
        [[nodiscard]]
        static AssetHandle GenerateFromKey(std::string_view rawKey) {
            const std::string key = NormalizeKey(rawKey);

            // FNV-1a（64bit）でキーをハッシュ化する
            u64 hash = 14695981039346656037ULL;    // FNV offset basis
            for(unsigned char c : key) {
                hash ^= static_cast<u64>(c);
                hash *= 1099511628211ULL;          // FNV prime
            }

            // 0はAssetHandle::Invalid()と衝突するため、その場合だけ別の値へ寄せる
            if(hash == 0)
                hash = 1469598103934665603ULL;

            return AssetHandle(hash);
        }
    };
}    // namespace Tsukino::Asset
