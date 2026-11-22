//--------------------------------------------------------------
//! @file   ScriptSystem.cpp
//! @brief  ScriptComponent の処理を呼ぶシステムの実装
//--------------------------------------------------------------
#include <Tsukino/EngineIntegration/ECS/System/ScriptSystem.hpp>

#include <Tsukino/EngineIntegration/EngineContext.hpp>
#include <Tsukino/BuiltIn/ECS/Event/CollisionEnterEvent.hpp>
#include <Tsukino/Core/ECS/Event/EventBus.hpp>

#include <utility>
#include <vector>

// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {

    namespace {
        //--------------------------------------------------------------
        //! @brief ScriptComponent の処理を1つ呼びます。
        //! @note  処理の中でコンポーネントを足すとプールが詰め直されて関数の置き場所が動きうるので、
        //!        呼ぶ間は関数をコンポーネントから取り出しておき、呼び終えたら戻す。
        //!        写しを呼ぶだけにすると、ラムダの中の状態（mutable で数えた回数など）が毎回捨てられてしまう。
        //!        - 処理の中で自分の ScriptComponent が外された・エンティティが消えた：戻さない
        //!        - 処理の中で同じ処理が別の関数に差し替えられた：差し替え後を残す
        //--------------------------------------------------------------
        template <typename Function, typename... Args>
        void Invoke(Tsukino::ECS::Registry& registry, Tsukino::ECS::Entity entity, Function ScriptComponent::*member, Args&&... args) {
            if(!registry.IsValid(entity) || !registry.HasComponent<ScriptComponent>(entity))
                return;
            Function function = std::move(registry.GetComponent<ScriptComponent>(entity).*member);
            if(!function)
                return;
            registry.GetComponent<ScriptComponent>(entity).*member = nullptr;

            function(std::forward<Args>(args)...);

            if(!registry.IsValid(entity) || !registry.HasComponent<ScriptComponent>(entity))
                return;
            if(Function& slot = registry.GetComponent<ScriptComponent>(entity).*member; !slot)
                slot = std::move(function);
        }
    }    // namespace

    //--------------------------------------------------------------
    //! @brief 衝突イベントの購読を始めます。
    //--------------------------------------------------------------
    ScriptSystem::ScriptSystem(Tsukino::ECS::Scene& scene) : m_scene(&scene) {
        m_connection = scene.GetEventBus().Subscribe<CollisionEnterEvent>([this](const CollisionEnterEvent& event) { OnCollisionEnter(event); });
    }

    //--------------------------------------------------------------
    //! @brief onStart（初回だけ）と onUpdate を呼びます。
    //--------------------------------------------------------------
    void ScriptSystem::Update(Tsukino::ECS::Registry& registry, float deltaTime) {
        m_time += deltaTime;

        auto* engine = registry.GetContext<Tsukino::EngineIntegration::EngineContext*>();
        if(engine == nullptr)
            return;

        // 処理の中でエンティティを作る・コンポーネントを足すと View の反復が壊れるので、先に対象を写し取る
        std::vector<Tsukino::ECS::Entity> entities;
        for(Tsukino::ECS::Entity entity : registry.View<ScriptComponent>())
            entities.push_back(entity);

        for(Tsukino::ECS::Entity entity : entities) {
            ScriptContext context{*m_scene, registry, *engine, entity, m_time};

            if(!registry.IsValid(entity) || !registry.HasComponent<ScriptComponent>(entity))
                continue;
            if(auto& script = registry.GetComponent<ScriptComponent>(entity); !script.started) {
                script.started = true;
                Invoke(registry, entity, &ScriptComponent::onStart, context);
            }
            Invoke(registry, entity, &ScriptComponent::onUpdate, context, deltaTime);
        }
    }

    //--------------------------------------------------------------
    //! @brief 当たった側（self）の onCollisionEnter を呼びます。
    //! @note  1回の衝突で A→B と B→A の2通りのイベントが来るので、両方の onCollisionEnter がちょうど1回ずつ呼ばれる
    //--------------------------------------------------------------
    void ScriptSystem::OnCollisionEnter(const CollisionEnterEvent& event) {
        Tsukino::ECS::Registry& registry = m_scene->GetRegistry();
        if(!registry.IsValid(event.self) || !registry.HasComponent<ScriptComponent>(event.self))
            return;

        auto* engine = registry.GetContext<Tsukino::EngineIntegration::EngineContext*>();
        if(engine == nullptr)
            return;

        ScriptContext context{*m_scene, registry, *engine, event.self, m_time};
        Invoke(registry, event.self, &ScriptComponent::onCollisionEnter, context, event.other);
    }
}    // namespace Tsukino::BuiltIn::ECS
