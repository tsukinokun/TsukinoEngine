//--------------------------------------------------------------------
// @file   EntityRefResolverArchive.hpp
// @brief  EntityRef::localName を実体（entt::entity）へ解決するための擬似cerealアーカイブ
// @author 山﨑愛
//--------------------------------------------------------------------
#pragma once
#include <Tsukino/Core/ECS/EntityRef/EntityRef.hpp>
#include <Tsukino/Core/Log.hpp>

#include <cereal/cereal.hpp>
//--------------------------------------------------------------------
// cereal::load / cereal::serialize という名前を確実に存在させるため、
// 代表的なコンテナ型のシリアライズ定義を明示的にインクルードしておく
// （using cereal::load; 等のusing宣言は、名前自体が1つも宣言されていないと
//   コンパイルエラーになるため、呼び出し側TUの状況に依存させない）
//--------------------------------------------------------------------
#include <cereal/types/array.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/string.hpp>

#include <string>
#include <unordered_map>
#include <utility>

// 名前空間 : Tsukino::ECS
namespace Tsukino::ECS {

    //--------------------------------------------------------------------
    //! @class  EntityRefResolverArchive
    //! @brief  cerealの本物のアーカイブではなく、既にメモリ上にロード済みのコンポーネントを
    //!         再訪問して EntityRef フィールドだけを解決する軽量なビジタークラス。
    //!
    //!         各コンポーネントの load(Archive&, T&) はArchive型に依存しないテンプレートで
    //!         書かれているため、このクラスをそのまま渡すだけで再利用できる。
    //!         - EntityRef 以外のフィールドへの operator() は何もしない（no-op）
    //!         - ネストした構造体・コンテナ（std::array等）へは load/serialize を辿って再帰する
    //--------------------------------------------------------------------
    class EntityRefResolverArchive {
    public:
        //--------------------------------------------------------------------
        //! @brief  コンストラクタ
        //! @param  nameMap [in] InstantiateGroup で生成した「名前 -> エンティティ」対応表
        //--------------------------------------------------------------------
        explicit EntityRefResolverArchive(const std::unordered_map<std::string, Entity>& nameMap)
            : m_nameMap(nameMap) {}

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
        //! @brief  EntityRef を発見したら名前表から実体を引いて解決する
        //--------------------------------------------------------------------
        void Process(EntityRef& ref) {
            if(ref.localName.empty()) {
                return;
            }

            std::string key = ref.localName;
            if(key.front() == '#') {
                key.erase(0, 1);
            }

            auto it = m_nameMap.find(key);
            if(it != m_nameMap.end()) {
                ref.entity = it->second;
            } else {
                Tsukino::Core::Log::Warn("EntityRef unresolved: " + ref.localName);
            }
        }

        //--------------------------------------------------------------------
        //! @brief  それ以外の型：load/serializeが定義されていれば再帰し、
        //!         プリミティブ型など末端のフィールドでは何もしない
        //--------------------------------------------------------------------
        template <typename T>
        void Process(T& value) {
            using cereal::load;
            using cereal::serialize;

            if constexpr(requires(EntityRefResolverArchive& archive) { load(archive, value); }) {
                load(*this, value);
            } else if constexpr(requires(EntityRefResolverArchive& archive) { serialize(archive, value); }) {
                serialize(*this, value);
            }
            // load/serialize が存在しないプリミティブ型等はここで再帰が止まる（no-op）
        }

        const std::unordered_map<std::string, Entity>& m_nameMap;
    };

}    // namespace Tsukino::ECS
