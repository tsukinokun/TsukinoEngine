//-------------------------------------------------------------
//! @file   PlatformGeneratorSystem.cpp
//! @brief  PlatformGeneratorSystemクラスの実装
//! @author 山﨑愛
//-------------------------------------------------------------
#include <Tsukino/Sandbox/JumpGameSample/ECS/System/PlatformGeneratorSystem.hpp>
#include <Tsukino/Sandbox/JumpGameSample/ECS/Component/PlatformComponent.hpp>
#include <Tsukino/Sandbox/JumpGameSample/ECS/Component/PlatformGeneratorComponent.hpp>
#include <Tsukino/Sandbox/JumpGameSample/ECS/Component/LandedOnPlatformComponent.hpp>

#include <Tsukino/Sandbox/JumpGameSample/ECS/State/GameState.hpp>

#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/RigidbodyComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/CollisionComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/ModelComponent.hpp>

#include <Tsukino/EngineIntegration/EngineContext.hpp>

#include <Tsukino/Engine/Asset/AssetManager.hpp>

#include <Tsukino/Core/Input/InputSystem.hpp>
#include <Tsukino/Core/Path.hpp>

#include <hlsl++.h>
// 名前空間 : JumpGameSample::ECS
namespace JumpGameSample::ECS {
    //-------------------------------------------------------------
    //! @brief システムの更新
    //-------------------------------------------------------------
    void PlatformGeneratorSystem::Update(Tsukino::ECS::Registry& registry, float deltaTime) {
        //-------------------------------------------------------------
        // コンテキストの取得
        //-------------------------------------------------------------
        Tsukino::EngineIntegration::EngineContext* ctx = registry.GetContext<Tsukino::EngineIntegration::EngineContext*>();
        if(!ctx)
            return;

        // 稼働中のPlatform（isMoving == true）を探す
        auto movingPlatforms     = registry.View<PlatformComponent>();
        bool isAnyPlatformMoving = false;

        for(auto entity : movingPlatforms) {
            if(movingPlatforms.get<PlatformComponent>(entity).isMoving) {
                isAnyPlatformMoving = true;
                break;
            }
        }

        // もし動いているものがなければ、Generatorを見て新規生成
        if(!isAnyPlatformMoving) {
            auto generators = registry.View<PlatformGeneratorComponent>();
            for(auto entity : generators) {
                auto&                          gen   = generators.get<PlatformGeneratorComponent>(entity);
                JumpGameSample::ECS::GameState state = registry.GetContext<JumpGameSample::ECS::GameState>();
                if(state != JumpGameSample::ECS::GameState::Playing) {
                    continue;    // ゲームがプレイ中でなければ生成しない
                }
                SpawnNewPlatform(registry, gen.spawnDistance);     // 生成関数
                constexpr float spawnDistance  = 20.0f;            // 土台同士の距離
                gen.spawnDistance             += spawnDistance;    // 次の土台が出るまでの距離を更新
            }
        }
    }

    //-------------------------------------------------------------
    //! @brief  新しい土台の生成
    //-------------------------------------------------------------
    void PlatformGeneratorSystem::SpawnNewPlatform(Tsukino::ECS::Registry& registry, float offsetY) {
        //-------------------------------------------------------------
        // コンテキストの取得
        //-------------------------------------------------------------
        Tsukino::EngineIntegration::EngineContext* ctx = registry.GetContext<Tsukino::EngineIntegration::EngineContext*>();
        if(!ctx)
            return;

        Tsukino::ECS::Entity entity = registry.CreateEntity();

        // ランダムな初期位置と速度の設定
        float randomDirection = (rand() % 2 == 0) ? 1.0f : -1.0f;    // 1.0(右始動) or -1.0(左始動)

        // TransformComponent の追加と初期化
        Tsukino::BuiltIn::ECS::TransformComponent& platformTransform = registry.AddComponent<Tsukino::BuiltIn::ECS::TransformComponent>(entity);
        platformTransform.position                                   = hlslpp::float3(300.0f * randomDirection, offsetY, 0.0f);
        platformTransform.rotation                                   = hlslpp::quaternion(0.0f, 0.0f, 0.0f, 1.0f);    // 無回転
        platformTransform.scale                                      = hlslpp::float3(1.0f, 1.0f, 1.0f);              // 土台
        platformTransform.dirty                                      = true;                                          // 初回計算のためフラグを立てる
        platformTransform.parent                                     = entt::null;                                    // 親なし

        // ModelComponent の追加
        Tsukino::BuiltIn::ECS::ModelComponent& model = registry.AddComponent<Tsukino::BuiltIn::ECS::ModelComponent>(entity);
        model.modelHandle                            = ctx->assetManager->Load(Tsukino::Core::Path("Tsukino.Sandbox/Assets/JumpGameSample/Models/Block.fbx"));
        model.visible                                = true;

        // コリジョンを追加
        Tsukino::BuiltIn::ECS::CollisionComponent& collision = registry.AddComponent<Tsukino::BuiltIn::ECS::CollisionComponent>(entity);
        collision.extent                                     = hlslpp::float3(50.0f, 10.0f, 50.0f);    // 土台の当たり判定
        collision.isSensor                                   = false;                                  // 衝突判定を有効にする

        // RBをつける
        Tsukino::BuiltIn::ECS::RigidbodyComponent& rb = registry.AddComponent<Tsukino::BuiltIn::ECS::RigidbodyComponent>(entity);
        rb.type                                       = Tsukino::BuiltIn::ECS::RigidbodyType::Kinematic;    // 動く床なので Kinematic にする

        // PlatformComponent の追加
        JumpGameSample::ECS::PlatformComponent& platform = registry.AddComponent<JumpGameSample::ECS::PlatformComponent>(entity);
        platform.speed                                   = 100.0f * randomDirection;    // 土台の移動速度
        platform.isMoving                                = true;                        // 移動中フラグを立てる
    }
}    // namespace JumpGameSample::ECS
