//-------------------------------------------------------------
//! @file   PenguinAnimationSystem.hpp
//! @brief  PenguinAnimationSystemクラスの宣言
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once
#include <Tsukino/BuiltIn/ECS/Component/SpriteComponent.hpp>

#include <Tsukino/Sandbox/PenguinGame/ECS/Components/PenguinAnimatorComponent.hpp>
#include <Tsukino/Sandbox/PenguinGame/ECS/Systems/PenguinAnimationSystem.hpp>

#include <Tsukino/EngineIntegration/EngineContext.hpp>

#include <Tsukino/Core/Input/InputSystem.hpp>
#include <Tsukino/Core/Input/KeyCodes.hpp>
namespace PenguinGame::ECS {
    //-------------------------------------------------------------
    //! @brief  更新処理
    //-------------------------------------------------------------
    void PenguinAnimationSystem::Update(Tsukino::ECS::Registry& registry, float deltaTime) {
        //-------------------------------------------------------------
        // コンテキストの取得
        //-------------------------------------------------------------
        Tsukino::EngineIntegration::EngineContext* ctx = registry.GetContext<Tsukino::EngineIntegration::EngineContext*>();
        if(!ctx)
            return;

        //-------------------------------------------------------------
        // インプットシステムの取得
        //-------------------------------------------------------------
        Tsukino::Input::InputSystem* input = ctx->inputSystem;

        auto view = registry.View<PenguinAnimatorComponent>();

        view.each([&](auto& animator) {
            if(!animator.currentSkin)
                return;    // スキンが設定されていなければ何もしない

            // まばたき処理 (時間経過)
            animator.blinkTimer += deltaTime;
            if(animator.blinkTimer >= animator.blinkInterval) {
                animator.blinkTimer     = 0.0f;
                animator.isCenterFrame2 = !animator.isCenterFrame2;
            }

            // 手のアニメーション処理
            if(input->IsKeyPressed(Tsukino::Input::KeyCode::Space)) {
                if(!animator.lastHandWasLeft) {    // 次は左を動かす
                    animator.isLeftFrame2    = true;
                    animator.leftActionTimer = animator.actionDuration;
                } else {    // 次は右を動かす
                    animator.isRightFrame2    = true;
                    animator.rightActionTimer = animator.actionDuration;
                }
                animator.lastHandWasLeft = !animator.lastHandWasLeft;
            }

            // タイマーによる戻し処理
            if(animator.isLeftFrame2) {
                animator.leftActionTimer -= deltaTime;
                if(animator.leftActionTimer <= 0)
                    animator.isLeftFrame2 = false;
            }
            if(animator.isRightFrame2) {
                animator.rightActionTimer -= deltaTime;
                if(animator.rightActionTimer <= 0)
                    animator.isRightFrame2 = false;
            }

            // スプライトの更新
            auto updateSprite = [&](Tsukino::ECS::Entity e, Tsukino::Asset::AssetHandle handle) {
                auto& sprite         = registry.GetComponent<Tsukino::BuiltIn::ECS::SpriteComponent>(e);
                sprite.textureHandle = handle;
            };

            updateSprite(animator.centerEntity, animator.currentSkin->centerTexs[animator.isCenterFrame2 ? 1 : 0]);

            updateSprite(animator.leftHandEntity, animator.currentSkin->leftTexs[animator.isLeftFrame2 ? 1 : 0]);

            updateSprite(animator.rightHandEntity, animator.currentSkin->rightTexs[animator.isRightFrame2 ? 1 : 0]);
        });
    }
}    // namespace PenguinGame::ECS
