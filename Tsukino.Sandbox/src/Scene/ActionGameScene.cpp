//-------------------------------------------------------------
//! @file    ActionGameScene.cpp
//! @brief   アクションゲームシーンの実装
//! @author  山﨑愛
//-------------------------------------------------------------
#include <Tsukino/Sandbox/Scene/ActionGameScene.hpp>

#include <Tsukino/Sandbox/ActionGame/ECS/Component/PlayerComponent.hpp>
#include <Tsukino/Sandbox/ActionGame/ECS/Component/HealthComponent.hpp>
#include <Tsukino/Sandbox/ActionGame/ECS/Component/WeaponComponent.hpp>
#include <Tsukino/Sandbox/ActionGame/ECS/Component/EnemyComponent.hpp>
#include <Tsukino/Sandbox/ActionGame/ECS/Component/TpsCameraComponent.hpp>
#include <Tsukino/Sandbox/ActionGame/ECS/Component/PlayerAnimationSetComponent.hpp>
#include <Tsukino/Sandbox/ActionGame/ECS/Component/PickupComponent.hpp>
#include <Tsukino/Sandbox/ActionGame/ECS/Component/PickupPromptComponent.hpp>
#include <Tsukino/Sandbox/ActionGame/ECS/Component/BehaviorTreeComponent.hpp>
#include <Tsukino/Sandbox/ActionGame/ECS/Component/EnemyAnimationSetComponent.hpp>
#include <Tsukino/Sandbox/ActionGame/ECS/AI/BigZombieBehavior.hpp>
#include <Tsukino/Sandbox/ActionGame/ECS/System/PlayerSystem.hpp>
#include <Tsukino/Sandbox/ActionGame/ECS/System/CombatSystem.hpp>
#include <Tsukino/Sandbox/ActionGame/ECS/System/AttackMotionBlurSystem.hpp>
#include <Tsukino/Sandbox/ActionGame/ECS/System/EnemySystem.hpp>
#include <Tsukino/Sandbox/ActionGame/ECS/System/EnemyBehaviorSystem.hpp>
#include <Tsukino/Sandbox/ActionGame/ECS/System/EnemyAnimationSystem.hpp>
#include <Tsukino/Sandbox/ActionGame/ECS/System/TpsCameraSystem.hpp>
#include <Tsukino/Sandbox/ActionGame/ECS/System/PlayerAnimationSystem.hpp>
#include <Tsukino/Sandbox/ActionGame/ECS/System/PickupSystem.hpp>
#include <Tsukino/Sandbox/ActionGame/ECS/System/HealthBarSystem.hpp>
#ifdef _DEBUG
#include <Tsukino/Sandbox/DebugTools/ECS/System/LightStressTestSystem.hpp>
#include <Tsukino/Sandbox/DebugTools/ECS/Component/LightStressTestComponent.hpp>
#include <Tsukino/Sandbox/ActionGame/ECS/System/WeaponGripDebugSystem.hpp>
#include <Tsukino/Sandbox/ActionGame/ECS/Component/WeaponGripDebugComponent.hpp>
#endif

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
#include <Tsukino/EngineIntegration/ECS/System/LightSystem.hpp>
#include <Tsukino/EngineIntegration/ECS/System/SkyAtmosphereSystem.hpp>
#include <Tsukino/EngineIntegration/ECS/System/MotionBlurSystem.hpp>
#include <Tsukino/EngineIntegration/ECS/System/MotionVectorSnapshotSystem.hpp>
#include <Tsukino/EngineIntegration/ECS/System/DebugCameraSystem.hpp>
#include <Tsukino/EngineIntegration/ECS/System/EffectSystem.hpp>
#include <Tsukino/EngineIntegration/ECS/System/WorldAnchorSystem.hpp>

#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/CameraComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/SpriteComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/WorldAnchorComponent.hpp>
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
#include <Tsukino/BuiltIn/ECS/Component/PointLightComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/SpotLightComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/SkyAtmosphereComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/MotionBlurComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/SpringBoneComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/DebugCameraComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/DebugCameraTag.hpp>
#include <Tsukino/BuiltIn/ECS/Component/EffectComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/HighlightComponent.hpp>

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
        enum class SystemPriority : int {
            MotionVectorSnapshot = -2,    // モーションブラー用に前フレームのworldMatrix/ボーン行列を退避する。
                                          // TransformSystem・AnimationSystemが今フレームの値を書く「前」に読むことで、
                                          // ダブルバッファなしに前フレームの値を取り出している。ここより後ろへ動かすと
                                          // 速度が常にゼロになりブラーが効かなくなる
            LightStressTest = -1,    // デバッグ用の多光源スポーン。生成/移動したライトの
                                     // worldMatrixを同じフレームで確定させるためTransformより前に置く
            Transform = 0,    // 一番最初に計算する
            Movement,         // プレイヤー入力・敵AIの移動をTransformの後、Physicsの前に反映する
            Gameplay,         // ダメージ処理・アニメーション更新は移動確定後に行う
            WeaponGripDebug,  // （デバッグビルドのみ）握り位置調整はisAttackingを上書きするため、
                              // PlayerAnimationSystem（Gameplay）が今フレームのisAttackingを確定させた後、
                              // CombatSystem（WeaponAttach）がそれを読む前に割り込む
            WeaponAttach,     // 武器の追従（ボーンアタッチ）はAnimationSystemが今フレームのボーン姿勢を書き込んだ後に行う
            HealthBar,        // 頭上HPバーの表示可否・残量幅の更新。被弾（WeaponAttachでCombatSystemが
                              // hpBarVisibleTimerをセット）の後、WorldAnchorSystemが座標を確定させる前に行う
            TransformLate,    // Movement/WeaponAttachで更新したposition/rotationをworldMatrixへ反映する2回目のTransformSystem。
                              // これが無いと、このフレームで更新された所有者の回転がworldMatrix（描画に使われる）へ
                              // 反映されるのは次フレームになり、武器はowner.rotationを直接読むため1フレーム分
                              // 先行してしまい、回転中（旋回中）だけ武器の位置が体からずれて見える不具合が起きる
            Camera3D,         // TPS/デバッグカメラの追従は移動確定後、カメラ行列計算の前に行う
            Camera,           // カメラ行列は描画前に計算する
            WorldAnchor,      // WorldAnchorComponentを持つエンティティ（拾得プロンプト・HPバー等）の
                              // ワールド→スクリーン座標変換は今フレームのカメラ行列を使うためCameraの後に置く
            TransformUI,      // WorldAnchorSystemが書いたUI要素のposition（画面ピクセル座標）をworldMatrixへ反映する。
                              // FontRendererSystemはworldMatrix[3]を読むため、これが無いと1フレーム遅れて表示がスウィムする
            Font,
            AttackMotionBlur,    // 攻撃の進行度（CombatSystemが更新するattackBlend）をブラー強度へ反映する。
                                 // WeaponAttachより後、MotionBlurより前
            MotionBlur,          // ブラーパラメータをRendererへ転送し、MotionVectorComponentを自動アタッチする。
                                 // ModelSystem（Render）が描画コマンドを積む前である必要がある
            Render,
            Audio,
            Physics,    // コリジョンの更新は最後に行う
            Light,      // ディレクショナル/点光源/スポットをまとめてRendererへ渡す。
                        // worldMatrixから位置を取るのでTransform系より後である必要がある
            SkyAtmosphere,
        };

        // 登録
        // ライトのスポーン/移動はTransformSystemより前に行う。そうしないと
        // 生成・移動したライトのworldMatrixが1フレーム遅れ、LightSystemが古い位置を読む
        // モーションブラー用の前フレーム退避は、TransformSystem/AnimationSystemが
        // 今フレームの値で上書きする前に読む必要があるので最初に登録する
        m_scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::MotionVectorSnapshotSystem>(), (int)SystemPriority::MotionVectorSnapshot);
