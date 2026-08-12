//-------------------------------------------------------------
//! @file    LuckGameSampleScene.cpp
//! @brief   チンチロゲームの実装
//! @author  山﨑愛
//-------------------------------------------------------------
#include <Tsukino/Sandbox/Scene/LuckGameSampleScene.hpp>

#include <Tsukino/EngineIntegration/EngineAPI.hpp>
#include <Tsukino/EngineIntegration/EngineContext.hpp>
#include <Tsukino/Engine/Asset/AssetManager.hpp>
#include <Tsukino/Engine/ECS/Prefab/PrefabFactory.hpp>

#include <Tsukino/Core/Path.hpp>
#include <Tsukino/Core/Log.hpp>

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
#include <Tsukino/EngineIntegration/ECS/System/HeightmapGenerationSystem.hpp>

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
#include <Tsukino/BuiltIn/ECS/Component/AnimationControllerComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/DirectionalLightComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/SkyAtmosphereComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/SpringBoneComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/DebugCameraComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/DebugCameraTag.hpp>
#include <Tsukino/BuiltIn/ECS/Component/EffectComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/TerrainGenerationRequestComponent.hpp>

#include <Tsukino/BuiltIn/ECS/Serialization/TransformComponentSerialization.hpp>
#include <Tsukino/BuiltIn/ECS/Serialization/CameraComponentSerialization.hpp>

// チンチロ固有のコンポーネント
#include <Tsukino/Sandbox/LuckGameSampleScene/ECS/Component/DiceComponent.hpp>
#include <Tsukino/Sandbox/LuckGameSampleScene/ECS/Component/RoundOwnerComponent.hpp>
#include <Tsukino/Sandbox/LuckGameSampleScene/ECS/Component/RoundComponent.hpp>
#include <Tsukino/Sandbox/LuckGameSampleScene/ECS/Component/PlayerComponent.hpp>
#include <Tsukino/Sandbox/LuckGameSampleScene/ECS/Component/CPUControllerComponent.hpp>
#include <Tsukino/Sandbox/LuckGameSampleScene/ECS/Component/GameStateComponent.hpp>
#include <Tsukino/Sandbox/LuckGameSampleScene/ECS/Component/UILabelTags.hpp>

// チンチロ固有のシステム
#include <Tsukino/Sandbox/LuckGameSampleScene/ECS/System/DiceRestDetectionSystem.hpp>
#include <Tsukino/Sandbox/LuckGameSampleScene/ECS/System/DiceFaceReadSystem.hpp>
#include <Tsukino/Sandbox/LuckGameSampleScene/ECS/System/HandJudgeSystem.hpp>
#include <Tsukino/Sandbox/LuckGameSampleScene/ECS/System/DiceBoundsSystem.hpp>
#include <Tsukino/Sandbox/LuckGameSampleScene/ECS/System/TurnRuleSystem.hpp>
#include <Tsukino/Sandbox/LuckGameSampleScene/ECS/System/CompareSystem.hpp>
#include <Tsukino/Sandbox/LuckGameSampleScene/ECS/System/RollTriggerSystem.hpp>
#include <Tsukino/Sandbox/LuckGameSampleScene/ECS/System/CPURerollSystem.hpp>
#include <Tsukino/Sandbox/LuckGameSampleScene/ECS/System/LuckGameSampleSceneUISystem.hpp>
#include <Tsukino/Sandbox/LuckGameSampleScene/ECS/System/ResultInputSystem.hpp>
#ifdef _DEBUG
#include <Tsukino/Sandbox/LuckGameSampleScene/ECS/System/DiceDebugOverrideSystem.hpp>
#endif

#include <entt/entt.hpp>
#include <hlsl++.h>
#include <array>

// 名前空間 : (無名) チンチロ固有のエンティティ生成ヘルパー
namespace {

