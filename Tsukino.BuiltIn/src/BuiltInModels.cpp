//--------------------------------------------------------------
//! @file   BuiltInModels.cpp
//! @brief  ビルトインモデル（立方体・平面・球）集約クラスの実装
//--------------------------------------------------------------
#include <Tsukino/BuiltIn/BuiltInModels.hpp>

#include <Tsukino/Engine/Asset/AssetManager.hpp>

#include <Tsukino/Core/IO/FileSystem.hpp>
#include <Tsukino/Core/Path.hpp>
// 名前空間 : Tsukino::BuiltIn
namespace Tsukino::BuiltIn {
    //--------------------------------------------------------------
    //! @brief ビルトインモデルを読み込みます。
    //--------------------------------------------------------------
    void BuiltInModels::Initialize(Tsukino::Asset::AssetManager* assetManager) {
        // エンジン自身が所有するアセットのため、取り込み側リポジトリの
        // GetAssetRootPath()ではなくGetEngineAssetRootPath()から解決する（BuiltInFonts と同じ）
        const Tsukino::Core::Path root = Tsukino::IO::FileSystem::GetEngineAssetRootPath() / "Tsukino.BuiltIn/Assets/Models";
        cube                           = assetManager->Load(root / "Cube.obj");
        plane                          = assetManager->Load(root / "Plane.obj");
        sphere                         = assetManager->Load(root / "Sphere.obj");
    }
}    // namespace Tsukino::BuiltIn