#ifdef _DEBUG
        m_scene.AddSystem(std::make_shared<DebugTools::ECS::LightStressTestSystem>(), (int)SystemPriority::LightStressTest);
#endif
        m_scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::TransformSystem>(), (int)SystemPriority::Transform);
        m_scene.AddSystem(std::make_shared<ActionGame::ECS::PlayerSystem>(), (int)SystemPriority::Movement);
        m_scene.AddSystem(std::make_shared<ActionGame::ECS::EnemySystem>(), (int)SystemPriority::Movement);
        // BT駆動の敵（現状BigZombieのみ）。EnemyComponentは共有するがBehaviorTreeComponentの有無で
        // EnemySystem（直進追跡のみ）とは排他に動く（EnemySystem::Update側でスキップする）
        m_scene.AddSystem(std::make_shared<ActionGame::ECS::EnemyBehaviorSystem>(), (int)SystemPriority::Movement);
        m_scene.AddSystem(std::make_shared<ActionGame::ECS::PlayerAnimationSystem>(), (int)SystemPriority::Gameplay);
        // EnemyAnimationSystemが書いたAnimationControllerComponent::nextを同フレームでAnimationSystemが
        // 消費するため、PlayerAnimationSystemと同じくAnimationSystemより前に登録する
        m_scene.AddSystem(std::make_shared<ActionGame::ECS::EnemyAnimationSystem>(), (int)SystemPriority::Gameplay);
        m_scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::AnimationSystem>(), (int)SystemPriority::Gameplay);
        // カメラ行列を必要としなくなった（座標変換はWorldAnchorSystemが行う）ため、他のゲームプレイ系と同じ並びで良い
        m_scene.AddSystem(std::make_shared<ActionGame::ECS::PickupSystem>(), (int)SystemPriority::Gameplay);
#ifdef _DEBUG
        // 武器の握り位置・角度を実機で調整するためのデバッグ操作（F6で有効化）。
        // 詳細はWeaponGripDebugSystem.cppを参照
        m_scene.AddSystem(std::make_shared<ActionGame::ECS::WeaponGripDebugSystem>(), (int)SystemPriority::WeaponGripDebug);
#endif
        m_scene.AddSystem(std::make_shared<ActionGame::ECS::CombatSystem>(), (int)SystemPriority::WeaponAttach);
        m_scene.AddSystem(std::make_shared<ActionGame::ECS::HealthBarSystem>(), (int)SystemPriority::HealthBar);
        m_scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::TransformSystem>(), (int)SystemPriority::TransformLate);
        m_scene.AddSystem(std::make_shared<ActionGame::ECS::TpsCameraSystem>(), (int)SystemPriority::Camera3D);
#ifdef _DEBUG
        m_scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::DebugCameraSystem>(), (int)SystemPriority::Camera3D);