    //-------------------------------------------------------------
    //! @brief  お椀エンティティを生成する
    //! @param  scene           [in] 生成先のシーン
    //! @param  registry        [in] ECSレジストリ
    //! @param  bowlModelHandle [in] お椀モデルのアセットハンドル
    //! @param  centerX         [in] お椀の中心X座標（左右の配置に使用）
    //! @param  terrainSeed     [in] 地形生成の乱数シード（左右で同じ形にならないよう変える）
    //-------------------------------------------------------------
    Tsukino::ECS::Entity CreateBowl(Tsukino::ECS::Scene& scene, Tsukino::ECS::Registry& registry, Tsukino::Asset::AssetHandle bowlModelHandle,
                                     float centerX, uint32_t terrainSeed) {
        Tsukino::ECS::Entity bowlEntity = scene.CreateEntity();

        Tsukino::BuiltIn::ECS::TransformComponent& transform = registry.AddComponent<Tsukino::BuiltIn::ECS::TransformComponent>(bowlEntity);
        transform.position                                   = hlslpp::float3(centerX, 0.0f, 0.0f);
        transform.rotation                                   = hlslpp::quaternion(0.0f, 0.0f, 0.0f, 1.0f);    // 無回転
        transform.scale                                      = hlslpp::float3(1.0f, 1.0f, 1.0f);
        transform.dirty                                      = true;          // 初回計算のためフラグを立てる
        transform.parent                                     = entt::null;    // 親なし

        Tsukino::BuiltIn::ECS::ModelComponent& model = registry.AddComponent<Tsukino::BuiltIn::ECS::ModelComponent>(bowlEntity);
        model.modelHandle                            = bowlModelHandle;
        model.visible                                = true;

        // モデルにコリジョンをつける
        Tsukino::BuiltIn::ECS::CollisionComponent& collision = registry.AddComponent<Tsukino::BuiltIn::ECS::CollisionComponent>(bowlEntity);
        collision.type                                       = Tsukino::BuiltIn::ECS::ColliderType::Heightfield;
        collision.isSensor                                   = false;    // 衝突判定を有効にする

        Tsukino::BuiltIn::ECS::TerrainGenerationRequestComponent& req =
            registry.AddComponent<Tsukino::BuiltIn::ECS::TerrainGenerationRequestComponent>(bowlEntity);
        req.amplitude      = 15.0f;
        req.noiseFrequency = 0.08f;
        req.seed           = terrainSeed;
        req.noiseType      = Tsukino::BuiltIn::ECS::TerrainNoiseType::Noise;

        // RBをつける
        Tsukino::BuiltIn::ECS::RigidbodyComponent& rb = registry.AddComponent<Tsukino::BuiltIn::ECS::RigidbodyComponent>(bowlEntity);
        rb.type                                       = Tsukino::BuiltIn::ECS::RigidbodyType::Static;

        return bowlEntity;
    }

    //-------------------------------------------------------------
    //! @brief  1セット分（3個）のサイコロエンティティを生成する
    //! @param  scene           [in] 生成先のシーン
    //! @param  registry        [in] ECSレジストリ
    //! @param  diceModelHandle [in] サイコロモデルのアセットハンドル
    //! @param  bowlCenter      [in] 所属するお椀の中心座標（RoundOwnerComponent/場外判定に使用）
    //-------------------------------------------------------------
    std::array<Tsukino::ECS::Entity, 3> CreateDiceSet(Tsukino::ECS::Scene& scene, Tsukino::ECS::Registry& registry,
                                                        Tsukino::Asset::AssetHandle diceModelHandle, const hlslpp::float3& bowlCenter) {
        std::array<Tsukino::ECS::Entity, 3> diceEntities{};

        for(int i = 0; i < 3; ++i) {
            Tsukino::ECS::Entity diceEntity = scene.CreateEntity();

            // TransformComponent の追加と初期化
            // 3個が重ならないよう、投下位置をX方向に少しずつずらす
            Tsukino::BuiltIn::ECS::TransformComponent& transform = registry.AddComponent<Tsukino::BuiltIn::ECS::TransformComponent>(diceEntity);
            transform.position                                   = bowlCenter + hlslpp::float3(static_cast<float>(i - 1) * 4.0f, 10.0f, 3.0f);
            transform.rotation                                   = hlslpp::quaternion(0.0f, 0.0f, 0.0f, 1.0f);    // 無回転
            transform.scale                                      = hlslpp::float3(1.0f, 1.0f, 1.0f);
            transform.dirty                                      = true;          // 初回計算のためフラグを立てる
            transform.parent                                     = entt::null;    // 親なし

            // ModelComponent の追加
            Tsukino::BuiltIn::ECS::ModelComponent& model = registry.AddComponent<Tsukino::BuiltIn::ECS::ModelComponent>(diceEntity);
            model.modelHandle                            = diceModelHandle;
            model.visible                                = true;

            // モデルにコリジョンをつける
            Tsukino::BuiltIn::ECS::CollisionComponent& collision = registry.AddComponent<Tsukino::BuiltIn::ECS::CollisionComponent>(diceEntity);
            collision.type                                       = Tsukino::BuiltIn::ECS::ColliderType::Box;
            collision.extent                                     = hlslpp::float3(0.8f, 0.8f, 0.8f);
            collision.isSensor                                   = false;    // 衝突判定を有効にする

            // RBをつける
            Tsukino::BuiltIn::ECS::RigidbodyComponent& rb = registry.AddComponent<Tsukino::BuiltIn::ECS::RigidbodyComponent>(diceEntity);
            rb.type                                       = Tsukino::BuiltIn::ECS::RigidbodyType::Dynamic;
            rb.mass                                       = 1.0f;
            rb.gravityFactor                              = 1.0f;
            rb.restitution                                = 0.3f;
            rb.freezeRotationX                            = false;
            rb.freezeRotationY                            = false;
            rb.freezeRotationZ                            = false;

            // 静止判定・出目確定用
            registry.AddComponent<::LuckGameSampleScene::ECS::DiceComponent>(diceEntity);

            // 場外判定の基準座標（所属するお椀の中心）
            ::LuckGameSampleScene::ECS::RoundOwnerComponent& owner =
                registry.AddComponent<::LuckGameSampleScene::ECS::RoundOwnerComponent>(diceEntity);
            owner.bowlCenter = bowlCenter;

            diceEntities[i] = diceEntity;
        }

        return diceEntities;
    }

