//----------------------------------------------------------------------
// @file   Registry.hpp
// @brief  ECS レジストリラッパー
// @author 山﨑愛
//--------------------------------------------------------------------
#pragma once
#include <entt/entt.hpp>
#include <Tsukino/Core/ECS/Entity/Entity.hpp>

#include <algorithm>
#include <vector>
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
        //! @brief  エンティティの破棄（即時）
        //! @param  entity [in] 破棄するエンティティ
        //! @warning View の反復中に呼んではならない。EnTT はコンポーネントプールの
        //!          要素を入れ替えて詰めるため、反復中に破棄するとイテレータが壊れる。
        //!          System の中から破棄する場合は必ず QueueDestroy() を使うこと。
        //--------------------------------------------------------------------
        void DestroyEntity(Entity entity) { registry.destroy(entity); }

        //--------------------------------------------------------------------
        //! @brief  エンティティの破棄を予約する
        //! @param  entity [in] 破棄するエンティティ
        //! @details
        //! View の反復中でも安全に呼べる。実際の破棄は FlushDestroyQueue() が
        //! 全 System の更新後に行うため、反復中のイテレータ破壊が起きない。
        //--------------------------------------------------------------------
        void QueueDestroy(Entity entity) {
            if(entity == entt::null)
                return;

            m_destroyQueue.push_back(entity);
        }

        //--------------------------------------------------------------------
        //! @brief  破棄予約されたエンティティをまとめて破棄する
        //! @details
        //! Scene::Update() が全 System の更新後に呼ぶ。
        //! - 同一エンティティが複数回予約されることを想定し、重複を除去する
        //! - 破棄処理の中で更に QueueDestroy() されうる（親が子を巻き込む等）ため、
        //!   キューが空になるまで繰り返す。相互参照による無限ループを避けるため
        //!   反復回数に上限を設ける。
        //--------------------------------------------------------------------
        void FlushDestroyQueue() {
            constexpr int kMaxPasses = 8;

            for(int pass = 0; pass < kMaxPasses && !m_destroyQueue.empty(); ++pass) {
                //--------------------------------------------------------------------
                // 破棄中に積まれた分を次のパスへ回すため、いったん退避してから空にする
                //--------------------------------------------------------------------
                std::vector<Entity> pending;
                pending.swap(m_destroyQueue);

                std::sort(pending.begin(), pending.end());
                pending.erase(std::unique(pending.begin(), pending.end()), pending.end());

                for(const Entity entity : pending) {
                    // 予約後に別経路で破棄された可能性があるため、都度検証する
                    if(registry.valid(entity)) {
                        registry.destroy(entity);
                    }
                }
            }

            // 上限まで回っても空にならない場合は相互に破棄を積み合っている
            m_destroyQueue.clear();
        }

        //--------------------------------------------------------------------
        //! @brief  コンポーネント追加シグナルの取得
        //! @tparam T コンポーネントの型
        //! @return 接続用の sink。ハンドラは void(entt::registry&, entt::entity)
        //--------------------------------------------------------------------
        template <typename T>
        [[nodiscard]]
        auto OnConstruct() {
            return registry.template on_construct<T>();
        }

        //--------------------------------------------------------------------
        //! @brief  コンポーネント破棄シグナルの取得
        //! @tparam T コンポーネントの型
        //! @return 接続用の sink。ハンドラは void(entt::registry&, entt::entity)
        //! @details
        //! エンティティ破棄・コンポーネント削除のどの経路を通っても必ず発火するため、
        //! GPU リソースや物理ボディのような「所有権が ECS の外にあるもの」の
        //! 回収はイベントバスではなくこちらで行うこと。
        //--------------------------------------------------------------------
        template <typename T>
        [[nodiscard]]
        auto OnDestroy() {
            return registry.template on_destroy<T>();
        }

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
        //! @brief  コンポーネントのポインタ取得（存在しない場合はnullptr）
        //! @tparam T コンポーネントの型
        //! @param  entity [in] コンポーネントを取得するエンティティ
        //! @return コンポーネントのポインタ
        //--------------------------------------------------------------------
        template <typename T>
        [[nodiscard]]
        T* try_get(Entity entity) {
            return registry.try_get<T>(entity);
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
            auto& ctx = registry.ctx();

            if(ctx.contains<T>()) {
                //--------------------------------------------------------------------
                // 既に存在する場合は取得して直接値を代入する
                //--------------------------------------------------------------------
                T& existing = ctx.get<T>();
                existing    = T(std::forward<Args>(args)...);
                return existing;
            }

            //--------------------------------------------------------------------
            // 存在しない場合は新規作成
            //--------------------------------------------------------------------
            return ctx.emplace<T>(std::forward<Args>(args)...);
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
        entt::registry       registry;         //!< 内部のレジストリ
        std::vector<Entity>  m_destroyQueue;   //!< 破棄予約されたエンティティ（FlushDestroyQueue でまとめて処理）
    };

}    // namespace Tsukino::ECS