#endif
        m_scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::CameraSystem>(), (int)SystemPriority::Camera);
        m_scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::WorldAnchorSystem>(), (int)SystemPriority::WorldAnchor);
        m_scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::TransformSystem>(), (int)SystemPriority::TransformUI);
        m_scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::FontRendererSystem>(), (int)SystemPriority::Font);
        // 攻撃演出→ブラー強度→Rendererの順に流す
        m_scene.AddSystem(std::make_shared<ActionGame::ECS::AttackMotionBlurSystem>(), (int)SystemPriority::AttackMotionBlur);
        m_scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::MotionBlurSystem>(), (int)SystemPriority::MotionBlur);
        m_scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::SpriteRenderSystem>(), (int)SystemPriority::Render);
        m_scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::ModelSystem>(), (int)SystemPriority::Render);
        {
            auto effectSystem = std::make_shared<Tsukino::BuiltIn::ECS::EffectSystem>();
            m_scene.AddSystem(effectSystem, (int)SystemPriority::Render);
            effectSystem->Initialize(m_scene.GetRegistry(), eventBus);
            context->effectSystem = effectSystem.get();
        }
        m_scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::AudioSystem>(), (int)SystemPriority::Audio);
        {
            auto physicsSystem = std::make_shared<Tsukino::BuiltIn::ECS::PhysicsSystem>(eventBus);
#ifdef TSUKINO_DEBUG_COLLISION_DRAW
            // ActionGameSampleでは常にコリジョンのワイヤーフレームを表示する（F5で従来通りOFFも可能）
            physicsSystem->SetDebugDrawEnabled(true);
#endif
            m_scene.AddSystem(physicsSystem, (int)SystemPriority::Physics);
            context->physicsSystem = physicsSystem.get();
        }
        m_scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::LightSystem>(), (int)SystemPriority::Light);
        m_scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::SkyAtmosphereSystem>(), (int)SystemPriority::SkyAtmosphere);

        //--------------------------------------------------------------
        // アセットのロード
        //--------------------------------------------------------------

        Tsukino::Asset::AssetHandle modelHandle =
            context->assetManager->Load(Tsukino::Core::Path("Tsukino.Sandbox/Assets/ActionGameSample/Models/Player.fbx"));

        Tsukino::Asset::AssetHandle animationHandle =
            context->assetManager->Load(Tsukino::Core::Path("Tsukino.Sandbox/Assets/ActionGameSample/Anims/Jump.fbx"));

        // プレイヤーのアニメーションステートマシン（PlayerAnimationSystem）が使うクリップ
        Tsukino::Asset::AssetHandle idleAnimHandle = context->assetManager->Load(Tsukino::Core::Path("Tsukino.Sandbox/Assets/ActionGameSample/Anims/Idle.fbx"));
        Tsukino::Asset::AssetHandle runAnimHandle  = context->assetManager->Load(Tsukino::Core::Path("Tsukino.Sandbox/Assets/ActionGameSample/Anims/Run.fbx"));
        Tsukino::Asset::AssetHandle fastRunAnimHandle =
            context->assetManager->Load(Tsukino::Core::Path("Tsukino.Sandbox/Assets/ActionGameSample/Anims/Fast Run.fbx"));
        // Weapon Attack.fbx は3回斬るモーションが1クリップに入っており、連撃の各段は
        // 同じハンドルを時間レンジだけ変えて3回参照する（下のattackSteps初期化を参照）
        Tsukino::Asset::AssetHandle attackAnimHandle =
            context->assetManager->Load(Tsukino::Core::Path("Tsukino.Sandbox/Assets/ActionGameSample/Anims/Weapon Attack.fbx"));

        // BigZombie（EnemyAnimationSystem）が使うクリップ。Idle用クリップが無いため、
        // 待機はMutant Walkingをin_place再生（その場足踏み）にして流用する
        Tsukino::Asset::AssetHandle bigZombieWalkAnimHandle =
            context->assetManager->Load(Tsukino::Core::Path("Tsukino.Sandbox/Assets/ActionGameSample/Anims/BigZombie/Mutant Walking.fbx"));
        Tsukino::Asset::AssetHandle bigZombieAttackAnimHandle =
            context->assetManager->Load(Tsukino::Core::Path("Tsukino.Sandbox/Assets/ActionGameSample/Anims/BigZombie/Mutant Swiping.fbx"));

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
            collision.extent                                     = {5000.0f, 5.0f, 5000.0f};
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
        // index 0はMixamo製FBX共通の1tickスタブ、index 1が実モーション（PlayerAnimationSystemと合わせる）
        animPlayer.animation_index                                  = 1;
        animPlayer.elapsed_time                                     = 0.0f;
        animPlayer.playback_speed                                   = 1.0f;
        animPlayer.is_looping                                       = true;    // ループさせる
        animPlayer.is_playing                                       = true;    // 再生状態にする
        // In Placeの固定対象ノード名（Mixamoのリグ命名。WeaponComponent::handBoneNameと同じ流儀）。
        // 空にすると自動判定（スキニング対象ボーンのうち最も浅いもの）にフォールバックする
        animPlayer.root_motion_node_name                            = "mixamorig:Hips";

        // クリップの切り替え（AnimationSystemが読む「次に再生するクリップ」の受け皿）
        registry.AddComponent<Tsukino::BuiltIn::ECS::AnimationControllerComponent>(playerEntity);

        // PlayerAnimationSystemが参照する、ステートごとのアニメーションクリップ一式
        ActionGame::ECS::PlayerAnimationSetComponent& animSet = registry.AddComponent<ActionGame::ECS::PlayerAnimationSetComponent>(playerEntity);
        animSet.idleClip                                      = idleAnimHandle;
        animSet.runClip                                       = runAnimHandle;
        animSet.fastRunClip                                   = fastRunAnimHandle;
        animSet.jumpClip                                      = animationHandle;
        animSet.currentState                                  = ActionGame::ECS::PlayerAnimState::Idle;

        // Weapon Attack.fbx は3回斬るモーションが1クリップ（30fps / 106フレーム = 3.5333秒）に
        // 入っている。各段のstartTime/endTime/playbackSpeedは実機で見ながら個別に微調整する前提の
        // 初期値（_DEBUGビルドのPlayerAnimationSystemが出すATTACKログとWeaponGripDebugSystemの
        // F10/F11コマ送りで追い込む）。ループではなく段ごとに書き下すことで、1段ずつ独立して
        // 長さ・速度を変えられるようにしている
        constexpr float kAttackClipDuration = 3.5333f;
        constexpr float kAttackStepLength   = kAttackClipDuration / 3.0f;    // 約1.178秒 ≒ 35.3フレーム
        constexpr float kAttackPlaybackSpeed = 1.5f;    // 攻撃全体を等速より少し速く（1.0で従来通りの速さ）

        animSet.attackSteps[0].clip           = attackAnimHandle;
        animSet.attackSteps[0].animationIndex = 1;
        animSet.attackSteps[0].startTime      = kAttackStepLength * 0.0f;
        animSet.attackSteps[0].endTime        = kAttackStepLength * 1.0f;
        animSet.attackSteps[0].playbackSpeed  = kAttackPlaybackSpeed;
        // 攻撃モーションのルート前進を殺す（コリジョンから離れる/戻る瞬間に吸い寄せられる問題への対処）
        animSet.attackSteps[0].inPlace        = true;

        animSet.attackSteps[1].clip           = attackAnimHandle;
        animSet.attackSteps[1].animationIndex = 1;
        animSet.attackSteps[1].startTime      = kAttackStepLength * 1.0f;
        animSet.attackSteps[1].endTime        = kAttackStepLength * 1.3f;
        animSet.attackSteps[1].playbackSpeed  = kAttackPlaybackSpeed;
        animSet.attackSteps[1].inPlace        = true;

        animSet.attackSteps[2].clip           = attackAnimHandle;
        animSet.attackSteps[2].animationIndex = 1;
        animSet.attackSteps[2].startTime      = kAttackStepLength * 1.3f;
        animSet.attackSteps[2].endTime        = kAttackClipDuration;
        animSet.attackSteps[2].playbackSpeed  = kAttackPlaybackSpeed;
        animSet.attackSteps[2].inPlace        = true;

        Tsukino::BuiltIn::ECS::SpringBoneComponent& springBone = registry.AddComponent<Tsukino::BuiltIn::ECS::SpringBoneComponent>(playerEntity);

        Tsukino::BuiltIn::ECS::SpringBoneComponent::ChainDef breastL;
        breastL.name                   = "Breast_L";
        breastL.rootNodeName           = "Breast_L";
        breastL.maxDepth               = 1;
        breastL.settings.stiffness     = 0.35f;    // リアル(0.55)より少し柔らかく、揺れ幅を出す
        breastL.settings.drag          = 0.13f;    // 収まりをやや長めに（2〜3往復くらい残る）
        breastL.settings.inertia       = 0.5f;     // 体の動きに対して、わずかに「置いていかれる」感を演出
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
        // 武器エンティティ生成（プレイヤーの周りをふわふわ浮遊させる演出）。
        // 複数の武器を同時に浮遊させられるよう、1本分の生成処理を共通化する。
        // 位置・回転はCombatSystemが毎フレーム所有者（プレイヤー）を基準に計算する
        //--------------------------------------------------------------
        auto spawnFloatingWeapon = [&](const Tsukino::Core::Path& modelPath, const hlslpp::float3& localOffset,
                                       const hlslpp::float3& gripPointLocal, const hlslpp::float3& attackLocalOffset,
                                       const hlslpp::quaternion& attackGripRotationOffset) -> Tsukino::ECS::Entity {
            Tsukino::ECS::Entity weaponEntity = m_scene.CreateEntity();

            Tsukino::BuiltIn::ECS::TransformComponent& weaponTransform = registry.AddComponent<Tsukino::BuiltIn::ECS::TransformComponent>(weaponEntity);
            weaponTransform.position                                   = playerTransform.position;    // 初期値。以後CombatSystemが上書きする
            weaponTransform.rotation                                   = hlslpp::quaternion(0.0f, 0.0f, 0.0f, 1.0f);
            weaponTransform.scale                                      = hlslpp::float3(1.0f, 1.0f, 1.0f);    // 暫定値。実機で見た目を確認しながら調整する
            weaponTransform.dirty                                      = true;
            weaponTransform.parent                                     = entt::null;

            Tsukino::Asset::AssetHandle weaponModelHandle = context->assetManager->Load(modelPath);
            Tsukino::BuiltIn::ECS::ModelComponent& weaponModel = registry.AddComponent<Tsukino::BuiltIn::ECS::ModelComponent>(weaponEntity);
            weaponModel.modelHandle                            = weaponModelHandle;
            weaponModel.visible                                = true;

            ActionGame::ECS::WeaponComponent& weapon = registry.AddComponent<ActionGame::ECS::WeaponComponent>(weaponEntity);
            weapon.owner                             = playerEntity;
            // 非攻撃時：右手ボーンへのアタッチは、Idle.fbx（アニメーションクリップ）側のボーン姿勢データが
            // 実際の見た目のポーズと一致しない（別アセットのため、ボーン名は一致してもリグの前提が食い違っている）
            // ため使わない。handTrackingWeight=0で所有者のルートTransformからの固定オフセットにのみ追従させ、
            // floatEnabledで「手に持つ」のではなく肩の斜め上をふわふわ浮遊する演出にする。
            // 位置・姿勢とも所有者のローカル空間を基準に計算するので、どの向きでも所有者に対する
            // 相対位置・相対姿勢が変わらない（＝キャラクターの旋回に合わせて武器も一緒に回る。
            // 攻撃への入り／抜けのブレンド経路を向きに依存させないために必要）。
            // gripRotationOffsetは武器がなるべく縦向きになるよう実機で見た目を確認しながら調整した値。
            // 攻撃時：PlayerAnimationSystemがWeaponComponent::isAttackingをセットし、CombatSystemはこの間
            // floatEnabledを無視してattackHandTrackingWeight（既定1.0=完全追従）でAttackクリップの
            // 右手ボーン姿勢へ追従させる（Idleと違い、Attackクリップ自体の腕の振りに合わせて動くため）。
            weapon.localOffset              = localOffset;
            weapon.gripRotationOffset       = hlslpp::quaternion::rotation_x(1.5708f);
            weapon.handTrackingWeight       = 0.0f;
            weapon.attackHandTrackingWeight = 1.0f;
            // idleと同じく、武器メッシュのデフォルト向き（エクスポート時の軸）を握り姿勢へ補正する。
            // 未設定のままだと攻撃中だけこの補正が抜け落ち、手の回転がそのままメッシュの想定外の軸に
            // 乗ってしまい暴れて見える原因になっていた。gripPointLocal/attackLocalOffset/
            // attackGripRotationOffsetはWeaponGripDebugSystem（_DEBUGビルドのF6調整モード）で
            // 実機の見た目を確認しながら武器ごとに調整し、確定値をここへ焼き込む
            weapon.gripPointLocal           = gripPointLocal;
            weapon.attackGripRotationOffset = attackGripRotationOffset;
            weapon.attackLocalOffset        = attackLocalOffset;
            weapon.floatEnabled             = true;

            return weaponEntity;
        };

        // warhammer：プレイヤーの右肩斜め上で浮遊させる（最初から装備している唯一の武器）。
        // 握りパラメータはWeaponGripDebugSystemで実機調整済みの値
        Tsukino::ECS::Entity warhammerEntity = spawnFloatingWeapon(
            Tsukino::Core::Path("Tsukino.Sandbox/Assets/ActionGameSample/Models/warhammer.fbx"),
            hlslpp::float3(35.0f, 170.0f, -20.0f),
            hlslpp::float3(0.0f, 0.0f, 10.0f),
            hlslpp::float3(0.0f, 0.0f, 0.0f),
            hlslpp::quaternion(0.5f, 0.5f, -0.5f, 0.5f));

        // 切り替え対象の武器一覧（PlayerSystemがマウスホイール入力でここを順送りする）。
        // 初期状態はwarhammerのみ。他の武器はワールドに落ちており、Fキーで拾うとここに増える
        player.weaponInventory     = {warhammerEntity};
        player.selectedWeaponIndex = 0;
        player.weaponEntity         = warhammerEntity;
        registry.GetComponent<ActionGame::ECS::WeaponComponent>(warhammerEntity).floatSelected = true;

        //--------------------------------------------------------------
        // 地面に落ちている武器の生成（Fキーで拾える）。
        // ownerを設定しないため、CombatSystemの追従処理（owner != entt::nullが条件）には入らず
        // その場に留まる。PickupSystemが範囲内・最近傍の1本だけをハイライトし、Fキーで
        // WeaponComponent::ownerをプレイヤーへ設定して浮遊武器へ昇格させる
        //--------------------------------------------------------------
        auto spawnWorldWeapon = [&](const Tsukino::Core::Path& modelPath,
                                    const hlslpp::float3&      worldPosition,
                                    const std::wstring&        displayName,
                                    const hlslpp::float3&      gripPointLocal,
                                    const hlslpp::float3&      attackLocalOffset,
                                    const hlslpp::quaternion&  attackGripRotationOffset) -> Tsukino::ECS::Entity {
            Tsukino::ECS::Entity weaponEntity = m_scene.CreateEntity();

            Tsukino::BuiltIn::ECS::TransformComponent& transform = registry.AddComponent<Tsukino::BuiltIn::ECS::TransformComponent>(weaponEntity);
            transform.position                                   = worldPosition;
            transform.rotation                                   = hlslpp::quaternion::rotation_x(1.5708f);    // 地面に横たわらせる
            transform.scale                                      = hlslpp::float3(1.0f, 1.0f, 1.0f);
            transform.dirty                                      = true;
            transform.parent                                     = entt::null;

            Tsukino::BuiltIn::ECS::ModelComponent& model = registry.AddComponent<Tsukino::BuiltIn::ECS::ModelComponent>(weaponEntity);
            model.modelHandle                            = context->assetManager->Load(modelPath);
            model.visible                                = true;

            ActionGame::ECS::WeaponComponent& weapon = registry.AddComponent<ActionGame::ECS::WeaponComponent>(weaponEntity);
            weapon.owner                              = entt::null;    // 未所有＝ワールドに落ちている状態
            weapon.localOffset                        = hlslpp::float3(35.0f, 170.0f, -20.0f);
            weapon.gripRotationOffset                 = hlslpp::quaternion::rotation_x(1.5708f);
            // 握りパラメータは呼び出し側で武器ごとに指定する（WeaponGripDebugSystemで実機調整した値）
            weapon.gripPointLocal                     = gripPointLocal;
            weapon.attackGripRotationOffset           = attackGripRotationOffset;
            weapon.attackLocalOffset                  = attackLocalOffset;
            weapon.handTrackingWeight                 = 0.0f;
            weapon.floatEnabled                       = false;    // 拾った時にPickupSystemがtrueにする

            ActionGame::ECS::PickupComponent& pickup = registry.AddComponent<ActionGame::ECS::PickupComponent>(weaponEntity);
            pickup.displayName                        = displayName;

            registry.AddComponent<Tsukino::BuiltIn::ECS::HighlightComponent>(weaponEntity);

            return weaponEntity;
        };

        // 動作確認用に近い位置へ2本まとめて置き、「同時に範囲内でも1つだけ光る」ことを確認できるようにする。
        // greatswordはwarhammerと同じ調整済み値を暫定適用（メッシュが違うため厳密には別値が必要になりうる。
        // ずれる場合はWeaponGripDebugSystemのF6調整モードで別途詰める）
        spawnWorldWeapon(Tsukino::Core::Path("Tsukino.Sandbox/Assets/ActionGameSample/Models/greatsword.fbx"),
                         hlslpp::float3(250.0f, 10.0f, 0.0f), L"グレートソード",
                         hlslpp::float3(0.0f, 0.0f, 10.0f),
                         hlslpp::float3(0.0f, 0.0f, 0.0f),
                         hlslpp::quaternion(0.5f, 0.5f, -0.5f, 0.5f));
        // warhammerは最初から装備している個体（spawnFloatingWeapon）と同じモデルのため、同じ調整済み値を使う
        spawnWorldWeapon(Tsukino::Core::Path("Tsukino.Sandbox/Assets/ActionGameSample/Models/warhammer.fbx"),
                         hlslpp::float3(340.0f, 10.0f, 0.0f), L"ウォーハンマー",
                         hlslpp::float3(0.0f, 0.0f, 10.0f),
                         hlslpp::float3(0.0f, 0.0f, 0.0f),
                         hlslpp::quaternion(0.5f, 0.5f, -0.5f, 0.5f));

        //--------------------------------------------------------------
        // 敵エンティティ生成（Phase A: 本番の敵アセットが無いため、既存のBlock.fbxを仮の敵体として流用）。
        // 当たり判定はPhase Bとして物理形状（Joltのカプセルセンサー）ベースに差し替え済み。
        // bodyRadius/bodyHalfHeightは呼び出し側でモデルの見た目に合わせて指定する
        // （プレイヤーとの接触ダメージ判定は引き続きEnemyComponent::bodyRadiusを使った距離判定のまま）
        //--------------------------------------------------------------
        // 頭上HPバー（背景・残量）の見た目に使う単色テクスチャ。
        // 全ての敵で使い回すため、ここで1回だけロードする
        Tsukino::Asset::AssetHandle hpBarTextureHandle =
            context->assetManager->Load(Tsukino::Core::Path("Tsukino.Sandbox/Assets/ActionGameSample/Textures/UI/WhitePixel.png"));

        auto spawnEnemy = [&](hlslpp::float3 spawnPosition, float moveSpeed, float maxHealth,
                               const Tsukino::Core::Path& modelPath, hlslpp::float3 scale,
                               float bodyRadius, float bodyHalfHeight) -> Tsukino::ECS::Entity {
            Tsukino::ECS::Entity enemyEntity = m_scene.CreateEntity();

            Tsukino::BuiltIn::ECS::TransformComponent& enemyTransform = registry.AddComponent<Tsukino::BuiltIn::ECS::TransformComponent>(enemyEntity);
            enemyTransform.position                                   = spawnPosition;
            enemyTransform.rotation                                   = hlslpp::quaternion(0.0f, 0.0f, 0.0f, 1.0f);
            enemyTransform.scale                                      = scale;
            enemyTransform.dirty                                      = true;
            enemyTransform.parent                                     = entt::null;

            Tsukino::Asset::AssetHandle enemyModelHandle = context->assetManager->Load(modelPath);
            Tsukino::BuiltIn::ECS::ModelComponent& enemyModel = registry.AddComponent<Tsukino::BuiltIn::ECS::ModelComponent>(enemyEntity);
            enemyModel.modelHandle                            = enemyModelHandle;
            enemyModel.visible                                = true;

            ActionGame::ECS::EnemyComponent& enemy = registry.AddComponent<ActionGame::ECS::EnemyComponent>(enemyEntity);
            enemy.moveSpeed                        = moveSpeed;
            enemy.bodyRadius                       = bodyRadius;

            ActionGame::ECS::HealthComponent& enemyHealth = registry.AddComponent<ActionGame::ECS::HealthComponent>(enemyEntity);
            enemyHealth.maxHealth                         = maxHealth;
            enemyHealth.currentHealth                     = maxHealth;

            // 武器のヒット判定（CombatSystemのOverlapCapsule）に拾わせるためのカプセルセンサー。
            // Kinematicにすることで、EnemySystemが毎フレーム書き換えるTransformへPhysicsSystemが
            // 追従してくれる（Static/RigidbodyComponent無しだと初期位置に固定されたままになる）。
            // isSensor=trueなので物理的な押し出し（ブロッキング）は発生しない
            Tsukino::BuiltIn::ECS::RigidbodyComponent& enemyRigidbody = registry.AddComponent<Tsukino::BuiltIn::ECS::RigidbodyComponent>(enemyEntity);
            enemyRigidbody.type                                       = Tsukino::BuiltIn::ECS::RigidbodyType::Kinematic;

            Tsukino::BuiltIn::ECS::CollisionComponent& enemyCollision = registry.AddComponent<Tsukino::BuiltIn::ECS::CollisionComponent>(enemyEntity);
            enemyCollision.type                                       = Tsukino::BuiltIn::ECS::ColliderType::Capsule;
            enemyCollision.extent                                     = hlslpp::float3(bodyRadius, bodyHalfHeight, 0.0f);
            enemyCollision.isSensor                                   = true;
            // Transform位置＝足元とみなし、カプセル中心をそこから上へオフセットする
            // （CharacterControllerComponent::centerOffsetと同じ考え方）
            enemyCollision.offsetPosition = hlslpp::float3(0.0f, bodyHalfHeight + bodyRadius, 0.0f);

            //-------------------------------------------------------------
            // 頭上HPバー（背景＋残量の2エンティティ）。カプセル上端（2*(bodyHalfHeight+bodyRadius)）より
            // 少し上に浮かせる。WorldAnchorSystemが毎フレームスクリーン座標へ投影し、
            // HealthBarSystemが残量に応じて見た目を更新する（被弾時のみ表示）
            //-------------------------------------------------------------
            hlslpp::float3 hpBarWorldOffset = hlslpp::float3(0.0f, (bodyHalfHeight + bodyRadius) * 2.0f + 20.0f, 0.0f);

            Tsukino::ECS::Entity hpBarBackgroundEntity = m_scene.CreateEntity();
            {
                Tsukino::BuiltIn::ECS::TransformComponent& t = registry.AddComponent<Tsukino::BuiltIn::ECS::TransformComponent>(hpBarBackgroundEntity);
                t.scale                                       = hlslpp::float3(0.0f, 0.0f, 0.0f);    // 非表示状態で開始（被弾時にHealthBarSystemが表示する）

                Tsukino::BuiltIn::ECS::WorldAnchorComponent& anchor =
                    registry.AddComponent<Tsukino::BuiltIn::ECS::WorldAnchorComponent>(hpBarBackgroundEntity);
                anchor.target      = enemyEntity;
                anchor.worldOffset = hpBarWorldOffset;

                Tsukino::BuiltIn::ECS::SpriteComponent& sprite = registry.AddComponent<Tsukino::BuiltIn::ECS::SpriteComponent>(hpBarBackgroundEntity);
                sprite.textureHandle                            = hpBarTextureHandle;
                sprite.tintColor                                = hlslpp::float4(0.15f, 0.15f, 0.15f, 0.9f);    // 暗いグレー
                sprite.sortOrder                                = 0;    // 残量バーより先に描く
            }

            Tsukino::ECS::Entity hpBarFillEntity = m_scene.CreateEntity();
            {
                Tsukino::BuiltIn::ECS::TransformComponent& t = registry.AddComponent<Tsukino::BuiltIn::ECS::TransformComponent>(hpBarFillEntity);
                t.scale                                       = hlslpp::float3(0.0f, 0.0f, 0.0f);

                Tsukino::BuiltIn::ECS::WorldAnchorComponent& anchor =
                    registry.AddComponent<Tsukino::BuiltIn::ECS::WorldAnchorComponent>(hpBarFillEntity);
                anchor.target      = enemyEntity;
                anchor.worldOffset = hpBarWorldOffset;

                Tsukino::BuiltIn::ECS::SpriteComponent& sprite = registry.AddComponent<Tsukino::BuiltIn::ECS::SpriteComponent>(hpBarFillEntity);
                sprite.textureHandle                            = hpBarTextureHandle;
                sprite.tintColor                                = hlslpp::float4(0.0f, 1.0f, 0.0f, 1.0f);    // 満タン時は緑
                sprite.sortOrder                                = 1;    // 背景の上に描く
            }

            enemyHealth.hpBarBackgroundEntity = hpBarBackgroundEntity;
            enemyHealth.hpBarFillEntity       = hpBarFillEntity;

            return enemyEntity;
        };

        const Tsukino::Core::Path blockModelPath("Tsukino.Sandbox/Assets/ActionGameSample/Models/Block.fbx");
        spawnEnemy(hlslpp::float3(200.0f, 20.0f, 200.0f), 100.0f, 40.0f, blockModelPath, hlslpp::float3(1.5f, 1.5f, 1.5f), 40.0f, 70.0f);    // 弱い近接タイプ
        spawnEnemy(hlslpp::float3(-200.0f, 20.0f, 200.0f), 80.0f, 80.0f, blockModelPath, hlslpp::float3(1.5f, 1.5f, 1.5f), 40.0f, 70.0f);    // やや硬い近接タイプ
        spawnEnemy(hlslpp::float3(0.0f, 20.0f, -250.0f), 90.0f, 60.0f, blockModelPath, hlslpp::float3(1.5f, 1.5f, 1.5f), 40.0f, 70.0f);      // 3体目

        // BigZombie（Phase A: モデルは仮配置。カプセルサイズは見た目のスケール(2.2倍)に合わせて拡大している）
        Tsukino::ECS::Entity bigZombieEntity =
            spawnEnemy(hlslpp::float3(-250.0f, 20.0f, -250.0f), 70.0f, 150.0f,
                       Tsukino::Core::Path("Tsukino.Sandbox/Assets/ActionGameSample/Models/BigZombie.fbx"),
                       hlslpp::float3(2.2f, 2.2f, 2.2f), 70.0f, 110.0f);

        //--------------------------------------------------------------
        // BigZombieはビヘイビアツリー駆動（歩いて近づき、射程内で攻撃する）にする。
        // 他の敵（Block.fbxの3体）はEnemyComponentのみでBehaviorTreeComponentを持たないため
        // 従来通りEnemySystemの直進追跡のまま
        //--------------------------------------------------------------
        {
            // アニメーション再生・制御用コンポーネント（初期状態はIdle。以後はEnemyAnimationSystemが管理する）
            Tsukino::BuiltIn::ECS::AnimationPlayerComponent& bigZombieAnimPlayer =
                registry.AddComponent<Tsukino::BuiltIn::ECS::AnimationPlayerComponent>(bigZombieEntity);
            bigZombieAnimPlayer.current_clip_id       = bigZombieWalkAnimHandle;
            bigZombieAnimPlayer.animation_index       = 1;    // Mixamo製FBXはindex 0が1tickのスタブ、index 1が実モーション
            bigZombieAnimPlayer.elapsed_time          = 0.0f;
            bigZombieAnimPlayer.playback_speed        = 1.0f;
            bigZombieAnimPlayer.is_looping            = true;
            bigZombieAnimPlayer.is_playing            = true;
            bigZombieAnimPlayer.in_place              = true;    // その場足踏み（移動はEnemyBehaviorSystemがTransformを直接書く）
            bigZombieAnimPlayer.root_motion_node_name = "mixamorig:Hips";

            // クリップの切り替え（AnimationSystemが読む「次に再生するクリップ」の受け皿）
            registry.AddComponent<Tsukino::BuiltIn::ECS::AnimationControllerComponent>(bigZombieEntity);

            // 計算されたボーン行列の出力先（スキニング用）コンポーネント。
            // これが無いとAnimationSystemのView<AnimationPlayerComponent, SkeletonOutputComponent>に
            // 乗らずアニメーションが再生されない
            registry.AddComponent<Tsukino::BuiltIn::ECS::SkeletonOutputComponent>(bigZombieEntity);

            // EnemyAnimationSystemが参照する、ステートごとのアニメーションクリップ一式
            ActionGame::ECS::EnemyAnimationSetComponent& bigZombieAnimSet =
                registry.AddComponent<ActionGame::ECS::EnemyAnimationSetComponent>(bigZombieEntity);
            bigZombieAnimSet.walkClip   = bigZombieWalkAnimHandle;
            bigZombieAnimSet.attackClip = bigZombieAttackAnimHandle;

            // ビヘイビアツリー本体（歩く→射程内で攻撃、を行う）
            ActionGame::ECS::BehaviorTreeComponent& bigZombieBehaviorTree =
                registry.AddComponent<ActionGame::ECS::BehaviorTreeComponent>(bigZombieEntity);
            bigZombieBehaviorTree.root = ActionGame::ECS::BuildBigZombieTree();

            // 攻撃射程の調整。bodyRadius(70)+playerRadiusより広く取り、振りかぶる前に
            // 接触ダメージ（CombatSystem）が先に成立しないようにする
            registry.GetComponent<ActionGame::ECS::EnemyComponent>(bigZombieEntity).attackRange = 150.0f;
        }

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
        // 「Fキーで拾う」UIラベル用エンティティの生成。
        // 毎フレーム生成せず1つを使い回し、PickupSystemがWorldAnchorComponent.target/textを書き換え、
        // 実際の座標変換（position書き込み）はWorldAnchorSystemが行う
        //--------------------------------------------------------------
        Tsukino::ECS::Entity pickupPromptEntity = m_scene.CreateEntity();

        Tsukino::BuiltIn::ECS::TransformComponent& promptTransform = registry.AddComponent<Tsukino::BuiltIn::ECS::TransformComponent>(pickupPromptEntity);
        promptTransform.position                                    = hlslpp::float3(0.0f, 0.0f, 0.0f);    // 以後WorldAnchorSystemが毎フレーム上書きする
        promptTransform.scale                                       = hlslpp::float3(1.0f, 1.0f, 1.0f);
        promptTransform.dirty                                       = true;

        Tsukino::BuiltIn::ECS::WorldAnchorComponent& promptAnchor = registry.AddComponent<Tsukino::BuiltIn::ECS::WorldAnchorComponent>(pickupPromptEntity);
        promptAnchor.target                                        = entt::null;    // 以後PickupSystemが毎フレーム更新する

        Tsukino::BuiltIn::ECS::FontComponent& promptFont = registry.AddComponent<Tsukino::BuiltIn::ECS::FontComponent>(pickupPromptEntity);
        promptFont.text                                   = L"";    // 空文字の間はFontRendererSystemが描画しない
        promptFont.color                                  = hlslpp::float4(1.0f, 1.0f, 1.0f, 1.0f);
        promptFont.origin                                 = hlslpp::float2(0.0f, 0.0f);
        // fontHandle未設定 → builtinAssets->fonts.defaultFont（Default.dfont、動的フォントアトラス経路）が使われるため
        // 日本語をそのまま渡してよい（旧Arial.spritefontはASCII専用でDirectXTKが例外を投げる）

        registry.AddComponent<ActionGame::ECS::PickupPromptComponent>(pickupPromptEntity);

