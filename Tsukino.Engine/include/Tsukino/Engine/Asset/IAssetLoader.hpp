//--------------------------------------------------------------
//! @file   IAssetLoader.hpp
//! @brief  アセットローダーのインターフェース
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
#include <Tsukino/Core/Path.hpp>
#include <Tsukino/Core/Memory.hpp>
#include <Tsukino/Engine/Asset/IAsset.hpp>
// namespace Tsukino::Asset
namespace Tsukino::Asset {
    //--------------------------------------------------------------
    //! @class  IAssetLoader
    //! @brief  アセットをロードするための抽象インターフェース
    //--------------------------------------------------------------
    class IAssetLoader {
    public:
        virtual ~IAssetLoader() = default;

        //--------------------------------------------------------------
        // このローダーが指定拡張子を扱えるか判定する
        //! @param extension [in] ".png" や ".shader" などの拡張子
        //! @return 対応している場合 true
        //--------------------------------------------------------------
        [[nodiscard]]
        virtual bool CanLoad(const std::string& extension) const = 0;

        //--------------------------------------------------------------
        // アセットをロードする
        //! @param path [in] ロード対象のパス
        //! @return ロードされたアセット（失敗時は nullptr）
        //--------------------------------------------------------------
        [[nodiscard]]
        virtual Tsukino::Core::Ref<IAsset> Load(const Tsukino::Core::Path& path) = 0;
    };

}    // namespace Tsukino::Asset
