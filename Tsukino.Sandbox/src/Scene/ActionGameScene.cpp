//-------------------------------------------------------------
//! @file    ActionGameScene.cpp
//! @brief   サンプルシーン1の実装
//! @author  山﨑愛
//-------------------------------------------------------------
#include <Tsukino/Sandbox/Scene/ActionGameScene.hpp>

#include <Tsukino/Sandbox/ActionGame/ECS/Component/PlayerComponent.hpp>
#include <Tsukino/Sandbox/ActionGame/ECS/Component/HealthComponent.hpp>
#include <Tsukino/Sandbox/ActionGame/ECS/Component/WeaponComponent.hpp>
#include <Tsukino/Sandbox/ActionGame/ECS/Component/EnemyComponent.hpp>
#include <Tsukino/Sandbox/ActionGame/ECS/Component/TpsCameraComponent.hpp>
#include <Tsukino/Sandbox/ActionGame/ECS/Component/PlayerAnimationSetComponent.hpp>
#include <Tsukino/Sandbox/ActionGame/ECS/System/PlayerSystem.hpp>
#include <Tsukino/Sandbox/ActionGame/ECS/System/CombatSystem.hpp>
#include <Tsukino/Sandbox/ActionGame/ECS/System/EnemySystem.hpp>
#include <Tsukino/Sandbox/ActionGame/ECS/System/TpsCameraSystem.hpp>
#include <Tsukino/Sandbox/ActionGame/ECS/System/PlayerAnimationSystem.hpp>

#include <Tsukino/EngineIntegration/EngineAPI.hpp>
#include <Tsukino/EngineIntegration/EngineContext.hpp>
#include <Tsukino/Engine/Asset/AssetManager.hpp>

#include <Tsukino/Core/Path.hpp>
#include <Tsukino/Core/Log.hpp>
#include <Tsukino/Core/DebugTools/DebugFeatures.hpp>

// 必要なシステムとコンポーネントのインクルード
#include <Tsukino/EngineIntegration/ECS/System/TransformSystem.hpp>
#include <Tsukino/EngineIntegration/ECS/System/CameraSystem.hpp>
#include <Tsukino/EngineIntegration/ECS/System/SpriteRendererSystem.hpp>
#include <Tsukino/EngineIntegration/ECS/System/FontRendererSystem.hpp>
#include <Tsukino/EngineIntegration/ECS/System/AudioSystem.hpp>
#include <Tsukino/EngineIntegration/ECS/System/PhysicsSystem.hpp>
#include <Tsukino/EngineIntegration/ECS/System/ModelSystem.hpp>
#include <Tsukino/EngineIntegration/ECS/System/AnimationSystem.hpp>
#include <Tsukino/EngineIntegration/ECS/System/DirectionalLightSystem.hpp>
#include <Tsukino/EngineIntegration/ECS/System/SkyAtmosphereSystem.hpp>
#include <Tsukino/EngineIntegration/ECS/System/DebugCameraSystem.hpp>
#include <Tsukino/EngineIntegration/ECS/System/EffectSystem.hpp>

#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/CameraComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/SpriteComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/FontComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/AudioComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/ModelComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/AnimationPlayerComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/SkeletonOutputComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/CollisionComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/RigidBodyComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/CharacterControllerComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/AnimationControllerComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/DirectionalLightComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/SkyAtmosphereComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/SpringBoneComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/DebugCameraComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/DebugCameraTag.hpp>
#include <Tsukino/BuiltIn/ECS/Component/EffectComponent.hpp>

