//--------------------------------------------------------------
//! @file   AssetManager.hpp
//! @brief  アセット管理クラスの宣言
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
#include <Tsukino/Engine/Asset/AssetType.hpp>
#include <Tsukino/Engine/Asset/AssetHandle.hpp>
#include <Tsukino/Engine/Asset/AssetMap.hpp>
#include <Tsukino/Engine/Asset/IAssetImporter.hpp>

#include <Tsukino/Core/Path.hpp>
#include <Tsukino/Core/Memory.hpp>

#include <unordered_map>
#include <vector>
#include <memory>
// 名前空間 : Tsukino::Asset
namespace Tsukino::Asset {
    class IAsset;          // 前方宣言
    class IAssetLoader;    // 前方宣言
    //--------------------------------------------------------------
    //! @class  AssetManager
    //! @brief  アセットのロード、管理を行うクラス
    //--------------------------------------------------------------
    class AssetManager {
    public:
        //--------------------------------------------------------------
        // デフォルトコンストラクタ
        //--------------------------------------------------------------
        AssetManager() = default;

        //--------------------------------------------------------------
        // デストラクタ
        //--------------------------------------------------------------
        ~AssetManager();

        //--------------------------------------------------------------
        // AssetManager を初期化する関数
        //--------------------------------------------------------------
        void Initialize();

        //--------------------------------------------------------------
        // アセットをロードする関数
        //! @param  path [in] ロードするアセットのパス
        //! @return ロードしたアセットのハンドル
        //--------------------------------------------------------------
        [[nodiscard]]
        AssetHandle Load(const Tsukino::Core::Path& path);

        //--------------------------------------------------------------
        // ハンドルからアセットを取得する関数
        //! @param  handle [in] 取得するアセットのハンドル
        //! @return 取得したアセットのshared_ptr
        //--------------------------------------------------------------
        [[nodiscard]]
        Tsukino::Core::Ref<IAsset> Get(AssetHandle handle);

        //--------------------------------------------------------------
        // ハンドルから、アセットが存在するか確認する関数
        //! @param  handle [in] 確認するアセットのハンドル
        //! @return 存在する場合は true、存在しない場合は false
        //--------------------------------------------------------------
        [[nodiscard]]
        bool Exists(AssetHandle handle);

        //--------------------------------------------------------------
        // インポーターを登録する関数
        //! @param  type     [in] 登録するインポーターが対応するアセットの種類
        //! @param  importer [in] 登録するインポーターのshared_ptr
        //--------------------------------------------------------------
        void RegisterImporter(AssetType type, Tsukino::Core::Ref<IAssetImporter> importer);

    private:
        //--------------------------------------------------------------
        // ローダーを登録する関数
        //! @param  loader [in] 登録するローダーのshared_ptr
        //--------------------------------------------------------------
        void RegisterLoader(Tsukino::Core::Ref<IAssetLoader> loader);

        //--------------------------------------------------------------
        // 拡張子からアセットの種類を取得する関数
        //! @param  ext [in] 拡張子
        //! @return 拡張子に対応するアセットの種類。
        //! @note   対応する種類がない場合は AssetType::None を返す。
        //--------------------------------------------------------------
        AssetType GetAssetTypeFromExtension(const std::string& ext);

        //--------------------------------------------------------------
        // ソースパスからキャッシュパスに変換する関数
        //! @param  sourcePath [in] ソースパス
        //! @return キャッシュパス
        //--------------------------------------------------------------
        [[nodiscard]]
        Tsukino::Core::Path ConvertToCachePath(const Tsukino::Core::Path& sourcePath);

        // AssetManagerがアセットの共有所有者
        AssetMap s_Assets;

        // LoaderもAssetManagerが共有所有
        std::vector<Tsukino::Core::Ref<IAssetLoader>> s_Loaders;

        // AssetTypeごとにインポーターを管理するマップのエイリアス
        using ImporterMap = std::unordered_map<AssetType, Tsukino::Core::Ref<IAssetImporter>>;

        // ImporterもAssetManagerが共有所有
        ImporterMap s_Importers;
    };

}    // namespace Tsukino::Asset