    //-------------------------------------------------------------
    //! @brief  UIラベルエンティティ（TransformComponent + FontComponent）を生成する
    //! @param  scene          [in] 生成先のシーン
    //! @param  registry       [in] ECSレジストリ
    //! @param  screenPosition [in] スクリーン座標（左上原点のピクセル座標）
    //-------------------------------------------------------------
    Tsukino::ECS::Entity CreateLabel(Tsukino::ECS::Scene& scene, Tsukino::ECS::Registry& registry, const hlslpp::float3& screenPosition) {
        Tsukino::ECS::Entity labelEntity = scene.CreateEntity();

        Tsukino::BuiltIn::ECS::TransformComponent& transform = registry.AddComponent<Tsukino::BuiltIn::ECS::TransformComponent>(labelEntity);
        transform.position                                   = screenPosition;
        transform.rotation                                   = hlslpp::quaternion(0.0f, 0.0f, 0.0f, 1.0f);    // 無回転
        transform.scale                                      = hlslpp::float3(1.0f, 1.0f, 1.0f);
        transform.dirty                                      = true;          // 初回計算のためフラグを立てる
        transform.parent                                     = entt::null;    // 親なし

        Tsukino::BuiltIn::ECS::FontComponent& font = registry.AddComponent<Tsukino::BuiltIn::ECS::FontComponent>(labelEntity);
        font.text                                  = L"";    // 描画するテキスト（LuckGameSampleSceneUISystemが更新する）

        return labelEntity;
    }

}    // namespace

// 名前空間 : Tsukino::Sandbox
namespace Tsukino::Sandbox {
    //-------------------------------------------------------------
    //! @brief  シーン固有の初期化処理
    //-------------------------------------------------------------
    void LuckGameSampleScene::OnInitialize(Tsukino::EngineIntegration::EngineAPI& api) {
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
        // アニメーションはTransformの後に更新する (優先度 2)
        m_scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::AnimationSystem>(), 2);

        m_scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::HeightmapGenerationSystem>(), 3);

#ifdef _DEBUG
        m_scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::DebugCameraSystem>(), 4);
