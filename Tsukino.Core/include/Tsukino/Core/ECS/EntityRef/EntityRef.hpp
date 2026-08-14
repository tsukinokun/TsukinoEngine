//--------------------------------------------------------------------
// @file   EntityRef.hpp
// @brief  Prefabバッチ内の他エンティティへの間接参照
// @author 山﨑愛
//--------------------------------------------------------------------
#pragma once
#include <Tsukino/Core/ECS/Entity/Entity.hpp>

#include <cereal/cereal.hpp>
#include <cereal/specialize.hpp>

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
    //!
    //!         Entity（entt::entity）との暗黙変換を持つ「透過的なラッパー」として設計して
    //!         いるため、既存のEntity型を直接扱っているコードは変更なしでそのまま動作する。
    //--------------------------------------------------------------------
    struct EntityRef {
        Entity      entity = entt::null;    // 解決済みの実体（未解決なら entt::null）
        std::string localName;              // JSON上の名前（例: "#Dice0"）

        EntityRef() = default;
        // 暗黙変換にすると、cerealがEntity（entt::entity）自体のシリアライズ可否を
        // SFINAEで判定する際にこのコンストラクタ経由でEntityRefのsave_minimal/
        // load_minimalへ暗黙変換できてしまい、無関係な型のシリアライズ判定を
        // 壊してしまう（AssetRefで実際に踏んだのと同じ罠）。そのためexplicitにし、
        // 従来通り「field = entity;」で代入できるようoperator=を別途用意する。
        explicit EntityRef(Entity e)
            : entity(e) {}

        EntityRef& operator=(Entity e) {
            entity = e;
            return *this;
        }

        operator Entity() const { return entity; }

        bool operator==(const EntityRef& other) const { return entity == other.entity; }
        bool operator!=(const EntityRef& other) const { return entity != other.entity; }
        bool operator==(Entity other) const { return entity == other; }
        bool operator!=(Entity other) const { return entity != other; }
    };

    //--------------------------------------------------------------------
    //! @brief  cereal用：保存処理（localNameのみを書き出す）
    //! @note   save/loadではなくsave_minimal/load_minimalを使う理由：
    //!         通常のsave/loadだと、cerealはEntityRefを「ノード（オブジェクト）を
    //!         持つ複合型」とみなしstartNode()/finishNode()で入れ子構造を作ってしまう
    //!         （JSON上は "target": {"value0": "#Anchor"} のような形になる）。
    //!         EntityRefをJSON上で単純な文字列（"target": "#Anchor"）として
    //!         扱いたいため、cerealに「この型はスカラー値として保存する」と
    //!         明示するsave_minimal/load_minimalを使う。
    //--------------------------------------------------------------------
    template <class Archive>
    std::string save_minimal(const Archive&, const EntityRef& ref) {
        return ref.localName;
    }

    //--------------------------------------------------------------------
    //! @brief  cereal用：読み込み処理（localNameのみを読み込む。entityの解決はEntityRefResolverArchiveが行う）
    //--------------------------------------------------------------------
    template <class Archive>
    void load_minimal(const Archive&, EntityRef& ref, const std::string& value) {
        ref.localName = value;
        ref.entity    = entt::null;
    }

}    // namespace Tsukino::ECS

//--------------------------------------------------------------------
// EntityRef は Entity（entt::entity）への暗黙変換（operator Entity）を持つため、
// cerealがEntity用の（他で定義されうる）save/loadも「変換すれば呼べる候補」として
// 拾ってしまい、save_minimal/load_minimalと衝突してあいまいになりうる。
// 明示的に使用方式を指定して解消する（AssetRefと同じ理由）。
//--------------------------------------------------------------------
CEREAL_SPECIALIZE_FOR_ALL_ARCHIVES(Tsukino::ECS::EntityRef, cereal::specialization::non_member_load_save_minimal)