#ifdef _DEBUG
        //--------------------------------------------------------------
        // 武器の握り位置・角度を調整するデバッグHUD用エンティティ（F6で調整モードON時のみ表示）。
        // 上の「Fキーで拾う」ラベルと同じ作り。WeaponGripDebugSystemがtextを毎フレーム書き換える
        //--------------------------------------------------------------
        Tsukino::ECS::Entity weaponGripDebugHudEntity = m_scene.CreateEntity();

        Tsukino::BuiltIn::ECS::TransformComponent& gripHudTransform =
            registry.AddComponent<Tsukino::BuiltIn::ECS::TransformComponent>(weaponGripDebugHudEntity);
        gripHudTransform.position = hlslpp::float3(10.0f, 10.0f, 0.0f);    // 画面左上（生スクリーンピクセル座標）
        gripHudTransform.scale    = hlslpp::float3(1.0f, 1.0f, 1.0f);
        gripHudTransform.dirty    = true;

        Tsukino::BuiltIn::ECS::FontComponent& gripHudFont =
            registry.AddComponent<Tsukino::BuiltIn::ECS::FontComponent>(weaponGripDebugHudEntity);
        gripHudFont.text   = L"";    // 空文字の間はFontRendererSystemが描画しない
        gripHudFont.color  = hlslpp::float4(1.0f, 1.0f, 0.3f, 1.0f);
        gripHudFont.origin = hlslpp::float2(0.0f, 0.0f);

        registry.AddComponent<ActionGame::ECS::WeaponGripDebugComponent>(weaponGripDebugHudEntity);