#include <entt/entt.hpp>
#include <hlsl++.h>
// 名前空間 : Tsukino::Sandbox
namespace Tsukino::Sandbox {
    //-------------------------------------------------------------
    //! @brief  シーン固有の初期化処理
    //-------------------------------------------------------------
    void ActionGameScene::OnInitialize(Tsukino::EngineIntegration::EngineAPI& api) {
        //-------------------------------------------------------------
        // コンテキストをレジストリから取得
        //-------------------------------------------------------------
        Tsukino::EngineIntegration::EngineContext* context = m_scene.GetRegistry().GetContext<Tsukino::EngineIntegration::EngineContext*>();
        //-------------------------------------------------------------
        // イベントバスをレジストリから取得
        //-------------------------------------------------------------
        Tsukino::ECS::EventBus& eventBus = m_scene.GetEventBus();

        //--------------------------------------------------------------
        // クリアカラーを透明に設定
        //--------------------------------------------------------------
        context->renderer->SetClearColor(0.0f, 0.0f, 0.0f, 0.0f);

        //--------------------------------------------------------------
        // システムの生成と追加
        //--------------------------------------------------------------
        // Transformは一番最初に計算する (優先度 0)
        m_scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::TransformSystem>(), 0);
        // プレイヤーの入力はTransformの後、Physicsの前に反映する (優先度 1)
        m_scene.AddSystem(std::make_shared<ActionGame::ECS::PlayerSystem>(), 1);
        // 敵の追跡AI（移動）もプレイヤー入力と同じタイミングで反映する (優先度 1)
        m_scene.AddSystem(std::make_shared<ActionGame::ECS::EnemySystem>(), 1);
        // 武器の追従・攻撃判定・ダメージ処理はプレイヤー/敵の移動が確定した後に行う (優先度 2)
        m_scene.AddSystem(std::make_shared<ActionGame::ECS::CombatSystem>(), 2);
        // プレイヤーのアニメーションステートマシン。移動確定後、AnimationSystemに切替要求が
        // 反映されるようアニメーション更新の前後どちらでも良い程度の位置に置く (優先度 2)
        m_scene.AddSystem(std::make_shared<ActionGame::ECS::PlayerAnimationSystem>(), 2);
        // アニメーションはTransformの後に更新する (優先度 2)
        m_scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::AnimationSystem>(), 2);
        // TPSカメラの追従はプレイヤーの移動が確定した後、カメラ行列計算の前に行う (優先度 4)
        m_scene.AddSystem(std::make_shared<ActionGame::ECS::TpsCameraSystem>(), 4);
#ifdef _DEBUG
        m_scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::DebugCameraSystem>(), 4);
