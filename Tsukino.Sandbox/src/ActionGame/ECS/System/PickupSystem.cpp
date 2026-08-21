//-------------------------------------------------------------
//! @file   PickupSystem.cpp
//! @brief  PickupSystemクラスの実装
//! @author 山﨑愛
//-------------------------------------------------------------
#include <Tsukino/Sandbox/ActionGame/ECS/System/PickupSystem.hpp>
#include <Tsukino/Sandbox/ActionGame/ECS/Component/PickupComponent.hpp>
#include <Tsukino/Sandbox/ActionGame/ECS/Component/PickupPromptComponent.hpp>
#include <Tsukino/Sandbox/ActionGame/ECS/Component/PlayerComponent.hpp>
#include <Tsukino/Sandbox/ActionGame/ECS/Component/WeaponComponent.hpp>

#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/FontComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/HighlightComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/WorldAnchorComponent.hpp>

#include <Tsukino/EngineIntegration/EngineContext.hpp>

#include <Tsukino/Core/Input/InputSystem.hpp>

#include <hlsl++.h>
#include <cfloat>
#include <cmath>
// 名前空間 : ActionGame::ECS
namespace ActionGame::ECS {
    namespace {
        constexpr float kHighlightBlendSpeed = 10.0f;    //!< highlightBlendが0↔1へ遷移する速さ（大きいほど素早く切り替わる）
        constexpr float kPulseSpeed          = 3.0f;     //!< 白発光の脈動速度（rad/sec相当）
        constexpr float kRimColorR           = 0.3f;     //!< ネオン風リムカラー（シアン系）
        constexpr float kRimColorG           = 0.9f;
        constexpr float kRimColorB           = 1.0f;
        constexpr float kRimIntensityMax     = 4.0f;     //!< 完全点灯時のリム強度
        constexpr float kRimPower            = 2.5f;     //!< リムの鋭さ
        constexpr float kGlowMin             = 0.05f;    //!< 白発光の脈動の下限
        constexpr float kGlowMax             = 0.35f;    //!< 白発光の脈動の上限

        constexpr float kFloatSpacing = 70.0f;     //!< 浮遊武器を横に並べる間隔（隣同士のx距離）
        constexpr float kFloatHeight  = 170.0f;    //!< 浮遊武器の高さ（既存の初期配置に合わせる）
        constexpr float kFloatDepth   = -20.0f;    //!< 浮遊武器の前後オフセット（既存の初期配置に合わせる）
    }    // namespace

