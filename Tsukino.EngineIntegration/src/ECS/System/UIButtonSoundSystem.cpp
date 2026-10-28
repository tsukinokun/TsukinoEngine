//----------------------------------------------------------------------------
//! @file   UIButtonSoundSystem.cpp
//! @brief  画面UIのボタンの音を鳴らすシステムの実装
//----------------------------------------------------------------------------
#include <Tsukino/EngineIntegration/ECS/System/UIButtonSoundSystem.hpp>

#include <Tsukino/Audio/AudioManager.hpp>
#include <Tsukino/BuiltIn/ECS/Component/PointerTargetComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/UIButtonFeedbackComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/UIButtonSoundComponent.hpp>
#include <Tsukino/Core/ECS/Registry/Registry.hpp>
#include <Tsukino/Engine/Asset/AssetManager.hpp>
#include <Tsukino/Engine/Asset/Audio/AudioAsset.hpp>
#include <Tsukino/EngineIntegration/EngineContext.hpp>

// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {

    namespace {
        //----------------------------------------------------------------------------
        //! 音のアセットを1回鳴らします。
        //! @param  [in] context エンジンコンテキスト
        //! @param  [in] sound   鳴らす音（無効なら何もしない）
        //! @param  [in] volume  音量
        //! @param  [in] bus     鳴らすバス
        //----------------------------------------------------------------------------
        void PlayOnce(Tsukino::EngineIntegration::EngineContext& context, const Tsukino::Asset::AssetRef& sound, float volume, u8 bus) {
            if(!sound.IsValid()) {
                return;
            }
            const auto asset = std::dynamic_pointer_cast<Tsukino::Asset::AudioAsset>(context.assetManager->Get(sound.handle));
            if(asset) {
                context.audioManager->Play(*asset, false, volume, bus);
            }
        }
    }    // namespace

    //----------------------------------------------------------------------------
    //! ボタンの音を鳴らします。
    //----------------------------------------------------------------------------
    void UIButtonSoundSystem::Update(Tsukino::ECS::Registry& registry, float /*deltaTime*/) {
        Tsukino::EngineIntegration::EngineContext* context = registry.GetContext<Tsukino::EngineIntegration::EngineContext*>();
        if(!context || !context->assetManager || !context->audioManager) {
            return;
        }

        registry.View<PointerTargetComponent, UIButtonSoundComponent>().each(
            [&](Tsukino::ECS::Entity entity, const PointerTargetComponent& target, const UIButtonSoundComponent& sound) {
                if(!target.clicked && !target.hoverEntered) {
                    return;
                }
                // 押せない状態のボタンは、見た目の手ごたえと同じく音も出さない
                if(const auto* feedback = registry.try_get<UIButtonFeedbackComponent>(entity); feedback && !feedback->interactable) {
                    return;
                }
                if(target.clicked) {
                    PlayOnce(*context, sound.clickSound, sound.clickVolume, sound.bus);
                } else {
                    PlayOnce(*context, sound.hoverSound, sound.hoverVolume, sound.bus);
                }
            });
    }
}    // namespace Tsukino::BuiltIn::ECS
