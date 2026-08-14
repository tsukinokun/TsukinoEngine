//--------------------------------------------------------------------
// @file   EntityRef.hpp
// @brief  Prefabバッチ内の他エンティティへの間接参照
// @author 山﨑愛
//--------------------------------------------------------------------
#pragma once
#include <Tsukino/Core/ECS/Entity/Entity.hpp>

#include <cereal/cereal.hpp>

#include <string>

// 名前空間 : Tsukino::ECS
namespace Tsukino::ECS {

    //--------------------------------------------------------------------
    //! @struct EntityRef
    //! @brief  PrefabFactory::InstantiateGroup で生成した名前付きエンティティ群の中から
    //!         他エンティティを名前で参照するための間接参照型。
    //!
    //!         通常のJSONロード時は localName のみが読み込まれ、entity は未解決のまま
    //!         entt::null になる。InstantiateGroup 完了後に EntityRefResolverArchive を
    //!         使って localName -> entity の解決が行われる。
    //--------------------------------------------------------------------
    struct EntityRef {
        Entity      entity = entt::null;    // 解決済みの実体（未解決なら entt::null）
        std::string localName;              // JSON上の名前（例: "#Dice0"）
    };

    //--------------------------------------------------------------------
    //! @brief  cereal用：保存処理（localNameのみを書き出す）
    //--------------------------------------------------------------------
    template <class Archive>
    void save(Archive& archive, const EntityRef& ref) {
        archive(ref.localName);
    }

    //--------------------------------------------------------------------
    //! @brief  cereal用：読み込み処理（localNameのみを読み込む。entityの解決はEntityRefResolverArchiveが行う）
    //--------------------------------------------------------------------
    template <class Archive>
    void load(Archive& archive, EntityRef& ref) {
        archive(ref.localName);
        ref.entity = entt::null;
    }

}    // namespace Tsukino::ECS
