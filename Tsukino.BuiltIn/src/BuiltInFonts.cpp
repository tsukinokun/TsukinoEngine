//--------------------------------------------------------------
//! @file   BuiltInFonts.cpp
//! @brief  ビルトインフォント集約クラスの実装
//! @author 山﨑愛
//--------------------------------------------------------------
#include <Tsukino/BuiltIn/BuiltInFonts.hpp>

#include <Tsukino/Engine/Asset/AssetManager.hpp>

#include <Tsukino/Core/Path.hpp>
#include <Tsukino/Core/IO/FileSystem.hpp>
// 名前空間 : Tsukino::BuiltIn
namespace Tsukino::BuiltIn {
    //--------------------------------------------------------------
    //! @brief ビルトインフォントの初期化関数
    //--------------------------------------------------------------
    void BuiltInFonts::Initialize(Tsukino::Asset::AssetManager* assetManager) {
        // エンジン自身が所有するアセットのため、取り込み側リポジトリの
        // GetAssetRootPath()ではなくGetEngineAssetRootPath()から解決する
        defaultFont = assetManager->Load(Tsukino::IO::FileSystem::GetEngineAssetRootPath() / "Tsukino.BuiltIn/Assets/Fonts/Arial.font");
    }
}    // namespace Tsukino::BuiltIn
