//--------------------------------------------------------------
//! @file   CollisionSoundRecipe.cpp
//! @brief  レシピ: 衝突イベントを受け取り、その瞬間に音を鳴らす
//! @note   要点（詳しくは Recipes/README.md の「衝突イベント」「音」）
//!         - 衝突は PhysicsSystem が EventBus へ CollisionEnterEvent として出す（触れ始めた瞬間に1回）
//!         - 1回の衝突で A→B と B→A の2通りが届く。self 側に注目して処理すれば二重にならない
//!         - イベントは PhysicsSystem::Update の中で即座に（同期で）届く
//!         - Subscribe の戻り値（ScopedConnection）はメンバで持つ。破棄されると購読も外れる
//!         - 音は AssetManager から AudioAsset を取り出し、AudioManager::Play で鳴らす
//--------------------------------------------------------------
#include <Tsukino/Sandbox/Recipes/Recipes.hpp>

#include <Tsukino/EngineIntegration/EngineContext.hpp>
#include <Tsukino/BuiltIn/ECS/Event/CollisionEnterEvent.hpp>
#include <Tsukino/Engine/Asset/AssetManager.hpp>
#include <Tsukino/Engine/Asset/Audio/AudioAsset.hpp>
#include <Tsukino/Audio/AudioManager.hpp>
#include <Tsukino/Core/ECS/Event/EventBus.hpp>
#include <Tsukino/Core/Log.hpp>

#include <memory>
#include <string>

// 名前空間 : Tsukino::Sandbox::Recipes
namespace Tsukino::Sandbox::Recipes {

    //--------------------------------------------------------------
    //! @brief 衝突イベントの購読を始めます。
    //--------------------------------------------------------------
    CollisionSoundSystem::CollisionSoundSystem(Tsukino::ECS::EventBus& eventBus, Tsukino::ECS::Registry& registry) : m_registry(&registry) {
        m_connection = eventBus.Subscribe<Tsukino::BuiltIn::ECS::CollisionEnterEvent>(
            [this](const Tsukino::BuiltIn::ECS::CollisionEnterEvent& event) { OnCollisionEnter(event); });
    }

    //--------------------------------------------------------------
    //! @brief 毎フレームの処理は無い（イベントが来たときだけ動く）。
    //--------------------------------------------------------------
    void CollisionSoundSystem::Update(Tsukino::ECS::Registry& /*registry*/, float /*deltaTime*/) {}

    //--------------------------------------------------------------
    //! @brief 当たった側（self）が CollisionSoundComponent を持っていれば、その音を鳴らします。
    //--------------------------------------------------------------
    void CollisionSoundSystem::OnCollisionEnter(const Tsukino::BuiltIn::ECS::CollisionEnterEvent& event) {
        Tsukino::ECS::Registry& registry = *m_registry;
        if(!registry.IsValid(event.self) || !registry.HasComponent<CollisionSoundComponent>(event.self))
            return;    // 相手側から見た通知（other が音を持つ側）はここで捨てる

        auto& collisionSound = registry.GetComponent<CollisionSoundComponent>(event.self);
        auto* context        = registry.GetContext<Tsukino::EngineIntegration::EngineContext*>();

        // AssetManager::Get は基底の型で返すので、AudioAsset に直してから鳴らす
        const auto audio = std::dynamic_pointer_cast<Tsukino::Asset::AudioAsset>(context->assetManager->Get(collisionSound.sound));
        if(!audio) {
            Tsukino::Core::Log::Error("[Recipe] CollisionSoundSystem: the sound asset is not loaded. Nothing is played.");
            return;
        }
        context->audioManager->Play(*audio, false, collisionSound.volume);
        ++collisionSound.playCount;

        // ログは Log::SetLogFile で指定したファイルにも出る（デバッガの外で確かめるときに読む）
        Tsukino::Core::Log::Info("[Recipe] collision sound played (count=" + std::to_string(collisionSound.playCount) + ")");
    }
}    // namespace Tsukino::Sandbox::Recipes
