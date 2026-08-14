//--------------------------------------------------------------------
// @file   AssetRefResolverArchive.hpp
// @brief  AssetRef::path をAssetHandleへ解決するための擬似cerealアーカイブ
// @author 山﨑愛
//--------------------------------------------------------------------
#pragma once
#include <Tsukino/Engine/Asset/AssetRef.hpp>
#include <Tsukino/Engine/Asset/AssetManager.hpp>

#include <Tsukino/Core/Path.hpp>

#include <cereal/cereal.hpp>
#include <cereal/types/array.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/string.hpp>

// 名前空間 : Tsukino::Asset
namespace Tsukino::Asset {

    //--------------------------------------------------------------------
    //! @class  AssetRefResolverArchive
    //! @brief  Tsukino::ECS::EntityRefResolverArchive と同じ設計の軽量ビジター。
    //!         既にロード済みのコンポーネントを再訪問し、AssetRefフィールドだけを
    //!         AssetManager::Loadで解決する。EntityRefと違いバッチ内の順序に
    //!         依存しないため、Instantiate直後にその場で1回呼ぶだけでよい。
    //--------------------------------------------------------------------
    class AssetRefResolverArchive {
    public:
        //--------------------------------------------------------------------
        //! @brief  コンストラクタ
        //! @param  assetManager [in] パスからハンドルを解決するために使うAssetManager
        //--------------------------------------------------------------------
        explicit AssetRefResolverArchive(AssetManager& assetManager)
            : m_assetManager(assetManager) {}

        //--------------------------------------------------------------------
        //! @brief  cerealのload()内から呼ばれる可変長のarchive(...)呼び出しを受け取る
        //--------------------------------------------------------------------
        template <typename... Types>
        void operator()(Types&&... args) {
            (Process(std::forward<Types>(args)), ...);
        }

    private:
        //--------------------------------------------------------------------
        //! @brief  NameValuePair（cereal::make_nvpの戻り値）はラップを外して中身だけ処理する
        //--------------------------------------------------------------------
        template <typename T>
        void Process(cereal::NameValuePair<T> nvp) {
            Process(nvp.value);
        }

        //--------------------------------------------------------------------
        //! @brief  AssetRef を発見したらAssetManagerでパスを解決する
        //--------------------------------------------------------------------
        void Process(AssetRef& ref) {
            if(ref.path.empty()) {
                return;
            }
            ref.handle = m_assetManager.Load(Tsukino::Core::Path(ref.path));
        }

        //--------------------------------------------------------------------
        //! @brief  それ以外の型：load/serializeが定義されていれば再帰し、
        //!         プリミティブ型など末端のフィールドでは何もしない
        //--------------------------------------------------------------------
        template <typename T>
        void Process(T& value) {
            using cereal::load;
            using cereal::serialize;

            if constexpr(requires(AssetRefResolverArchive& archive) { load(archive, value); }) {
                load(*this, value);
            } else if constexpr(requires(AssetRefResolverArchive& archive) { serialize(archive, value); }) {
                serialize(*this, value);
            }
            // load/serialize が存在しないプリミティブ型等はここで再帰が止まる（no-op）
        }

        AssetManager& m_assetManager;
    };

}    // namespace Tsukino::Asset