#endif
        // カメラは描画前に更新する (優先度 5)
        m_scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::CameraSystem>(), 5);
        // フォント描画 (優先度 9)
        m_scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::FontRendererSystem>(), 9);
        // スプライトなど描画用のコマンド生成は後で行う (優先度 10)
        m_scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::SpriteRenderSystem>(), 10);
        // モデル描画 (優先度 10)
        m_scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::ModelSystem>(), 10);
        // エフェクト描画 (優先度 10)
        {
            auto effectSystem = std::make_shared<Tsukino::BuiltIn::ECS::EffectSystem>();
            m_scene.AddSystem(effectSystem, 10);
            effectSystem->Initialize(m_scene.GetRegistry(), eventBus);
            context->effectSystem = effectSystem.get();
        }
        // オーディオの更新 (優先度 11)
        m_scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::AudioSystem>(), 11);
        // コリジョンの更新は最後に行う (優先度 12)
        {
            auto physicsSystem = std::make_shared<Tsukino::BuiltIn::ECS::PhysicsSystem>(eventBus);
#ifdef TSUKINO_DEBUG_COLLISION_DRAW
            // ActionGameSampleでは常にコリジョンのワイヤーフレームを表示する（F5で従来通りOFFも可能）
            physicsSystem->SetDebugDrawEnabled(true);
#endif
            m_scene.AddSystem(physicsSystem, 12);
        }
        // ライトの更新 (優先度 13)
        m_scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::DirectionalLightSystem>(), 13);
        // スカイアトモスフィアの更新 (優先度 14)
        m_scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::SkyAtmosphereSystem>(), 14);

        //--------------------------------------------------------------
        // アセットのロード
        //--------------------------------------------------------------

        Tsukino::Asset::AssetHandle modelHandle = context->assetManager->Load(Tsukino::Core::Path("Tsukino.Sandbox/Assets/ActionGameSample/Models/CharaTest.fbx"));

        Tsukino::Asset::AssetHandle animationHandle = context->assetManager->Load(Tsukino::Core::Path("Tsukino.Sandbox/Assets/ActionGameSample/Anims/Jump.fbx"));

        // プレイヤーのアニメーションステートマシン（PlayerAnimationSystem）が使うクリップ
        Tsukino::Asset::AssetHandle idleAnimHandle    = context->assetManager->Load(Tsukino::Core::Path("Tsukino.Sandbox/Assets/ActionGameSample/Anims/Idle.fbx"));
        Tsukino::Asset::AssetHandle runAnimHandle     = context->assetManager->Load(Tsukino::Core::Path("Tsukino.Sandbox/Assets/ActionGameSample/Anims/Run.fbx"));
        Tsukino::Asset::AssetHandle fastRunAnimHandle = context->assetManager->Load(Tsukino::Core::Path("Tsukino.Sandbox/Assets/ActionGameSample/Anims/Fast Run.fbx"));

        Tsukino::ECS::Registry& registry = m_scene.GetRegistry();

        //--------------------------------------------------------------
        // 地面エンティティ
        //--------------------------------------------------------------
        {
            Tsukino::ECS::Entity groundEntity = m_scene.CreateEntity();
            // TransformComponent の追加と初期化
            // JumpGameSample等と同じ「1ユニット≒1cm」規約。半径5の薄い床にして、上面がちょうどy=0に来るよう中心をy=-5に置く
            Tsukino::BuiltIn::ECS::TransformComponent& groundTransform = registry.AddComponent<Tsukino::BuiltIn::ECS::TransformComponent>(groundEntity);
            groundTransform.position                                   = hlslpp::float3(0.0f, -5.0f, 0.0f);
            groundTransform.rotation                                   = hlslpp::quaternion(0.0f, 0.0f, 0.0f, 1.0f);    // 無回転
            groundTransform.scale                                      = hlslpp::float3(1.0f, 1.0f, 1.0f);
            groundTransform.dirty                                      = true;          // 初回計算のためフラグを立てる
            groundTransform.parent                                     = entt::null;    // 親なし

            // コリジョンをつける（一辺1000 x 厚さ10の床）
            Tsukino::BuiltIn::ECS::CollisionComponent& collision = registry.AddComponent<Tsukino::BuiltIn::ECS::CollisionComponent>(groundEntity);
            collision.extent                                     = {500.0f, 5.0f, 500.0f};
            collision.type                                       = Tsukino::BuiltIn::ECS::ColliderType::Box;
            collision.isSensor                                   = false;    // 明示的にソリッド判定にする（デフォルトも今はfalse）
        
            // RBをつける
            Tsukino::BuiltIn::ECS::RigidbodyComponent& rb = registry.AddComponent<Tsukino::BuiltIn::ECS::RigidbodyComponent>(groundEntity);
            rb.type                                       = Tsukino::BuiltIn::ECS::RigidbodyType::Static;
        }

        //--------------------------------------------------------------
        // プレイヤーエンティティ生成
        //--------------------------------------------------------------
        Tsukino::ECS::Entity playerEntity = m_scene.CreateEntity();

        // TransformComponent の追加と初期化
        // CharacterControllerComponent.centerOffsetを使うため、position＝カプセル底面（足元/接地位置）
        // を表す。地面の上面はy=0なので、埋まった状態で出現しないよう少し余裕を持たせてy=5から開始する
        Tsukino::BuiltIn::ECS::TransformComponent& playerTransform = registry.AddComponent<Tsukino::BuiltIn::ECS::TransformComponent>(playerEntity);
        playerTransform.position                                   = hlslpp::float3(0.0f, 5.0f, 0.0f);
        playerTransform.rotation                                   = hlslpp::quaternion(0.0f, 0.0f, 0.0f, 1.0f);    // 無回転
        // CharaTest.fbxの実寸を計測したところ身長はY=0〜100（足元がローカルY=0）で、想定していた
        // 「身長約210」の半分以下だったため、2.1倍(=210/100)スケールして合わせる
        playerTransform.scale  = hlslpp::float3(2.1f, 2.1f, 2.1f);
        playerTransform.dirty  = true;          // 初回計算のためフラグを立てる
        playerTransform.parent = entt::null;    // 親なし

        // プレイヤーとして動かすためCharacterControllerComponentをつける
        // （JumpGameSampleのカプセル(radius=35, halfHeight=70)と同じ規約に合わせる。
        //   CharacterVirtualの重力計算は手動なので、gravityFactorで底上げしないとほぼ落下しない）
        Tsukino::BuiltIn::ECS::CharacterControllerComponent& characterController =
            registry.AddComponent<Tsukino::BuiltIn::ECS::CharacterControllerComponent>(playerEntity);
        characterController.radius        = 35.0f;
        characterController.halfHeight    = 70.0f;
        characterController.maxSlopeDeg   = 45.0f;
        characterController.gravityFactor = 100.0f;    // 1ユニット=1cm換算でほぼ実重力(9.81m/s^2)相当
        characterController.jumpSpeed     = 300.0f;    // 約45cm跳ぶ想定（v^2 / (2*981)）
        // カプセル中心をTransform位置から (halfHeight+radius) だけ上にずらし、
        // Transform位置＝カプセル底面（足元）を表すようにする（モデルの足元原点と揃えるため）
        characterController.centerOffset = hlslpp::float3(0.0f, characterController.halfHeight + characterController.radius, 0.0f);

        // プレイヤーコンポーネントをつける（PlayerSystemが入力を読み取るための目印）
        ActionGame::ECS::PlayerComponent& player = registry.AddComponent<ActionGame::ECS::PlayerComponent>(playerEntity);

        // HPを持たせる（Phase A: 敵の接触ダメージ計算に使用）
        registry.AddComponent<ActionGame::ECS::HealthComponent>(playerEntity);

        // ModelComponent の追加
        Tsukino::BuiltIn::ECS::ModelComponent& model = registry.AddComponent<Tsukino::BuiltIn::ECS::ModelComponent>(playerEntity);
        model.modelHandle                            = modelHandle;
        model.visible                                = true;

        // アニメーションを再生・制御するコンポーネント（初期状態はIdle。以後はPlayerAnimationSystemが管理する）
        Tsukino::BuiltIn::ECS::AnimationPlayerComponent& animPlayer = registry.AddComponent<Tsukino::BuiltIn::ECS::AnimationPlayerComponent>(playerEntity);
        animPlayer.current_clip_id                                  = idleAnimHandle;
        animPlayer.animation_index                                  = 0;
        animPlayer.elapsed_time                                     = 0.0f;
        animPlayer.playback_speed                                   = 1.0f;
        animPlayer.is_looping                                       = true;    // ループさせる
        animPlayer.is_playing                                       = true;    // 再生状態にする

        // クリップの切り替え（AnimationSystemが読む「次に再生するクリップ」の受け皿）
        registry.AddComponent<Tsukino::BuiltIn::ECS::AnimationControllerComponent>(playerEntity);

        // PlayerAnimationSystemが参照する、ステートごとのアニメーションクリップ一式
        ActionGame::ECS::PlayerAnimationSetComponent& animSet = registry.AddComponent<ActionGame::ECS::PlayerAnimationSetComponent>(playerEntity);
        animSet.idleClip                                       = idleAnimHandle;
        animSet.runClip                                        = runAnimHandle;
        animSet.fastRunClip                                    = fastRunAnimHandle;
        animSet.jumpClip                                       = animationHandle;
        animSet.currentState                                   = ActionGame::ECS::PlayerAnimState::Idle;

        Tsukino::BuiltIn::ECS::SpringBoneComponent& springBone = registry.AddComponent<Tsukino::BuiltIn::ECS::SpringBoneComponent>(playerEntity);

        Tsukino::BuiltIn::ECS::SpringBoneComponent::ChainDef breastL;
        breastL.name                   = "Breast_L";
        breastL.rootNodeName           = "Breast_L";
        breastL.maxDepth               = 1;
        breastL.settings.stiffness     = 0.35f;     // リアル(0.55)より少し柔らかく、揺れ幅を出す
        breastL.settings.drag          = 0.13f;    // 収まりをやや長めに（2〜3往復くらい残る）
        breastL.settings.inertia       = 0.5f;    // 体の動きに対して、わずかに「置いていかれる」感を演出
        breastL.settings.gravityScale  = 1.0f;
        breastL.settings.angleLimitDeg = 26.0f;
        springBone.chainDefs.push_back(breastL);

        Tsukino::BuiltIn::ECS::SpringBoneComponent::ChainDef breastR;
        breastR.name                   = "Breast_R";
        breastR.rootNodeName           = "Breast_R";
        breastR.maxDepth               = 1;
        breastR.settings.stiffness     = 0.35f;
        breastR.settings.drag          = 0.13f;
        breastR.settings.inertia       = 0.5f;
        breastR.settings.gravityScale  = 1.0f;
        breastR.settings.angleLimitDeg = 26.0f;
        springBone.chainDefs.push_back(breastR);

        // 計算されたボーン行列の出力先（スキニング用）コンポーネント
        Tsukino::BuiltIn::ECS::SkeletonOutputComponent& skeletonOutput = registry.AddComponent<Tsukino::BuiltIn::ECS::SkeletonOutputComponent>(playerEntity);

        //--------------------------------------------------------------
        // 武器エンティティ生成（Phase A: 本番の剣アセットが無いため、既存のPaddle.fbxを仮の剣として流用）
        // 位置はCombatSystemが毎フレーム所有者（プレイヤー）に追従させる
        //--------------------------------------------------------------
        {
            Tsukino::ECS::Entity weaponEntity = m_scene.CreateEntity();

            Tsukino::BuiltIn::ECS::TransformComponent& weaponTransform = registry.AddComponent<Tsukino::BuiltIn::ECS::TransformComponent>(weaponEntity);
            weaponTransform.position                                   = playerTransform.position;    // 初期値。以後CombatSystemが上書きする
            weaponTransform.rotation                                   = hlslpp::quaternion(0.0f, 0.0f, 0.0f, 1.0f);
            weaponTransform.scale                                      = hlslpp::float3(0.4f, 0.4f, 1.4f);    // 仮の剣らしいシルエットにする簡易スケール
            weaponTransform.dirty                                      = true;
            weaponTransform.parent                                     = entt::null;

            Tsukino::Asset::AssetHandle weaponModelHandle = context->assetManager->Load(Tsukino::Core::Path("Tsukino.Sandbox/Assets/ActionGameSample/Models/Paddle.fbx"));
            Tsukino::BuiltIn::ECS::ModelComponent& weaponModel = registry.AddComponent<Tsukino::BuiltIn::ECS::ModelComponent>(weaponEntity);
            weaponModel.modelHandle                            = weaponModelHandle;
            weaponModel.visible                                = true;

            ActionGame::ECS::WeaponComponent& weapon = registry.AddComponent<ActionGame::ECS::WeaponComponent>(weaponEntity);
            weapon.owner                              = playerEntity;

            // プレイヤーに装備中の武器エンティティを紐付ける（PlayerSystemが攻撃入力時に参照する）
            player.weaponEntity = weaponEntity;
        }

        //--------------------------------------------------------------
        // 敵エンティティ生成（Phase A: 本番の敵アセットが無いため、既存のBlock.fbxを仮の敵体として流用）
        //--------------------------------------------------------------
        auto spawnEnemy = [&](hlslpp::float3 spawnPosition, float moveSpeed, float maxHealth) {
            Tsukino::ECS::Entity enemyEntity = m_scene.CreateEntity();

            Tsukino::BuiltIn::ECS::TransformComponent& enemyTransform = registry.AddComponent<Tsukino::BuiltIn::ECS::TransformComponent>(enemyEntity);
            enemyTransform.position                                   = spawnPosition;
            enemyTransform.rotation                                   = hlslpp::quaternion(0.0f, 0.0f, 0.0f, 1.0f);
            enemyTransform.scale                                      = hlslpp::float3(1.5f, 1.5f, 1.5f);
            enemyTransform.dirty                                      = true;
            enemyTransform.parent                                     = entt::null;

            Tsukino::Asset::AssetHandle enemyModelHandle = context->assetManager->Load(Tsukino::Core::Path("Tsukino.Sandbox/Assets/ActionGameSample/Models/Block.fbx"));
            Tsukino::BuiltIn::ECS::ModelComponent& enemyModel = registry.AddComponent<Tsukino::BuiltIn::ECS::ModelComponent>(enemyEntity);
            enemyModel.modelHandle                            = enemyModelHandle;
            enemyModel.visible                                = true;

            ActionGame::ECS::EnemyComponent& enemy = registry.AddComponent<ActionGame::ECS::EnemyComponent>(enemyEntity);
            enemy.moveSpeed                          = moveSpeed;

            ActionGame::ECS::HealthComponent& enemyHealth = registry.AddComponent<ActionGame::ECS::HealthComponent>(enemyEntity);
            enemyHealth.maxHealth                          = maxHealth;
            enemyHealth.currentHealth                      = maxHealth;
        };

        spawnEnemy(hlslpp::float3(200.0f, 20.0f, 200.0f), 100.0f, 40.0f);     // 弱い近接タイプ
        spawnEnemy(hlslpp::float3(-200.0f, 20.0f, 200.0f), 80.0f, 80.0f);     // やや硬い近接タイプ
        spawnEnemy(hlslpp::float3(0.0f, 20.0f, -250.0f), 90.0f, 60.0f);       // 3体目

        //--------------------------------------------------------------
        // 2Dカメラエンティティの生成
        //--------------------------------------------------------------
        Tsukino::ECS::Entity cameraEntity2D = m_scene.CreateEntity();

        // TransformComponent (カメラの位置)
        Tsukino::BuiltIn::ECS::TransformComponent& camTransform2D = registry.AddComponent<Tsukino::BuiltIn::ECS::TransformComponent>(cameraEntity2D);
        camTransform2D.position                                   = hlslpp::float3(0.0f, 0.0f, -1.0f);    // 手前に引く

        // CameraComponent (投影設定)
        Tsukino::BuiltIn::ECS::CameraComponent& camera2D = registry.AddComponent<Tsukino::BuiltIn::ECS::CameraComponent>(cameraEntity2D);
        camera2D.projectionType                          = Tsukino::BuiltIn::ECS::CameraComponent::ProjectionType::Orthographic;
        camera2D.orthoSize                               = 1000.0f;    // 画面の縦幅を 720 ユニットにする
        camera2D.isPrimary                               = false;      // これをメインカメラにしない

        //--------------------------------------------------------------
        // TPS（三人称視点）カメラエンティティの生成
        // プレイヤーの背後に追従するメインカメラ（isPrimary = true）
        //--------------------------------------------------------------
        {
            Tsukino::ECS::Entity tpsCameraEntity = m_scene.CreateEntity();

            Tsukino::BuiltIn::ECS::TransformComponent& tpsCamTransform = registry.AddComponent<Tsukino::BuiltIn::ECS::TransformComponent>(tpsCameraEntity);
            tpsCamTransform.position                                    = playerTransform.position + hlslpp::float3(0.0f, 200.0f, -400.0f);
            tpsCamTransform.dirty                                       = true;

            Tsukino::BuiltIn::ECS::CameraComponent& tpsCam = registry.AddComponent<Tsukino::BuiltIn::ECS::CameraComponent>(tpsCameraEntity);
            tpsCam.projectionType                          = Tsukino::BuiltIn::ECS::CameraComponent::ProjectionType::Perspective;
            tpsCam.fov                                     = 60.0f;
            tpsCam.nearZ                                   = 0.3f;
            tpsCam.farZ                                    = 2000.0f;
            tpsCam.useLookAt                               = true;
            tpsCam.lookAtTarget                            = playerTransform.position;
            tpsCam.isPrimary                               = true;

            ActionGame::ECS::TpsCameraComponent& tpsCameraComponent = registry.AddComponent<ActionGame::ECS::TpsCameraComponent>(tpsCameraEntity);
            tpsCameraComponent.target                                 = playerEntity;
        }

        //--------------------------------------------------------------
        // デバッグカメラエンティティの生成 (デバッグビルドのみ)
        //--------------------------------------------------------------
