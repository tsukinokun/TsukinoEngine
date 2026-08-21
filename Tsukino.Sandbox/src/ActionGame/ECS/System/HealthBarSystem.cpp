//-------------------------------------------------------------
//! @file   HealthBarSystem.cpp
//! @brief  HealthBarSystemクラスの実装
//-------------------------------------------------------------
#include <Tsukino/Sandbox/ActionGame/ECS/System/HealthBarSystem.hpp>
#include <Tsukino/Sandbox/ActionGame/ECS/Component/HealthComponent.hpp>

#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/WorldAnchorComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/SpriteComponent.hpp>

#include <hlsl++.h>
#include <entt/entt.hpp>

#include <algorithm>
// 名前空間 : ActionGame::ECS
namespace ActionGame::ECS {
    namespace {
        // HPバー用テクスチャ（1辺kBarTexturePixelSizeの単色正方形）をこのピクセルサイズまで引き伸ばして表示する
        constexpr float kBarTexturePixelSize = 4.0f;
        constexpr float kBarWidthPixels      = 60.0f;    //!< バー全体（満タン時）の幅
        constexpr float kBarHeightPixels     = 8.0f;     //!< バーの高さ

        constexpr float kFullScaleX = kBarWidthPixels / kBarTexturePixelSize;
        constexpr float kFullScaleY = kBarHeightPixels / kBarTexturePixelSize;
    }    // namespace

    //-------------------------------------------------------------
    //! @brief システムの更新
    //-------------------------------------------------------------
    void HealthBarSystem::Update(Tsukino::ECS::Registry& registry, float deltaTime) {
        auto view = registry.View<HealthComponent>();
        view.each([&](entt::entity, HealthComponent& health) {
            if(health.hpBarBackgroundEntity == entt::null || health.hpBarFillEntity == entt::null)
                return;    // HPバーを持たないエンティティ（プレイヤー等）は対象外

            if(!registry.IsValid(health.hpBarBackgroundEntity) || !registry.IsValid(health.hpBarFillEntity))
                return;

            //-------------------------------------------------------------
            // 表示タイマーの更新（被弾時にCombatSystemがhpBarVisibleTimerをセットする）
            //-------------------------------------------------------------
            if(health.hpBarVisibleTimer > 0.0f) {
                health.hpBarVisibleTimer = std::max(0.0f, health.hpBarVisibleTimer - deltaTime);
            }

            bool show = health.hpBarVisibleTimer > 0.0f && !health.isDead;

            float ratio = health.maxHealth > 0.0f ? std::clamp(health.currentHealth / health.maxHealth, 0.0f, 1.0f) : 0.0f;

            //-------------------------------------------------------------
            // 背景バー：常に満タン幅のまま、表示/非表示だけscaleで切り替える
            //-------------------------------------------------------------
            Tsukino::BuiltIn::ECS::TransformComponent& backgroundTransform =
                registry.GetComponent<Tsukino::BuiltIn::ECS::TransformComponent>(health.hpBarBackgroundEntity);
            backgroundTransform.scale = show ? hlslpp::float3(kFullScaleX, kFullScaleY, 1.0f) : hlslpp::float3(0.0f, 0.0f, 0.0f);

            //-------------------------------------------------------------
            // 残量バー：ratio分だけ幅を縮める。中央基準で縮むと右端しか動かないように見えるため、
            // WorldAnchorComponent.screenOffsetを左へずらして左端を固定し、右端だけが減っていくようにする
            //-------------------------------------------------------------
            Tsukino::BuiltIn::ECS::TransformComponent& fillTransform =
                registry.GetComponent<Tsukino::BuiltIn::ECS::TransformComponent>(health.hpBarFillEntity);
            fillTransform.scale = show ? hlslpp::float3(kFullScaleX * ratio, kFullScaleY, 1.0f) : hlslpp::float3(0.0f, 0.0f, 0.0f);

            if(auto* fillAnchor = registry.try_get<Tsukino::BuiltIn::ECS::WorldAnchorComponent>(health.hpBarFillEntity)) {
                fillAnchor->screenOffset.x = -(kBarWidthPixels * (1.0f - ratio) * 0.5f);
            }

            //-------------------------------------------------------------
            // 残量に応じてtintColorを緑→赤へ補間する
            //-------------------------------------------------------------
            if(auto* fillSprite = registry.try_get<Tsukino::BuiltIn::ECS::SpriteComponent>(health.hpBarFillEntity)) {
                fillSprite->tintColor = hlslpp::float4(1.0f - ratio, ratio, 0.0f, 1.0f);
            }
        });
    }
}    // namespace ActionGame::ECS