    //-------------------------------------------------------------
    //! @brief システムの更新
    //-------------------------------------------------------------
    void PickupSystem::Update(Tsukino::ECS::Registry& registry, float deltaTime) {
        //-------------------------------------------------------------
        // コンテキストの取得
        //-------------------------------------------------------------
        Tsukino::EngineIntegration::EngineContext* ctx = registry.GetContext<Tsukino::EngineIntegration::EngineContext*>();
        if(!ctx || !ctx->inputSystem)
            return;

        Tsukino::Input::InputSystem* inputSystem = ctx->inputSystem;

        //-------------------------------------------------------------
        // プレイヤーを取得（単一プレイヤー前提）
        //-------------------------------------------------------------
        entt::entity     playerEntity   = entt::null;
        PlayerComponent* player         = nullptr;
        hlslpp::float3   playerPosition = hlslpp::float3(0.0f, 0.0f, 0.0f);

        auto playerView = registry.View<PlayerComponent, Tsukino::BuiltIn::ECS::TransformComponent>();
        for(auto entity : playerView) {
            playerEntity   = entity;
            player         = &playerView.get<PlayerComponent>(entity);
            playerPosition = playerView.get<Tsukino::BuiltIn::ECS::TransformComponent>(entity).position;
            break;
        }

        if(playerEntity == entt::null || !player)
            return;

        //-------------------------------------------------------------
        // 浮遊武器同士が同じ位置に重ならないよう、インベントリ内の並び順(index)と
        // 総数(count)から横方向の位置を割り出して毎フレーム配置し直す。
        // 個数が変わった瞬間もCombatSystem側の指数追従（attachPositionLerpSpeed）で
        // 新しい位置へ滑らかに移動するため、ここで値を書き換えるだけでよい
        //-------------------------------------------------------------
        {
            int weaponCount = static_cast<int>(player->weaponInventory.size());
            for(int i = 0; i < weaponCount; ++i) {
                entt::entity weaponEntity = player->weaponInventory[i];
                if(!registry.HasComponent<WeaponComponent>(weaponEntity))
                    continue;

                // count等分した位置に中央揃えで並べる（例: 2本なら-35, +35）
                float             offsetX = (static_cast<float>(i) - (weaponCount - 1) * 0.5f) * kFloatSpacing;
                WeaponComponent& weapon   = registry.GetComponent<WeaponComponent>(weaponEntity);
                weapon.localOffset        = hlslpp::float3(offsetX, kFloatHeight, kFloatDepth);
            }
        }

        //-------------------------------------------------------------
        // 拾える対象のうち、プレイヤーに最も近い1つだけを選ぶ
        // （範囲内に複数あっても常に1つだけを拾える対象にするための絞り込み）
        //-------------------------------------------------------------
        entt::entity nearest         = entt::null;
        float        nearestDistance = FLT_MAX;

        auto pickupView = registry.View<PickupComponent, Tsukino::BuiltIn::ECS::TransformComponent>();
        pickupView.each([&](entt::entity entity, PickupComponent& pickup, Tsukino::BuiltIn::ECS::TransformComponent& transform) {
            hlslpp::float3 toItem = transform.position - playerPosition;
            toItem.y              = 0.0f;    // 高さのずれで拾えなくならないよう水平距離のみで判定する

            float distance = hlslpp::length(toItem);
            if(distance <= pickup.radius && distance < nearestDistance) {
                nearestDistance = distance;
                nearest         = entity;
            }
        });
        player->pickupTarget = nearest;

        //-------------------------------------------------------------
        // 全ての拾えるアイテムのハイライト演出を更新する。
        // 対象になっているものだけ0→1へ、それ以外は1→0へ滑らかに戻す
        //-------------------------------------------------------------
        pickupView.each([&](entt::entity entity, PickupComponent& pickup, Tsukino::BuiltIn::ECS::TransformComponent& transform) {
            float target = (entity == nearest) ? 1.0f : 0.0f;
            float t      = 1.0f - std::exp(-kHighlightBlendSpeed * deltaTime);
            pickup.highlightBlend += (target - pickup.highlightBlend) * t;
            pickup.pulseTime += deltaTime;

            // 0→1→0を往復する脈動。sinを2乗して滑らかな山にする
            float wave  = std::sin(pickup.pulseTime * kPulseSpeed);
            float pulse = wave * wave;

            if(auto* highlight = registry.try_get<Tsukino::BuiltIn::ECS::HighlightComponent>(entity)) {
                highlight->active       = pickup.highlightBlend > 0.001f;
                highlight->rimColor     = hlslpp::float3(kRimColorR, kRimColorG, kRimColorB);
                highlight->rimIntensity = kRimIntensityMax * pickup.highlightBlend;
                highlight->rimPower     = kRimPower;
                highlight->glow         = (kGlowMin + (kGlowMax - kGlowMin) * pulse) * pickup.highlightBlend;
            }
        });

        //-------------------------------------------------------------
        // 「Fキーで拾う」UIラベルの更新（対象がいるときだけ表示する）。
        // 座標計算そのものはWorldAnchorSystemに任せ、ここではtarget/worldOffsetの
        // 設定とテキストの更新だけを行う
        //-------------------------------------------------------------
        auto promptView =
            registry.View<PickupPromptComponent, Tsukino::BuiltIn::ECS::FontComponent, Tsukino::BuiltIn::ECS::WorldAnchorComponent>();
        promptView.each([&](entt::entity, PickupPromptComponent& prompt, Tsukino::BuiltIn::ECS::FontComponent& promptFont,
                            Tsukino::BuiltIn::ECS::WorldAnchorComponent& promptAnchor) {
            if(nearest == entt::null || !registry.HasComponent<PickupComponent>(nearest)) {
                promptAnchor.target = entt::null;
                promptFont.text.clear();    // 空文字ならFontRendererSystemが描画をスキップする
                return;
            }

            const auto& pickup = registry.GetComponent<PickupComponent>(nearest);

            promptAnchor.target       = nearest;
            promptAnchor.worldOffset  = hlslpp::float3(0.0f, pickup.labelHeight, 0.0f);
            promptAnchor.screenOffset = hlslpp::float2(0.0f, prompt.screenOffsetY);
            promptFont.text           = L"F : " + pickup.displayName + L" を拾う";
        });

        //-------------------------------------------------------------
        // Fキーで取得する。反復中にコンポーネント構成を変えるとViewが壊れるため、
        // 上の絞り込み・演出更新が終わった後にここでまとめて行う
        //-------------------------------------------------------------
        if(nearest != entt::null && inputSystem->IsKeyPressed(Tsukino::Input::KeyCode::F)) {
            if(registry.HasComponent<WeaponComponent>(nearest)) {
                // 拾った武器は「所有者つきの浮遊武器」へ昇格させる
                WeaponComponent& weapon = registry.GetComponent<WeaponComponent>(nearest);
                weapon.owner            = playerEntity;
                weapon.floatEnabled     = true;

                player->weaponInventory.push_back(nearest);
            }

            // 演出とワールド判定を止める（PickupComponentを外すのでこれ以降候補に上がらない）
            if(auto* highlight = registry.try_get<Tsukino::BuiltIn::ECS::HighlightComponent>(nearest)) {
                highlight->active = false;
            }
            registry.RemoveComponent<PickupComponent>(nearest);
            player->pickupTarget = entt::null;
        }
    }
}    // namespace ActionGame::ECS
