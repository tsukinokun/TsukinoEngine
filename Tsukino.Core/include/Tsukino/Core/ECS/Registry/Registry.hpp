//--------------------------------------------------------------------
// @file   Registry.hpp
// @brief  ECS レジストリラッパー
// @author 山﨑愛
//--------------------------------------------------------------------
#pragma once
#include <entt/entt.hpp>
#include <Tsukino/Core/ECS/Entity/Entity.hpp>
// 名前空間 : Tsukino::ECS
namespace Tsukino::ECS {
    //--------------------------------------------------------------------
    //! @file   Registry.hpp
    //! @brief  ECS レジストリラッパー
    //--------------------------------------------------------------------
    class Registry {
    public:
        //--------------------------------------------------------------------
        //! @brief  エンティティの作成
        //! @return 作成されたエンティティ
        //--------------------------------------------------------------------
        [[nodiscard]]
        Entity CreateEntity() {
            return registry.create();
        }

        //--------------------------------------------------------------------
        //! @brief  エンティティの破棄
        //! @param  entity [in] 破棄するエンティティ
        //--------------------------------------------------------------------
        void DestroyEntity(Entity entity) { registry.destroy(entity); }

        //--------------------------------------------------------------------
        //! @brief  コンポーネントの追加
        //! @tparam T コンポーネントの型
        //! @tparam T コンポーネントのコンストラクタの可変長引数
        //! @param  entity [in] コンポーネントを追加するエンティティ
        //! @param  args   [in] コンポーネントのコンストラクタ
        //--------------------------------------------------------------------
        template <typename T, typename... Args>
        [[nodiscard]]
        T& AddComponent(Entity entity, Args&&... args) {
            return registry.emplace<T>(entity, std::forward<Args>(args)...);
        }

        //--------------------------------------------------------------------
        //! @brief  コンポーネントの取得
        //! @tparam T コンポーネントの型
        //! @param  entity [in] コンポーネントを取得するエンティティ
        //! @return コンポーネントの参照
        //--------------------------------------------------------------------
        template <typename T>
        [[nodiscard]]
        T& GetComponent(Entity entity) {
            return registry.get<T>(entity);
        }

        //--------------------------------------------------------------------
        //! @brief  コンポーネントの存在確認
        //! @tparam T コンポーネントの型
        //! @param  entity [in] コンポーネントを確認するエンティティ
        //! @param  entity [in] コンポーネントがいるか否か
        //--------------------------------------------------------------------
        template <typename T>
        [[nodiscard]]
        bool HasComponent(Entity entity) const {
            return registry.any_of<T>(entity);
        }

        //--------------------------------------------------------------------
        //! @brief  コンポーネントの削除
        //! @tparam T コンポーネントの型
        //! @param  entity [in] コンポーネントを削除するエンティティ
        //--------------------------------------------------------------------
        template <typename T>
        void RemoveComponent(Entity entity) {
            registry.remove<T>(entity);
        }

        //--------------------------------------------------------------------
        //! @brief  特定のコンポーネントのあるエンティティの列挙
        //! @tparam Components コンポーネントの型の可変長引数
        //! @return 特定のコンポーネントのあるエンティティの列挙
        //--------------------------------------------------------------------
        template <typename... Components>
        [[nodiscard]]
        auto View() {
            return registry.view<Components...>();
        }

        //--------------------------------------------------------------------
        //! @brief  コンテキスト変数の設定（グローバルデータの登録）
        //! @tparam T コンテキストの型
        //! @tparam Args コンストラクタ引数の型
        //! @param  args コンストラクタ引数
        //! @return 構築されたコンテキスト変数への参照
        //--------------------------------------------------------------------
        template <typename T, typename... Args>
        T& SetContext(Args&&... args) {
            return registry.ctx().emplace<T>(std::forward<Args>(args)...);
        }

        //--------------------------------------------------------------------
        //! @brief  コンテキスト変数の取得
        //! @tparam T コンテキストの型
        //! @return コンテキスト変数への参照
        //--------------------------------------------------------------------
        template <typename T>
        [[nodiscard]]
        T& GetContext() {
            return registry.ctx().get<T>();
        }

        //--------------------------------------------------------------------
        //! @brief  コンテキスト変数の存在確認
        //! @tparam T コンテキストの型
        //! @return コンテキスト変数が存在するか否か
        //--------------------------------------------------------------------
        template <typename T>
        [[nodiscard]]
        bool HasContext() const {
            return registry.ctx().contains<T>();
        }

        //--------------------------------------------------------------------
        //! @brief  コンテキスト変数の削除
        //! @tparam T コンテキストの型
        //--------------------------------------------------------------------
        template <typename T>
        void RemoveContext() {
            registry.ctx().erase<T>();
        }

        //--------------------------------------------------------------------
        //! @brief  エンティティが有効（生存）しているか確認
        //! @param  entity [in] 確認するエンティティ
        //! @return 有効であれば true
        //--------------------------------------------------------------------
        [[nodiscard]]
        bool IsValid(Entity entity) const {
            return registry.valid(entity);
        }

    private:
        entt::registry registry;    //!< 内部のレジストリ
    };

}    // namespace Tsukino::ECS