#ifdef _DEBUG
        {
            Tsukino::ECS::Entity debugCamEntity = m_scene.CreateEntity();

            // 1ユニット≒1cm規約。身長約210のキャラクターを斜め上から見下ろす位置に置く
            Tsukino::BuiltIn::ECS::TransformComponent& t = registry.AddComponent<Tsukino::BuiltIn::ECS::TransformComponent>(debugCamEntity);
            t.position                                   = hlslpp::float3(0.0f, 180.0f, 300.0f);
            t.rotation                                   = hlslpp::quaternion(0.0f, 0.0f, 0.0f, 1.0f);
            t.dirty                                      = true;

            Tsukino::BuiltIn::ECS::CameraComponent& cam = registry.AddComponent<Tsukino::BuiltIn::ECS::CameraComponent>(debugCamEntity);
            cam.lookAtTarget                            = hlslpp::float3(0.0f, 100.0f, 0.0f);
            cam.nearZ                                   = 1.0f;
            cam.farZ                                    = 10000.0f;
            cam.isPrimary                               = false;

            Tsukino::BuiltIn::ECS::DebugCameraComponent& debug = registry.AddComponent<Tsukino::BuiltIn::ECS::DebugCameraComponent>(debugCamEntity);
            debug.moveSpeed                                    = 1.0f;

            registry.AddComponent<Tsukino::BuiltIn::ECS::DebugCameraTag>(debugCamEntity);
        }