#endif
        // カメラは描画前に更新する (優先度 5)
        m_scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::CameraSystem>(), 5);

        // 人間の入力検知（開始・個別振り直し） (優先度 6)
        m_scene.AddSystem(std::make_shared<::LuckGameSampleScene::ECS::RollTriggerSystem>(), 6);
        // CPUの「考え中」タイマー消化・自動振り直し (優先度 7)
        m_scene.AddSystem(std::make_shared<::LuckGameSampleScene::ECS::CPURerollSystem>(), 7);
        // UIラベルのテキスト更新。FontRendererSystemより前に置くこと (優先度 8)
        m_scene.AddSystem(std::make_shared<::LuckGameSampleScene::ECS::LuckGameSampleSceneUISystem>(), 8);

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
        m_scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::PhysicsSystem>(eventBus), 12);
        // 場外に出たサイコロをお椀中心へ戻す (優先度 13)
        m_scene.AddSystem(std::make_shared<::LuckGameSampleScene::ECS::DiceBoundsSystem>(), 13);
        // ライトの更新 (優先度 14)
        m_scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::DirectionalLightSystem>(), 14);
        // スカイアトモスフィアの更新 (優先度 15)
        m_scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::SkyAtmosphereSystem>(), 15);

        //--------------------------------------------------------------
        // サイコロの静止判定・出目確定・役判定
        //--------------------------------------------------------------
        m_scene.AddSystem(std::make_shared<::LuckGameSampleScene::ECS::DiceRestDetectionSystem>(), 16);
        m_scene.AddSystem(std::make_shared<::LuckGameSampleScene::ECS::DiceFaceReadSystem>(), 17);
        m_scene.AddSystem(std::make_shared<::LuckGameSampleScene::ECS::HandJudgeSystem>(), 18);

        //--------------------------------------------------------------
        // ラウンド進行（目なし/ヒフミの再挑戦・3回失敗、勝敗比較）
        //--------------------------------------------------------------
        m_scene.AddSystem(std::make_shared<::LuckGameSampleScene::ECS::TurnRuleSystem>(), 19);
        m_scene.AddSystem(std::make_shared<::LuckGameSampleScene::ECS::CompareSystem>(), 20);

        // リザルト中のスペース入力でシーンを再読込する (優先度 21)
        m_scene.AddSystem(std::make_shared<::LuckGameSampleScene::ECS::ResultInputSystem>(), 21);

#ifdef _DEBUG
        // 検証用：数字キーで役を強制発生させる（両お椀に同時に適用される。動作確認用）
        m_scene.AddSystem(std::make_shared<::LuckGameSampleScene::ECS::DiceDebugOverrideSystem>(), 15);
