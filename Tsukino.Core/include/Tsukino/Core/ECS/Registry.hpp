//---------------------------------------------------------------
//! @file	Registry.hpp
//! @brief  ECSのレジストリクラスの宣言
//! @author 山﨑愛
//---------------------------------------------------------------
#pragma once
#include <entt/entt.hpp>
#include "Entity.hpp"
// 名前空間 : Tsukino::ECS
namespace tsukino::ECS {
    //---------------------------------------------------------------
    //! @class   Registry
    //! @brief   ECSのレジストリクラス
    //! @details entt::registryをラップして、エンティティの生成・破棄やコンポーネントの管理を行うクラス
    //---------------------------------------------------------------
    class Registry {
    public:
        //---------------------------------------------------------------
        //! @brief  デフォルトコンストラクタ
        //---------------------------------------------------------------
        Registry() = default;

        //---------------------------------------------------------------
        //! @brief  デストラクタ
        //---------------------------------------------------------------
        Entity CreateEntity() { return registry.create(); }

        //---------------------------------------------------------------
        //! @brief  エンティティの破棄
        //! @param  entity [in] 破棄するエンティティID
        //---------------------------------------------------------------
        void DestroyEntity(Entity entity) { registry.destroy(entity); }

        //---------------------------------------------------------------
        //! @brief  コンポーネントの追加
        //! @Tparam  T      [in] コンポーネントの型
        //! @tparam  Args   [in] コンポーネントのコンストラクタ引数の型リスト
        //! @param  entity  [in] コンポーネントを追加するエンティティID
        //! @param  args    [in] コンポーネントのコンストラクタ引数
        //---------------------------------------------------------------
        template <typename T, typename... Args>
        T& AddComponent(Entity entity, Args&&... args) {
            return registry.emplace<T>(entity, std::forward<Args>(args)...);
        }

        //---------------------------------------------------------------
        //! @brief  コンポーネントの取得
        //! @Tparam  T     [in] コンポーネントの型
        //! @param  entity [in] コンポーネントを取得するエンティティID
        //---------------------------------------------------------------
        template <typename T>
        T& GetComponent(Entity entity) {
            return registry.get<T>(entity);
        }

        //---------------------------------------------------------------
        //! @brief  コンポーネントの存在確認
        //! @Tparam  T     [in] コンポーネントの型
        //! @param  entity [in] コンポーネントの存在を確認するエンティティID
        //---------------------------------------------------------------
        template <typename T>
        bool HasComponent(Entity entity) const {
            return registry.any_of<T>(entity);
        }

        //---------------------------------------------------------------
        //! @brief  コンポーネントの削除
        //---------------------------------------------------------------
        template <typename T>
        void RemoveComponent(Entity entity) {
            registry.remove<T>(entity);
        }

        //---------------------------------------------------------------
        //! @brief  ビューの取得
        //! @Tparam  Components [in] ビューに含めるコンポーネントの型リスト
        //---------------------------------------------------------------
        template <typename... Components>
        auto View() {
            return registry.view<Components...>();
        }

    private:
        entt::registry registry;
    };

}    // namespace tsukino::ECS