#endif

        //--------------------------------------------------------------
        // ディレクショナルライトエンティティの生成
        //--------------------------------------------------------------
        {
            Tsukino::ECS::Entity                              lightEntity = m_scene.CreateEntity();
            Tsukino::BuiltIn::ECS::DirectionalLightComponent& light = registry.AddComponent<Tsukino::BuiltIn::ECS::DirectionalLightComponent>(lightEntity);
            light.direction                                         = hlslpp::float3(0.0f, -0.5f, -1.0f);
            light.color                                             = hlslpp::float3(1.0f, 1.0f, 1.0f);
            light.intensity                                         = 5.0f;
            light.castShadow                                        = true;
        }

        //--------------------------------------------------------------
        // スカイアトモスフィアエンティティの生成
        //--------------------------------------------------------------
        {
            Tsukino::ECS::Entity skyEntity = m_scene.CreateEntity();
            registry.AddComponent<Tsukino::BuiltIn::ECS::SkyAtmosphereComponent>(skyEntity);
        }
    }

    //-------------------------------------------------------------
    //! @brief  シーンの更新
    //-------------------------------------------------------------
    void ActionGameScene::OnUpdate(Tsukino::EngineIntegration::EngineAPI& api, float deltaTime) {
        m_scene.Update(deltaTime);
    }

    //-------------------------------------------------------------
    //! @brief  シーンの終了処理
    //-------------------------------------------------------------
    void ActionGameScene::OnExit() {
        // シーン終了時の解放処理などが必要な場合はここに記述します
    }

}    // namespace Tsukino::Sandbox
