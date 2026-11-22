//--------------------------------------------------------------
//! @file   ScriptComponent.hpp
//! @brief  エンティティに処理（ラムダ）を直接付けるコンポーネント（Unity の MonoBehaviour に当たる）
//--------------------------------------------------------------
#pragma once
#include <Tsukino/Engine/ECS/Scene.hpp>
#include <Tsukino/Core/ECS/Entity/Entity.hpp>
#include <Tsukino/Core/ECS/Registry/Registry.hpp>

#include <functional>

// 名前空間 : Tsukino::EngineIntegration
namespace Tsukino::EngineIntegration {
    struct EngineContext;    // 前方宣言
}

// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {

    //--------------------------------------------------------------
    //! @struct ScriptContext
    //! @brief  ScriptComponent の処理に渡される、自分のエンティティとシーンへの入口
    //--------------------------------------------------------------
    struct ScriptContext {
        Tsukino::ECS::Scene&                       scene;       // 所属するシーン（CreateEntity など）
        Tsukino::ECS::Registry&                    registry;    // シーンのレジストリ
        Tsukino::EngineIntegration::EngineContext& engine;      // AssetManager・AudioManager・Renderer など
        Tsukino::ECS::Entity                       entity;      // 処理を付けたエンティティ
        float                                      time;        // シーンの経過時間（秒。deltaTime の合計）

        //--------------------------------------------------------------
        //! @brief 自分のコンポーネントを取得します（無ければ未定義動作。不確かなら TryGet）。
        //--------------------------------------------------------------
        template <typename T>
        T& Get() const {
            return registry.GetComponent<T>(entity);
        }

        //--------------------------------------------------------------
        //! @brief 自分のコンポーネントを取得します。
        //! @return コンポーネント（無ければ nullptr）
        //--------------------------------------------------------------
        template <typename T>
        T* TryGet() const {
            return registry.try_get<T>(entity);
        }

        //--------------------------------------------------------------
        //! @brief エンティティの破棄を予約します（全システムの更新が終わってから消える）。
        //! @param target [in] 破棄するエンティティ
        //--------------------------------------------------------------
        void Destroy(Tsukino::ECS::Entity target) const { scene.DestroyEntity(target); }

        //--------------------------------------------------------------
        //! @brief 自分のエンティティの破棄を予約します。
        //--------------------------------------------------------------
        void DestroySelf() const { scene.DestroyEntity(entity); }
    };

    //--------------------------------------------------------------
    //! @struct ScriptComponent
    //! @brief  エンティティに付ける処理です（Unity の MonoBehaviour の Start / Update / OnCollisionEnter に当たる）。
    //! @note   ScriptSystem（AddDefaultSystems で登録される）が呼ぶ。自作の System クラスを書かずに、
    //!         シーンを作るコードの中でラムダとして振る舞いを書ける。
    //!         - onStart          : 付けた後の最初の更新で1回
    //!         - onUpdate         : 毎フレーム（第2引数は deltaTime 秒）
    //!         - onCollisionEnter : 当たり始めた瞬間に1回（第2引数は相手。相手側には相手の onCollisionEnter が呼ばれる）
    //!         ラムダの中でエンティティを作る・壊す・コンポーネントを足してよい（壊すのは予約）。
    //!         ラムダが持つ状態は呼び出しをまたいで保たれる（[timer = 0.0f](…) mutable { timer += dt; } と書ける）。
    //--------------------------------------------------------------
    struct ScriptComponent {
        std::function<void(ScriptContext&)>                       onStart;
        std::function<void(ScriptContext&, float)>                onUpdate;
        std::function<void(ScriptContext&, Tsukino::ECS::Entity)> onCollisionEnter;

        bool started = false;    // onStart を呼んだか（ScriptSystem が書く）
    };
}    // namespace Tsukino::BuiltIn::ECS
