//--------------------------------------------------------------------
// @file   AssetRef.hpp
// @brief  パスベースのアセット間接参照
// @author 山﨑愛
//--------------------------------------------------------------------
#pragma once
#include <Tsukino/Engine/Asset/AssetHandle.hpp>
#include <Tsukino/Core/typedef.hpp>

#include <cereal/cereal.hpp>
#include <cereal/specialize.hpp>

#include <string>

// 名前空間 : Tsukino::Asset
namespace Tsukino::Asset {
    class AssetManager;

    //--------------------------------------------------------------------
    //! @struct AssetRef
    //! @brief  アセットをパスで間接参照する型。
    //!
    //!         AssetHandle はプロセス内限定の値でありJSONへ直接書き出せないため、
    //!         代わりにこの型がパス文字列を保持し、PrefabFactoryがInstantiate時に
    //!         AssetManager::Loadで解決してhandleへ書き込む（ScopedAssetRefResolver参照）。
    //!
    //!         AssetHandleとの暗黙変換・主要メソッドの転送を持つ「透過的なラッパー」
    //!         として設計しているため、既存のAssetHandleを直接扱っているコードは
    //!         変更なしでそのまま動作する。
    //--------------------------------------------------------------------
    struct AssetRef {
        AssetHandle handle;    // 解決済みハンドル（未解決ならInvalid）
        std::string path;      // JSON上のアセットパス

        AssetRef() = default;
        // 暗黙変換にすると、cerealがAssetHandle自体のシリアライズ可否をSFINAEで
        // 判定する際にこのコンストラクタ経由でAssetRefのsave_minimal/load_minimal
        // へ暗黙変換できてしまい、"cereal found more than one compatible
        // serialization function" という無関係な型の判定エラーを引き起こす。
        // そのためexplicitにし、従来通り「field = handle;」で代入できるよう
        // operator=を別途用意する。
        explicit AssetRef(AssetHandle h)
            : handle(h) {}

        AssetRef& operator=(AssetHandle h) {
            handle = h;
            return *this;
        }

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
    //! @note   save/loadではなくsave_minimal/load_minimalを使う理由：
    //!         通常のsave/loadだと、cerealはAssetRefを「ノード（オブジェクト）を
    //!         持つ複合型」とみなしstartNode()/finishNode()で入れ子構造を作ってしまう
    //!         （JSON上は "modelHandle": {"value0": "path"} のような形になる）。
    //!         AssetRefをJSON上で単純な文字列（"modelHandle": "path"）として
    //!         扱いたいため、cerealに「この型はスカラー値として保存する」と
    //!         明示するsave_minimal/load_minimalを使う（EntityRefと同じ理由）。
    //--------------------------------------------------------------------
    template <class Archive>
    std::string save_minimal(const Archive&, const AssetRef& ref) {
        return ref.path;
    }

    //--------------------------------------------------------------------
    //! @brief  パスからアセットを読んでハンドルを得る（AssetManager::Loadへの転送）
    //! @param  assetManager [in] 読み込みに使うAssetManager
    //! @param  path         [in] アセットのパス
    //! @return 読み込んだアセットのハンドル
    //! @note   load_minimalはテンプレートで、ここでAssetManagerを完全型にすると
    //!         AssetManager.hppとの循環includeになるため、実体はAssetManager.cppに置く
    //--------------------------------------------------------------------
    AssetHandle ResolveAssetRefPath(AssetManager& assetManager, const std::string& path);

    //--------------------------------------------------------------------
    //! @class  ScopedAssetRefResolver
    //! @brief  生存している間、このスレッドで読み込まれるAssetRefのhandleまで解決させるスコープ
    //! @note   PrefabFactory::Instantiateが、Componentを本物のJSONアーカイブで読む間だけ張る。
    //!         以前はload()を擬似アーカイブで再訪問して解決していたが、hlslppのベクトル等の
    //!         load()が「ローカルへ読んで代入」する作りのため、再訪問で全て0に上書きされていた。
    //!         読み込みの最中に解決すれば再訪問そのものが要らない
    //--------------------------------------------------------------------
    class ScopedAssetRefResolver {
    public:
        //--------------------------------------------------------------------
        //! @brief  このスレッドの解決先を設定する
        //! @param  assetManager [in] 解決に使うAssetManager（nullptrなら解決しない）
        //--------------------------------------------------------------------
        explicit ScopedAssetRefResolver(AssetManager* assetManager)
            : m_previous(Current()) {
            Current() = assetManager;
        }

        //--------------------------------------------------------------------
        //! @brief  解決先を元に戻す（入れ子にしても外側のスコープの設定へ戻る）
        //--------------------------------------------------------------------
        ~ScopedAssetRefResolver() { Current() = m_previous; }

        ScopedAssetRefResolver(const ScopedAssetRefResolver&)            = delete;
        ScopedAssetRefResolver& operator=(const ScopedAssetRefResolver&) = delete;

        //--------------------------------------------------------------------
        //! @brief  このスレッドで現在有効な解決先を得る
        //! @return 解決に使うAssetManager（スコープ外ならnullptr）
        //--------------------------------------------------------------------
        [[nodiscard]]
        static AssetManager*& Current() {
            thread_local AssetManager* s_current = nullptr;
            return s_current;
        }

    private:
        AssetManager* m_previous;    // 入れ子のとき戻す先
    };

    //--------------------------------------------------------------------
    //! @brief  cereal用：読み込み処理
    //! @note   pathを読む。ScopedAssetRefResolverの内側（Prefabの生成中）なら、handleまで解決する。
    //!         外側（PrefabFactory::Loadなど）ではpathだけを読み、handleは未解決のまま残す
    //--------------------------------------------------------------------
    template <class Archive>
    void load_minimal(const Archive&, AssetRef& ref, const std::string& value) {
        ref.path   = value;
        ref.handle = AssetHandle::Invalid();

        AssetManager* assetManager = ScopedAssetRefResolver::Current();
        if(assetManager != nullptr && !value.empty())
            ref.handle = ResolveAssetRefPath(*assetManager, value);
    }

}    // namespace Tsukino::Asset

//--------------------------------------------------------------------
// AssetRef は AssetHandle への暗黙変換（operator AssetHandle）を持つため、
// cerealがAssetHandle用のsave/loadも「変換すれば呼べる候補」として拾ってしまい、
// save_minimal/load_minimalと衝突してあいまいになる。明示的に使用方式を指定して解消する。
//--------------------------------------------------------------------
CEREAL_SPECIALIZE_FOR_ALL_ARCHIVES(Tsukino::Asset::AssetRef, cereal::specialization::non_member_load_save_minimal)
