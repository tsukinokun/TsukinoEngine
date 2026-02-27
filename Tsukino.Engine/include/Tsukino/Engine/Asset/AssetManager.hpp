//--------------------------------------------------------------
//! @file   AssetManager.hpp
//! @brief  アセット管理クラスの宣言
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
#include <unordered_map>
#include <vector>
#include <memory>

#include <Tsukino/Core/Path.hpp>
#include <Tsukino/Core/Memory.hpp>
#include <Tsukino/Engine/Asset/AssetHandle.hpp>
#include <Tsukino/Engine/Asset/AssetMap.hpp>
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
        // AssetManager を初期化する関数
        //--------------------------------------------------------------
        static void Initialize();

        //--------------------------------------------------------------
        // AssetManagerをシャットダウンする関数
        //--------------------------------------------------------------
        static void Destroy();

        //--------------------------------------------------------------
        // アセットをロードする関数
        //! @param  path [in] ロードするアセットのパス
        //! @return ロードしたアセットのハンドル
        //--------------------------------------------------------------
        [[nodiscard]]
        static AssetHandle Load(const Tsukino::Core::Path& path);

        //--------------------------------------------------------------
        // ハンドルからアセットを取得する関数
        //! @param  handle [in] 取得するアセットのハンドル
        //! @return 取得したアセットのshared_ptr
        //--------------------------------------------------------------
        [[nodiscard]]
        static Tsukino::Core::Ref<IAsset> Get(AssetHandle handle);

        //--------------------------------------------------------------
        //! @brief  ハンドルから、アセットが存在するか確認する関数
        //! @param  handle [in] 確認するアセットのハンドル
        //! @return 存在する場合は true、存在しない場合は false
        //--------------------------------------------------------------
        [[nodiscard]]
        static bool Exists(AssetHandle handle);

    private:
        //--------------------------------------------------------------
        //! @brief  ローダーを登録する関数
        //! @param  loader [in] 登録するローダーのshared_ptr
        //--------------------------------------------------------------
        static void RegisterLoader(Tsukino::Core::Ref<IAssetLoader> loader);

        // AssetManagerがアセットの共有所有者
        static AssetMap s_Assets;

        // LoaderもAssetManagerが共有所有
        static std::vector<Tsukino::Core::Ref<IAssetLoader>> s_Loaders;
    };

}    // namespace Tsukino::Asset