#endif

        //--------------------------------------------------------------
        // TPS（三人称視点）カメラエンティティの生成
        // プレイヤーの背後に追従するメインカメラ（isPrimary = true）
        //--------------------------------------------------------------
        {
            Tsukino::ECS::Entity tpsCameraEntity = m_scene.CreateEntity();

            Tsukino::BuiltIn::ECS::TransformComponent& tpsCamTransform = registry.AddComponent<Tsukino::BuiltIn::ECS::TransformComponent>(tpsCameraEntity);
            tpsCamTransform.position                                   = playerTransform.position + hlslpp::float3(0.0f, 200.0f, -400.0f);
            tpsCamTransform.dirty                                      = true;

            Tsukino::BuiltIn::ECS::CameraComponent& tpsCam = registry.AddComponent<Tsukino::BuiltIn::ECS::CameraComponent>(tpsCameraEntity);
            tpsCam.projectionType                          = Tsukino::BuiltIn::ECS::CameraComponent::ProjectionType::Perspective;
            tpsCam.fov                                     = 60.0f;
            tpsCam.nearZ                                   = 0.3f;
            tpsCam.farZ                                    = 2000.0f;
            tpsCam.useLookAt                               = true;
            tpsCam.lookAtTarget                            = playerTransform.position;
            tpsCam.isPrimary                               = true;

            ActionGame::ECS::TpsCameraComponent& tpsCameraComponent = registry.AddComponent<ActionGame::ECS::TpsCameraComponent>(tpsCameraEntity);
            tpsCameraComponent.target                               = playerEntity;

            //----------------------------------------------------------
            // モーションブラー（オブジェクト速度バッファ方式）
            // このコンポーネントを外せばモーションブラーごと無効になる。
            // strengthはAttackMotionBlurSystemが攻撃の進行度に応じて毎フレーム上書きする。
            //----------------------------------------------------------
            Tsukino::BuiltIn::ECS::MotionBlurComponent& motionBlur = registry.AddComponent<Tsukino::BuiltIn::ECS::MotionBlurComponent>(tpsCameraEntity);
            motionBlur.maxBlurRadius                               = 0.03f;
            motionBlur.sampleCount                                 = 8;
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
        // 点光源・スポットライトの生成（ディファードの多光源デモ）
        //
        // ディレクショナルライトと違い TransformComponent が必須。
        // LightSystem の View が <TransformComponent, PointLight/SpotLight> のため、
        // Transform を付け忘れると収集されず、何も光らない。
        // また worldMatrix から位置を取るので dirty=true を立てて
        // TransformSystem に初回計算をさせること。
        //--------------------------------------------------------------
        {
            struct PointLightSpec {
                hlslpp::float3 position;
                hlslpp::float3 color;
                float          intensity;
                float          range;
            };

            // プレイヤーとゾンビが戦う地面付近を、暖色と寒色で挟むように配置する。
            //
            // intensityの単位に注意：減衰が物理的な逆二乗（intensity / (d^2 + 1)）なので、
            // 到達させたい放射輝度に「距離の二乗」を掛けた値が必要になる。
            // このシーンは1ユニット≒1cm相当（プレイヤー身長が約210ユニット）で
            // ライトまでの距離が150前後になるため d^2 ≒ 22500。
            // つまり radiance を 3 程度にしたければ intensity は 7万前後が必要。
            // ディレクショナルライト（intensity=5）と桁が違うのは距離減衰の有無によるもの。
            const PointLightSpec pointSpecs[] = {
                {hlslpp::float3(160.0f, 60.0f, 90.0f), hlslpp::float3(1.0f, 0.45f, 0.15f), 45000.0f, 700.0f},     // 焚き火（橙）
                {hlslpp::float3(-170.0f, 60.0f, 60.0f), hlslpp::float3(0.25f, 0.5f, 1.0f), 45000.0f, 700.0f},     // 月明かり寄り（青）
                {hlslpp::float3(0.0f, 90.0f, -190.0f), hlslpp::float3(0.6f, 1.0f, 0.4f), 40000.0f, 700.0f},       // 背後（緑）
                {hlslpp::float3(0.0f, 40.0f, 170.0f), hlslpp::float3(1.0f, 0.2f, 0.35f), 35000.0f, 600.0f},       // 手前（赤）
            };

            for(const auto& spec : pointSpecs) {
                Tsukino::ECS::Entity                       e         = m_scene.CreateEntity();
                Tsukino::BuiltIn::ECS::TransformComponent& transform = registry.AddComponent<Tsukino::BuiltIn::ECS::TransformComponent>(e);
                transform.position                                   = spec.position;
                transform.rotation                                   = hlslpp::quaternion(0.0f, 0.0f, 0.0f, 1.0f);
                transform.scale                                      = hlslpp::float3(1.0f, 1.0f, 1.0f);
                transform.dirty                                      = true;          // 初回計算のためフラグを立てる
                transform.parent                                     = entt::null;    // 親なし

                Tsukino::BuiltIn::ECS::PointLightComponent& light = registry.AddComponent<Tsukino::BuiltIn::ECS::PointLightComponent>(e);
                light.color                                      = spec.color;
                light.intensity                                  = spec.intensity;
                light.range                                      = spec.range;
                light.enabled                                    = true;
            }

            //----------------------------------------------------------
            // スポットライト：真上からプレイヤー付近を照らす
            // 向きはエンティティのローカル+Z。X軸に+90度回すと+Zが真下(-Y)を向く
            //----------------------------------------------------------
            {
                Tsukino::ECS::Entity                       e         = m_scene.CreateEntity();
                Tsukino::BuiltIn::ECS::TransformComponent& transform = registry.AddComponent<Tsukino::BuiltIn::ECS::TransformComponent>(e);
                transform.position                                   = hlslpp::float3(0.0f, 300.0f, 0.0f);
                transform.rotation                                   = hlslpp::quaternion::rotation_x(1.5708f);
                transform.scale                                      = hlslpp::float3(1.0f, 1.0f, 1.0f);
                transform.dirty                                      = true;
                transform.parent                                     = entt::null;

                Tsukino::BuiltIn::ECS::SpotLightComponent& light = registry.AddComponent<Tsukino::BuiltIn::ECS::SpotLightComponent>(e);
                light.color                                     = hlslpp::float3(1.0f, 0.95f, 0.85f);
                light.intensity                                 = 120000.0f;    // 距離300から照らすため d^2=90000 を見込む
                light.range                                     = 900.0f;
                light.innerConeDeg                              = 18.0f;
                light.outerConeDeg                              = 32.0f;
                light.enabled                                   = true;
            }
        }

#ifdef _DEBUG
        //--------------------------------------------------------------
        // 多光源ストレステストのHUD（F1でライト数を切り替え、フレーム時間を表示）
        // 上の各種ラベルと同じ作り。LightStressTestSystemがtextを毎フレーム書き換える
        //--------------------------------------------------------------
        {
            Tsukino::ECS::Entity                       hudEntity    = m_scene.CreateEntity();
            Tsukino::BuiltIn::ECS::TransformComponent& hudTransform = registry.AddComponent<Tsukino::BuiltIn::ECS::TransformComponent>(hudEntity);
            hudTransform.position                                   = hlslpp::float3(10.0f, 40.0f, 0.0f);    // 画面左上（生スクリーンピクセル座標）
            hudTransform.scale                                      = hlslpp::float3(1.0f, 1.0f, 1.0f);
            hudTransform.dirty                                      = true;

            Tsukino::BuiltIn::ECS::FontComponent& hudFont = registry.AddComponent<Tsukino::BuiltIn::ECS::FontComponent>(hudEntity);
            hudFont.text                                  = L"";    // 空文字の間はFontRendererSystemが描画しない
            hudFont.color                                 = hlslpp::float4(0.4f, 1.0f, 0.6f, 1.0f);
            hudFont.origin                                = hlslpp::float2(0.0f, 0.0f);

            registry.AddComponent<DebugTools::ECS::LightStressTestHudComponent>(hudEntity);
        }
#endif    // _DEBUG

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
        auto* ctx = m_scene.GetRegistry().GetContext<Tsukino::EngineIntegration::EngineContext*>();

        // ヒットストップ：ctx->hitStopTimerが残っている間、Sceneへ渡すdeltaTimeそのものを
        // 縮小する（アニメーション・物理・カメラ追従まで一律にスローになる、意図的なグローバル停止）。
        // hitStopTimer自体は実時間（縮小前のdeltaTime）で減算しないと、
        // 停止時間そのものが引き伸ばされてしまう
        float scaledDeltaTime = deltaTime;
        if(ctx && ctx->hitStopTimer > 0.0f) {
            scaledDeltaTime = deltaTime * ctx->hitStopScale;
            ctx->hitStopTimer -= deltaTime;
            if(ctx->hitStopTimer < 0.0f)
                ctx->hitStopTimer = 0.0f;
        }

        m_scene.Update(scaledDeltaTime);
    }

    //-------------------------------------------------------------
    //! @brief  シーンの終了処理
    //-------------------------------------------------------------
    void ActionGameScene::OnExit() {
        // シーン終了時の解放処理などが必要な場合はここに記述します
    }

}    // namespace Tsukino::Sandbox
