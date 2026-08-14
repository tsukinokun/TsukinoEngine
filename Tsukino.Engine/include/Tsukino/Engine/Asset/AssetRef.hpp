//--------------------------------------------------------------------
// @file   AssetRef.hpp
// @brief  パスベースのアセット間接参照
// @author 山﨑愛
//--------------------------------------------------------------------
#pragma once
#include <Tsukino/Engine/Asset/AssetHandle.hpp>
#include <Tsukino/Core/typedef.hpp>

#include <cereal/cereal.hpp>

#include <string>

// 名前空間 : Tsukino::Asset
namespace Tsukino::Asset {

    //--------------------------------------------------------------------
    //! @struct AssetRef
    //! @brief  アセットをパスで間接参照する型。
    //!
    //!         AssetHandle はプロセス内限定の値でありJSONへ直接書き出せないため、
    //!         代わりにこの型がパス文字列を保持し、PrefabFactoryがInstantiate時に
    //!         AssetManager::Loadで解決してhandleへ書き込む（EntityRefと対になる設計）。
    //!
    //!         AssetHandleとの暗黙変換・主要メソッドの転送を持つ「透過的なラッパー」
    //!         として設計しているため、既存のAssetHandleを直接扱っているコードは
    //!         変更なしでそのまま動作する。
    //--------------------------------------------------------------------
    struct AssetRef {
        AssetHandle handle;    // 解決済みハンドル（未解決ならInvalid）
        std::string path;      // JSON上のアセットパス

        AssetRef() = default;
        AssetRef(AssetHandle h)
            : handle(h) {}

        operator AssetHandle() const { return handle; }

        [[nodiscard]]
        bool IsValid() const {
            return handle.IsValid();
        }

        [[nodiscard]]
        u64 Value() const {
            return handle.Value();
        }

        bool operator==(const AssetRef& other) const { return handle == other.handle; }
        bool operator!=(const AssetRef& other) const { return handle != other.handle; }
        bool operator==(AssetHandle other) const { return handle == other; }
        bool operator!=(AssetHandle other) const { return handle != other; }
    };

    //--------------------------------------------------------------------
    //! @brief  cereal用：保存処理（pathのみを書き出す）
    //--------------------------------------------------------------------
    template <class Archive>
    void save(Archive& archive, const AssetRef& ref) {
        archive(ref.path);
    }

    //--------------------------------------------------------------------
    //! @brief  cereal用：読み込み処理（pathのみを読み込む。handleの解決はAssetRefResolverArchiveが行う）
    //--------------------------------------------------------------------
    template <class Archive>
    void load(Archive& archive, AssetRef& ref) {
        archive(ref.path);
        ref.handle = AssetHandle::Invalid();
    }

}    // namespace Tsukino::Asset