#endif

        //--------------------------------------------------------------
        // アセットのロード
        //--------------------------------------------------------------
        Tsukino::Asset::AssetHandle bowlModelHandle = context->assetManager->Load(Tsukino::Core::Path("Tsukino.Sandbox/Assets/LuckGameSample/Models/Bowl.fbx"));
        Tsukino::Asset::AssetHandle diceModelHandle = context->assetManager->Load(Tsukino::Core::Path("Tsukino.Sandbox/Assets/LuckGameSample/Models/Dice.fbx"));

        Tsukino::ECS::Registry& registry = m_scene.GetRegistry();

        //--------------------------------------------------------------
        // お椀・サイコロを左右2セット生成する
        // CPU側 = 左（X-）、プレイヤー側 = 右（X+）
        //--------------------------------------------------------------
        // モデルアセットは実寸に近いスケールで作られている（お椀の実測AABBは約12x12x6ユニット）ため、
        // お椀の間隔もそれに合わせた現実的な値にする
        constexpr float kBowlOffsetX  = 20.0f;
        constexpr uint32_t kCpuTerrainSeed    = 12345;
        constexpr uint32_t kPlayerTerrainSeed = 54321;    // 左右で同じ地形にならないようシードを変える

        const hlslpp::float3 cpuBowlCenter    = hlslpp::float3(-kBowlOffsetX, 0.0f, 0.0f);
        const hlslpp::float3 playerBowlCenter = hlslpp::float3(kBowlOffsetX, 0.0f, 0.0f);

        CreateBowl(m_scene, registry, bowlModelHandle, cpuBowlCenter.x, kCpuTerrainSeed);
        CreateBowl(m_scene, registry, bowlModelHandle, playerBowlCenter.x, kPlayerTerrainSeed);

        std::array<Tsukino::ECS::Entity, 3> cpuDiceEntities    = CreateDiceSet(m_scene, registry, diceModelHandle, cpuBowlCenter);
        std::array<Tsukino::ECS::Entity, 3> playerDiceEntities = CreateDiceSet(m_scene, registry, diceModelHandle, playerBowlCenter);

        //--------------------------------------------------------------
        // RoundComponent：各セット3個のサイコロを束ねる
        //--------------------------------------------------------------
        Tsukino::ECS::Entity cpuRoundEntity = m_scene.CreateEntity();
        registry.AddComponent<::LuckGameSampleScene::ECS::RoundComponent>(cpuRoundEntity).dice = cpuDiceEntities;

        Tsukino::ECS::Entity playerRoundEntity = m_scene.CreateEntity();
        registry.AddComponent<::LuckGameSampleScene::ECS::RoundComponent>(playerRoundEntity).dice = playerDiceEntities;

        //--------------------------------------------------------------
        // PlayerComponent：人間・CPUそれぞれの進行状態
        //--------------------------------------------------------------
        Tsukino::ECS::Entity cpuEntity = m_scene.CreateEntity();
        {
            ::LuckGameSampleScene::ECS::PlayerComponent& cpuPlayer = registry.AddComponent<::LuckGameSampleScene::ECS::PlayerComponent>(cpuEntity);
            cpuPlayer.roundEntity                                   = cpuRoundEntity;
            registry.AddComponent<::LuckGameSampleScene::ECS::CPUControllerComponent>(cpuEntity);
        }

        Tsukino::ECS::Entity playerEntity = m_scene.CreateEntity();
        {
            ::LuckGameSampleScene::ECS::PlayerComponent& humanPlayer = registry.AddComponent<::LuckGameSampleScene::ECS::PlayerComponent>(playerEntity);
            humanPlayer.roundEntity                                    = playerRoundEntity;
        }

        //--------------------------------------------------------------
        // GameStateComponent：ゲーム全体の進行状態（シングルトン）
        //--------------------------------------------------------------
        ::LuckGameSampleScene::ECS::GameStateComponent& gameState = registry.SetContext<::LuckGameSampleScene::ECS::GameStateComponent>();
        gameState.phase                                             = ::LuckGameSampleScene::ECS::GamePhase::Ready;
        gameState.outcome                                           = ::LuckGameSampleScene::ECS::RoundOutcome::None;
        gameState.player                                            = playerEntity;
        gameState.cpu                                                = cpuEntity;

        //--------------------------------------------------------------
        // UIラベルエンティティの生成（画面は1700x1000を想定した暫定配置）
        //--------------------------------------------------------------
        {
            Tsukino::ECS::Entity cpuLabelEntity = CreateLabel(m_scene, registry, hlslpp::float3(40.0f, 40.0f, 0.0f));
            registry.AddComponent<::LuckGameSampleScene::ECS::CpuHandLabelTag>(cpuLabelEntity);

            Tsukino::ECS::Entity playerLabelEntity = CreateLabel(m_scene, registry, hlslpp::float3(1350.0f, 40.0f, 0.0f));
            registry.AddComponent<::LuckGameSampleScene::ECS::PlayerHandLabelTag>(playerLabelEntity);

            Tsukino::ECS::Entity messageLabelEntity = CreateLabel(m_scene, registry, hlslpp::float3(550.0f, 900.0f, 0.0f));
            registry.AddComponent<::LuckGameSampleScene::ECS::MessageLabelTag>(messageLabelEntity);
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
        // 3Dカメラエンティティの生成（左右両方のお椀が画角に収まる引き位置は
        // Assets/LuckGameSample/Prefabs/3DCamera 側で設定済み）
        //--------------------------------------------------------------
        {
            const std::string prefabPath = "Tsukino.Sandbox/Assets/LuckGameSample/Prefabs/3DCamera/Prefab.json";

            entt::entity testEntity = context->prefabFactory->Instantiate(prefabPath, registry);
        }

        //--------------------------------------------------------------
        // デバッグカメラエンティティの生成 (デバッグビルドのみ)
        //--------------------------------------------------------------
#ifdef _DEBUG
        {
            Tsukino::ECS::Entity debugCamEntity = m_scene.CreateEntity();

            // 実寸スケールのシーンに合わせた引き位置（自由視点なので厳密でなくてよい）
            Tsukino::BuiltIn::ECS::TransformComponent& t = registry.AddComponent<Tsukino::BuiltIn::ECS::TransformComponent>(debugCamEntity);
            t.position                                   = hlslpp::float3(0.0f, 50.0f, -50.0f);
            t.rotation                                   = hlslpp::quaternion(0.0f, 0.0f, 0.0f, 1.0f);
            t.dirty                                      = true;

            Tsukino::BuiltIn::ECS::CameraComponent& cam = registry.AddComponent<Tsukino::BuiltIn::ECS::CameraComponent>(debugCamEntity);
            cam.lookAtTarget                            = hlslpp::float3(0.0f, 0.0f, 0.0f);
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
    void LuckGameSampleScene::OnUpdate(Tsukino::EngineIntegration::EngineAPI& api, float deltaTime) {
        m_scene.Update(deltaTime);
    }

    //-------------------------------------------------------------
    //! @brief  シーンの終了処理
    //-------------------------------------------------------------
    void LuckGameSampleScene::OnExit() {
        // シーン終了時の解放処理などが必要な場合はここに記述します
    }

}    // namespace Tsukino::Sandbox
