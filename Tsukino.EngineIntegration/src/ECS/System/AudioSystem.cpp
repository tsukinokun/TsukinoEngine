#include <Tsukino/EngineIntegration/ECS/System/AudioSystem.hpp>
//--------------------------------------------------------------
//! @file   AudioSystem.cpp
//! @brief  オーディオコンポーネントを処理するシステムの実装
//! @author 山﨑愛
//--------------------------------------------------------------
#include <Tsukino/EngineIntegration/ECS/System/AudioSystem.hpp>
#include <Tsukino/EngineIntegration/EngineContext.hpp>
#include <Tsukino/Engine/Asset/AssetManager.hpp>
#include <Tsukino/Engine/Asset/Audio/AudioAsset.hpp>
#include <Tsukino/BuiltIn/ECS/Component/AudioComponent.hpp>
#include <Tsukino/Audio/AudioManager.hpp>

// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {

    //--------------------------------------------------------------
    //! @brief  システムの更新処理
    //--------------------------------------------------------------
    void AudioSystem::Update(Tsukino::ECS::Registry& registry, float deltaTime) {
        Tsukino::EngineIntegration::EngineContext* context = registry.GetContext<Tsukino::EngineIntegration::EngineContext*>();
        if(!context || !context->assetManager || !context->audioManager) {
            return;
        }

        auto view = registry.View<AudioComponent>();
        for(Tsukino::ECS::Entity entity : view) {
            AudioComponent& audioComp = view.get<AudioComponent>(entity);

            if(audioComp.playOnAwake && !audioComp.isPlaying && !audioComp.playTrigger) {
                audioComp.playTrigger = true;
                audioComp.playOnAwake = false;
            }

            if(audioComp.playTrigger) {
                if(audioComp.audioHandle.IsValid()) {
                    Tsukino::Core::Ref<Tsukino::Asset::IAsset> baseAsset = context->assetManager->Get(audioComp.audioHandle);
                    Tsukino::Core::Ref<Tsukino::Asset::AudioAsset> asset = std::dynamic_pointer_cast<Tsukino::Asset::AudioAsset>(baseAsset);
                    if(asset) {
                        context->audioManager->Play(*asset, audioComp.loop, audioComp.volume);
                        audioComp.isPlaying = true;
                    }
                }
                audioComp.playTrigger = false;
            }

            if(audioComp.stopTrigger) {
                if(audioComp.audioHandle.IsValid()) {
                    Tsukino::Core::Ref<Tsukino::Asset::IAsset> baseAsset = context->assetManager->Get(audioComp.audioHandle);
                    Tsukino::Core::Ref<Tsukino::Asset::AudioAsset> asset = std::dynamic_pointer_cast<Tsukino::Asset::AudioAsset>(baseAsset);
                    if(asset) {
                        context->audioManager->Stop(*asset);
                    }
                }
                audioComp.isPlaying   = false;
                audioComp.stopTrigger = false;
            }
        }
    }
}    // namespace Tsukino::BuiltIn::ECS

