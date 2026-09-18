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

#include <condition_variable>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
// 名前空間 : Tsukino::Asset
namespace Tsukino::Asset {
    class IAsset;          // 前方宣言
    class IAssetLoader;    // 前方宣言
    //--------------------------------------------------------------
    //! @class  AssetManager
    //! @brief  アセットのロード、管理を行うクラス
    //! @note   Load / Get / Exists / RegisterAsset は任意のスレッドから呼べる
    //!         （ロード画面が裏スレッドで読み、本体スレッドは描画で Get し続ける使い方を想定）。
    //!         インポート・ロードの本体はロックを持たずに走るので、裏で重い変換をしている間も
    //!         本体スレッドの Get は待たされない。アセット層は CPU 側のデータを作るだけで
    //!         D3D には触らない（GPU リソースは描画側が本体スレッドで遅延生成する）。
    //!         テクスチャの変換は WIC（COM）を使うため、裏スレッドから Load する場合は
    //!         呼び出し側がそのスレッドで CoInitializeEx を済ませておくこと。
    //!         Initialize / RegisterImporter はスレッドを立てる前（起動時）に呼ぶこと
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

        //--------------------------------------------------------------
        // アセットを登録する関数
        //! @param  handle [in] 登録するアセットのハンドル
        //! @param  asset  [in] 登録するアセットのshared_ptr
        //--------------------------------------------------------------
        void RegisterAsset(AssetHandle handle, Tsukino::Core::Ref<IAsset> asset);

    private:
        //--------------------------------------------------------------
        // キャッシュに無いアセットを実際に読み込む関数（Load の本体）
        //! @param  path    [in] ロードするアセットのパス
        //! @param  pathKey [in] 正規化済みのパス（表のキー）
        //! @return ロードしたアセットのハンドル。失敗したら Invalid
        //! @note   ロックを持たずに呼ぶ。表への登録だけ中でロックする
        //--------------------------------------------------------------
        [[nodiscard]]
        AssetHandle LoadUncached(const Tsukino::Core::Path& path, const std::string& pathKey);

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

        //--------------------------------------------------------------
        // 以下は AssetManager インスタンスごとのメンバであり、静的変数ではない。
        // 以前は s_ プレフィックス（コーディング規約では静的メンバ用）が
        // 付いていて、グローバルな共有状態と読み違えやすかったため m_ に改めた。
        //--------------------------------------------------------------

        // AssetManagerがアセットの共有所有者
        AssetMap m_assets;

        //--------------------------------------------------------------
        // 既にロード済みのパス -> ハンドル の対応表。
        //
        // これが無いと Load() は同じパスでも毎回キャッシュファイルを読み直し、
        // 新しいハンドルを発行して別アセットとして登録してしまう。
        // ModelSystem の GPU メッシュバッファのキャッシュはモデルハンドルを
        // キーにしているため、同じモデルを N 回 Load すると頂点／インデックス
        // バッファも N 組作られることになる。
        //
        // アセットを個別に解放する手段（Unload）は無く、AssetManager が生きている
        // 間はロード済みアセットも生き続けるため、この対応表が古くなることはない。
        //--------------------------------------------------------------
        std::unordered_map<std::string, AssetHandle> m_pathToHandle;

        // LoaderもAssetManagerが共有所有
        std::vector<Tsukino::Core::Ref<IAssetLoader>> m_loaders;

        // AssetTypeごとにインポーターを管理するマップのエイリアス
        using ImporterMap = std::unordered_map<AssetType, Tsukino::Core::Ref<IAssetImporter>>;

        // ImporterもAssetManagerが共有所有
        ImporterMap m_importers;

        //--------------------------------------------------------------
        // スレッド間の排他。
        //
        // m_assets / m_pathToHandle / m_loadingPaths を触る瞬間だけ取る。
        // インポート・ロードの本体の間は持たないので、ModelLoader が中で
        // テクスチャを Load し直す（再帰）ときにも自分で詰まらない。
        // m_loaders / m_importers は Initialize 後に変わらないので守らない。
        //
        // m_loadingPaths は「今どこかのスレッドが読み込み中のパス」。同じパスを
        // 2つのスレッドが同時に読むと、同じキャッシュファイルへ同時に書き込んで
        // 壊しかねないので、後から来た方は m_loadFinished で完了を待ち、
        // 先の方が登録したハンドルをそのまま返す
        //--------------------------------------------------------------
        std::mutex                      m_mutex;
        std::condition_variable         m_loadFinished;
        std::unordered_set<std::string> m_loadingPaths;
    };

}    // namespace Tsukino::Asset
