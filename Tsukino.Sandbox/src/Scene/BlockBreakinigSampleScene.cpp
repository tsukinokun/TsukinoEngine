//-------------------------------------------------------------
//! @file    BlockBreakingSampleScene.cpp
//! @brief   ブロック崩しサンプルの実装
//! @author  山﨑愛
//-------------------------------------------------------------
#include <Tsukino/Sandbox/Scene/BlockBreakingSampleScene.hpp>

#include <Tsukino/Sandbox/BlockBreakingSample/ECS/Component/PaddleComponent.hpp>
#include <Tsukino/Sandbox/BlockBreakingSample/ECS/Component/BallComponent.hpp>
#include <Tsukino/Sandbox/BlockBreakingSample/ECS/Component/WallComponent.hpp>
#include <Tsukino/Sandbox/BlockBreakingSample/ECS/Component/BrickComponent.hpp>
#include <Tsukino/Sandbox/BlockBreakingSample/ECS/System/PaddleSystem.hpp>
#include <Tsukino/Sandbox/BlockBreakingSample/ECS/System/BallSystem.hpp>
#include <Tsukino/Sandbox/BlockBreakingSample/ECS/System/BrickSystem.hpp>

#include <Tsukino/EngineIntegration/EngineAPI.hpp>
#include <Tsukino/EngineIntegration/EngineContext.hpp>
#include <Tsukino/Core/Input/InputSystem.hpp>
#include <Tsukino/Engine/Asset/AssetManager.hpp>
#include <Tsukino/Core/Path.hpp>

// 必要なシステムとコンポーネントのインクルード
#include <Tsukino/EngineIntegration/ECS/System/TransformSystem.hpp>
#include <Tsukino/EngineIntegration/ECS/System/CameraSystem.hpp>
#include <Tsukino/EngineIntegration/ECS/System/FontRendererSystem.hpp>
#include <Tsukino/EngineIntegration/ECS/System/PhysicsSystem.hpp>
#include <Tsukino/EngineIntegration/Scene/GameSceneManager.hpp>

#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/CameraComponent.hpp>
#include <Tsukino/EngineIntegration/ECS/System/ModelSystem.hpp>
#include <Tsukino/BuiltIn/ECS/Component/ModelComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/CollisionComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/RigidBodyComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/FontComponent.hpp>

#include <entt/entt.hpp>
#include <hlsl++.h>
// 名前空間 : Tsukino::Sandbox
namespace Tsukino::Sandbox {
    //-------------------------------------------------------------
    //! @brief  シーン固有の初期化処理
    //-------------------------------------------------------------
    void BlockBreakingSampleScene::OnInitialize(Tsukino::EngineIntegration::EngineAPI& api) {
        // コンテキストをレジストリから取得
        Tsukino::EngineIntegration::EngineContext* context = m_scene.GetRegistry().GetContext<Tsukino::EngineIntegration::EngineContext*>();
        //-------------------------------------------------------------
        // イベントバスをレジストリから取得
        //-------------------------------------------------------------
        Tsukino::ECS::EventBus& eventBus = m_scene.GetEventBus();

        // Transformは一番最初に計算する (優先度 0)
        m_scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::TransformSystem>(), 0);
        // カメラは描画前に更新する (優先度 5)
        m_scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::CameraSystem>(), 5);
        // パドルの操作 (優先度 6)
        m_scene.AddSystem(std::make_shared<BlockBreakingSample::ECS::PaddleSystem>(), 6);
        // ボールの操作 (優先度 7)
        m_scene.AddSystem(std::make_shared<BlockBreakingSample::ECS::BallSystem>(), 7);
        // フォント描画 (優先度 9)
        m_scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::FontRendererSystem>(), 9);
        // モデル描画 (優先度 10)
        m_scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::ModelSystem>(), 10);
        // コリジョンの更新は最後に行う (優先度 12)
        m_scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::PhysicsSystem>(eventBus), 12);
        // ブロックの状態更新 (優先度 13)
        m_scene.AddSystem(std::make_shared<BlockBreakingSample::ECS::BrickSystem>(), 13);

        Tsukino::ECS::Registry& registry = m_scene.GetRegistry();

        //--------------------------------------------------------------
        // 2Dカメラエンティティの生成
        //--------------------------------------------------------------
        {
            Tsukino::ECS::Entity cameraEntity2D = m_scene.CreateEntity();

            // TransformComponent (カメラの位置)
            Tsukino::BuiltIn::ECS::TransformComponent& camTransform2D = registry.AddComponent<Tsukino::BuiltIn::ECS::TransformComponent>(cameraEntity2D);
            camTransform2D.position                                   = hlslpp::float3(0.0f, 0.0f, -10.0f);    // 手前に引く

            // CameraComponent (投影設定)
            Tsukino::BuiltIn::ECS::CameraComponent& camera2D = registry.AddComponent<Tsukino::BuiltIn::ECS::CameraComponent>(cameraEntity2D);
            camera2D.projectionType                          = Tsukino::BuiltIn::ECS::CameraComponent::ProjectionType::Orthographic;
            camera2D.orthoSize                               = 720.0f;    // 画面の縦幅を 720 ユニットにする
            camera2D.isPrimary                               = false;     // これをメインカメラにしない
        }

        //--------------------------------------------------------------
        // 3Dカメラエンティティの生成
        //--------------------------------------------------------------
        {
            Tsukino::ECS::Entity cameraEntity3D = m_scene.CreateEntity();

            // TransformComponent (カメラの位置)
            Tsukino::BuiltIn::ECS::TransformComponent& camTransform3D = registry.AddComponent<Tsukino::BuiltIn::ECS::TransformComponent>(cameraEntity3D);
            camTransform3D.position                                   = hlslpp::float3(0.0f, 0.0f, -500.0f);    // 手前に引く

            // CameraComponent (投影設定)
            Tsukino::BuiltIn::ECS::CameraComponent& camera3D = registry.AddComponent<Tsukino::BuiltIn::ECS::CameraComponent>(cameraEntity3D);
            camera3D.useLookAt                               = true;                                // 注視点を向くようにする
            camera3D.lookAtTarget                            = hlslpp::float3(0.0f, 0.0f, 0.0f);    // 注視点は原点
            camera3D.projectionType                          = Tsukino::BuiltIn::ECS::CameraComponent::ProjectionType::Perspective;
            camera3D.fov                                     = 60.0f;    // 視野角
            camera3D.isPrimary                               = true;     // これをメインカメラにする
        }

        //--------------------------------------------------------------
        // パドル
        //--------------------------------------------------------------
        Tsukino::ECS::Entity paddleEntity = m_scene.CreateEntity();
        {
            // TransformComponent
            Tsukino::BuiltIn::ECS::TransformComponent& modelTransform = registry.AddComponent<Tsukino::BuiltIn::ECS::TransformComponent>(paddleEntity);
            modelTransform.position                                   = hlslpp::float3(0.0f, -200.0f, 0.0f);
            modelTransform.rotation                                   = hlslpp::quaternion(0.0f, 0.0f, 0.0f, 1.0f);    // 無回転
            modelTransform.scale                                      = hlslpp::float3(1.0f, 1.0f, 1.0f);
            modelTransform.dirty                                      = true;          // 初回計算のためフラグを立てる
            modelTransform.parent                                     = entt::null;    // 親なし

            // ModelComponent
            Tsukino::BuiltIn::ECS::ModelComponent& model = registry.AddComponent<Tsukino::BuiltIn::ECS::ModelComponent>(paddleEntity);
            model.modelHandle                            = context->assetManager->Load(Tsukino::Core::Path("Tsukino.Sandbox/Assets/Models/Paddle.fbx"));
            model.visible                                = true;
            // コリジョンをつける
            Tsukino::BuiltIn::ECS::CollisionComponent& collision = registry.AddComponent<Tsukino::BuiltIn::ECS::CollisionComponent>(paddleEntity);
            collision.extent                                     = {80.0f, 20.0f, 20.0f};    // パドルの当たり判定
            collision.type                                       = Tsukino::BuiltIn::ECS::ColliderType::Box;
            // RBをつける
            Tsukino::BuiltIn::ECS::RigidbodyComponent& rb = registry.AddComponent<Tsukino::BuiltIn::ECS::RigidbodyComponent>(paddleEntity);
            rb.type                                       = Tsukino::BuiltIn::ECS::RigidbodyType::Kinematic;
            // パドルコンポーネントを追加
            BlockBreakingSample::ECS::PaddleComponent& paddle = registry.AddComponent<BlockBreakingSample::ECS::PaddleComponent>(paddleEntity);
        }

        //--------------------------------------------------------------
        // ボール
        //--------------------------------------------------------------
        {
            Tsukino::ECS::Entity ballEntity = m_scene.CreateEntity();
            // TransformComponent
            Tsukino::BuiltIn::ECS::TransformComponent& ballTransform = registry.AddComponent<Tsukino::BuiltIn::ECS::TransformComponent>(ballEntity);
            ballTransform.position                                   = hlslpp::float3(0.0f, 0.0f, 0.0f);
            ballTransform.rotation                                   = hlslpp::quaternion(0.0f, 0.0f, 0.0f, 1.0f);    // 無回転
            ballTransform.scale                                      = hlslpp::float3(1.0f, 1.0f, 1.0f);
            ballTransform.dirty                                      = true;          // 初回計算のためフラグを立てる
            ballTransform.parent                                     = entt::null;    // 親なし

            // ModelComponent
            Tsukino::BuiltIn::ECS::ModelComponent& model = registry.AddComponent<Tsukino::BuiltIn::ECS::ModelComponent>(ballEntity);
            model.modelHandle                            = context->assetManager->Load(Tsukino::Core::Path("Tsukino.Sandbox/Assets/Models/Ball.fbx"));
            model.visible                                = true;
            // コリジョンをつける
            Tsukino::BuiltIn::ECS::CollisionComponent& collision = registry.AddComponent<Tsukino::BuiltIn::ECS::CollisionComponent>(ballEntity);
            collision.extent                                     = {20.0f, 20.0f, 20.0f};    // ボールの当たり判定
            collision.type                                       = Tsukino::BuiltIn::ECS::ColliderType::Sphere;

            Tsukino::BuiltIn::ECS::CollisionComponent& ballCol = registry.GetComponent<Tsukino::BuiltIn::ECS::CollisionComponent>(ballEntity);
            // コールバックを設定
            ballCol.onCollisionEnter = [&registry, ballEntity, this](entt::entity other) {
                // 壁・ブロック・パドルのいずれでもないなら無視
                bool isWall   = registry.HasComponent<BlockBreakingSample::ECS::WallComponent>(other);
                bool isBrick  = registry.HasComponent<BlockBreakingSample::ECS::BrickComponent>(other);
                bool isPaddle = registry.HasComponent<BlockBreakingSample::ECS::PaddleComponent>(other);

                if(!isWall && !isBrick && !isPaddle)
                    return;

                auto& rb       = registry.GetComponent<Tsukino::BuiltIn::ECS::RigidbodyComponent>(ballEntity);
                auto& ballTf   = registry.GetComponent<Tsukino::BuiltIn::ECS::TransformComponent>(ballEntity);
                auto& otherTf  = registry.GetComponent<Tsukino::BuiltIn::ECS::TransformComponent>(other);
                auto& otherCol = registry.GetComponent<Tsukino::BuiltIn::ECS::CollisionComponent>(other);

                // --- パドル独自の反射処理 ---
                if(isPaddle) {
                    // パドルの中心からの距離を -1.0 ~ 1.0 で正規化
                    // パドルの横幅(extent.x * 2)に対して、ボールがどこに当たったか
                    float relativeHitPos = (ballTf.position.x - otherTf.position.x) / otherCol.extent.x;

                    // 安全のために範囲をクランプ
                    relativeHitPos = std::clamp(relativeHitPos, -1.0f, 1.0f);

                    // 新しい速度ベクトルを計算
                    // 最大反射角（例えば60度）を定義
                    const float maxAngle = 60.0f * (3.14159f / 180.0f);
                    float       angle    = relativeHitPos * maxAngle;

                    // 現在の速さを維持
                    float speed = hlslpp::length(rb.linearVelocity);

                    // 新しい方向ベクトル (x = sin, y = cos で上向きを基準にする)
                    rb.linearVelocity = hlslpp::float3(std::sin(angle) * speed, std::cos(angle) * speed, 0.0f);

                    // 押し戻し（パドルの上に乗らないように）
                    ballTf.position.y = otherTf.position.y + otherCol.extent.y + 5.0f;
                    ballTf.dirty      = true;
                    return;    // パドル処理をしたのでここで終了
                }

                // --- 壁の場合 ---
                if(isWall) {
                    auto& wall = registry.GetComponent<BlockBreakingSample::ECS::WallComponent>(other);
                    if(wall.isDeadZone) {
                        isGameOver = true;    // ミス判定の壁に当たったらゲームオーバー
                    }
                }

                // --- 通常の壁・ブロックの反射処理（既存のロジック） ---
                hlslpp::float3 diff = ballTf.position - otherTf.position;
                float          nx   = diff.x / (otherCol.extent.x + 0.001f);
                float          ny   = diff.y / (otherCol.extent.y + 0.001f);

                hlslpp::float3 normal = hlslpp::float3(0, 0, 0);
                if(std::abs(nx) > std::abs(ny)) {
                    normal.x = (nx > 0) ? 1.0f : -1.0f;
                } else {
                    normal.y = (ny > 0) ? 1.0f : -1.0f;
                }

                float dot = hlslpp::dot(rb.linearVelocity, normal);
                if(dot < -0.01f) {
                    rb.linearVelocity  = rb.linearVelocity - 2.0f * dot * normal;
                    ballTf.position   += normal * 5.0f;
                    ballTf.dirty       = true;
                }
            };

            // RBをつける
            Tsukino::BuiltIn::ECS::RigidbodyComponent& rb = registry.AddComponent<Tsukino::BuiltIn::ECS::RigidbodyComponent>(ballEntity);
            rb.type                                       = Tsukino::BuiltIn::ECS::RigidbodyType::Kinematic;
            // ボールコンポーネントを追加
            BlockBreakingSample::ECS::BallComponent& paddle = registry.AddComponent<BlockBreakingSample::ECS::BallComponent>(ballEntity);
        }
        //--------------------------------------------------------------
        // 四方に壁エンティティを作成（Zオーダー/配置設定）
        //--------------------------------------------------------------
        {
            // ステージのサイズ定義
            const float stageWidth      = 250.0f;
            const float stageHeight     = 250.0f;
            const float wallThickness   = 1.0f;      // 壁の厚み
            const float modelExtent     = 0.027f;    // モデルが大きいことを考慮
            const float collisionExtent = 20.0f;     // 当たり判定のサイズ（壁の厚み + モデルの大きさ）

            struct WallConfig {
                std::string    name;
                hlslpp::float3 position;
                hlslpp::float3 scale;
                bool           isDeadZone;    // 下の壁（ミス判定用）かどうか
            };

            WallConfig configs[] = {
                {"Wall_Left",   {-stageWidth, 0.0f, 0.0f},  {wallThickness, stageHeight * 2.0f * modelExtent, 1.0f}, false},
                {"Wall_Right",  {stageWidth, 0.0f, 0.0f},   {wallThickness, stageHeight * 2.0f * modelExtent, 1.0f}, false},
                {"Wall_Top",    {0.0f, stageHeight, 0.0f},  {stageWidth * 2.0f * modelExtent, 1.0f, wallThickness},  false},
                {"Wall_Bottom", {0.0f, -stageHeight, 0.0f}, {stageWidth * 2.0f * modelExtent, 1.0f, wallThickness},  true }
            };

            for(const auto& config : configs) {
                Tsukino::ECS::Entity wallEntity = m_scene.CreateEntity();

                // --- TransformComponent ---
                auto& wallTransform    = registry.AddComponent<Tsukino::BuiltIn::ECS::TransformComponent>(wallEntity);
                wallTransform.position = config.position;
                wallTransform.rotation = hlslpp::quaternion(0.0f, 0.0f, 0.0f, 1.0f);
                wallTransform.scale    = config.scale;
                wallTransform.dirty    = true;
                wallTransform.parent   = entt::null;

                // --- ModelComponent ---
                auto& model       = registry.AddComponent<Tsukino::BuiltIn::ECS::ModelComponent>(wallEntity);
                model.modelHandle = context->assetManager->Load(Tsukino::Core::Path("Tsukino.Sandbox/Assets/Models/Wall.fbx"));
                model.visible     = true;

                // --- CollisionComponent ---
                auto& collision = registry.AddComponent<Tsukino::BuiltIn::ECS::CollisionComponent>(wallEntity);
                collision.type  = Tsukino::BuiltIn::ECS::ColliderType::Box;
                // スケールに合わせた当たり判定サイズ（extent）を設定
                collision.extent = config.scale * collisionExtent;

                // --- RigidbodyComponent ---
                auto& rb = registry.AddComponent<Tsukino::BuiltIn::ECS::RigidbodyComponent>(wallEntity);
                // 壁は動かないのでKinematic
                rb.type = Tsukino::BuiltIn::ECS::RigidbodyType::Kinematic;

                // --- WallComponent ---
                auto& wallComp      = registry.AddComponent<BlockBreakingSample::ECS::WallComponent>(wallEntity);
                wallComp.isDeadZone = config.isDeadZone;    // 下の壁はミス判定用
            }
        }
        //--------------------------------------------------------------
        // ブロック（Brick）の二重ループ生成（壁の内側に配置）
        //--------------------------------------------------------------
        {
            const int   rows    = 5;        // 行数
            const int   cols    = 8;        // 列数
            const float spacing = 20.0f;    // ブロック間の隙間

            // ブロックの見た目のサイズ（10.0f * 2 = 20.0f / 5.0f * 2 = 10.0f）
            const float brickWidth  = 30.0f;
            const float brickHeight = 10.0f;

            // 全体の横幅を計算して、中央寄せの開始地点を決める
            float totalWidth = (cols * brickWidth) + ((cols - 1) * spacing);
            float startX     = -(totalWidth / 2.0f) + (brickWidth / 2.0f);
            float startY     = 150.0f;    // 画面上方の開始高さ

            for(int y = 0; y < rows; ++y) {
                for(int x = 0; x < cols; ++x) {
                    Tsukino::ECS::Entity brickEntity = m_scene.CreateEntity();

                    // --- Transform ---
                    auto& tf = registry.AddComponent<Tsukino::BuiltIn::ECS::TransformComponent>(brickEntity);
                    // グリッド状に位置を計算
                    float posX = startX + (x * (brickWidth + spacing));
                    float posY = startY - (y * (brickHeight + spacing));

                    tf.position = hlslpp::float3(posX, posY, 0.0f);
                    tf.rotation = hlslpp::quaternion(0.0f, 0.0f, 0.0f, 1.0f);
                    tf.scale    = hlslpp::float3(1.0f, 1.0f, 1.0f);
                    tf.dirty    = true;
                    tf.parent   = entt::null;

                    // --- Model ---
                    auto& model       = registry.AddComponent<Tsukino::BuiltIn::ECS::ModelComponent>(brickEntity);
                    model.modelHandle = context->assetManager->Load(Tsukino::Core::Path("Tsukino.Sandbox/Assets/Models/Block.fbx"));
                    model.visible     = true;

                    // --- Collision ---
                    auto& col = registry.AddComponent<Tsukino::BuiltIn::ECS::CollisionComponent>(brickEntity);
                    col.type  = Tsukino::BuiltIn::ECS::ColliderType::Box;
                    //
                    col.extent = hlslpp::float3(20.0f, 10.0f, 10.0f);

                    // --- Rigidbody ---
                    auto& rb = registry.AddComponent<Tsukino::BuiltIn::ECS::RigidbodyComponent>(brickEntity);
                    rb.type  = Tsukino::BuiltIn::ECS::RigidbodyType::Kinematic;

                    // --- BrickComponent & Callback ---
                    auto& brick = registry.AddComponent<BlockBreakingSample::ECS::BrickComponent>(brickEntity);
                    brick.dead  = false;    // 初期化

                    col.onCollisionEnter = [&registry, brickEntity](entt::entity other) {
                        if(registry.HasComponent<BlockBreakingSample::ECS::BallComponent>(other)) {
                            if(registry.IsValid(brickEntity)) {
                                auto& b = registry.GetComponent<BlockBreakingSample::ECS::BrickComponent>(brickEntity);
                                b.dead  = true;    // フラグを立てて後で消去
                            }
                        }
                    };
                }
            }
        }
        //--------------------------------------------------------------
        // フォントUIエンティティの生成(ナビ)
        //--------------------------------------------------------------
        {
            Tsukino::ECS::Entity                       naviEntity    = m_scene.CreateEntity();
            Tsukino::BuiltIn::ECS::TransformComponent& fontTransform = registry.AddComponent<Tsukino::BuiltIn::ECS::TransformComponent>(naviEntity);
            fontTransform.position                                   = hlslpp::float3(400.0f, 400.0f, 0.0f);
            fontTransform.rotation                                   = hlslpp::quaternion(0.0f, 0.0f, 0.0f, 1.0f);    // 無回転
            fontTransform.scale                                      = hlslpp::float3(1.0f, 1.0f, 1.0f);
            fontTransform.dirty                                      = true;          // 初回計算のためフラグを立てる
            fontTransform.parent                                     = entt::null;    // 親なし

            // FontRendererComponent の追加
            Tsukino::BuiltIn::ECS::FontComponent& font = registry.AddComponent<Tsukino::BuiltIn::ECS::FontComponent>(naviEntity);
            font.text                                  = L"Start Space Key";    // 描画するテキスト
        }
    }

    //-------------------------------------------------------------
    //! @brief  シーンの更新
    //-------------------------------------------------------------
    void BlockBreakingSampleScene::OnUpdate(Tsukino::EngineIntegration::EngineAPI& api, float deltaTime) {
        Tsukino::EngineIntegration::EngineContext* context  = m_scene.GetRegistry().GetContext<Tsukino::EngineIntegration::EngineContext*>();
        Tsukino::ECS::Registry&                    registry = m_scene.GetRegistry();

        switch(m_currentState) {
        case GameState::Ready:
            {
                if(context->inputSystem->IsKeyPressed(Tsukino::Input::KeyCode::Space)) {
                    m_currentState       = GameState::Playing;
                    const auto& fontView = registry.View<Tsukino::BuiltIn::ECS::FontComponent>();
                    fontView.each([&](entt::entity entity, Tsukino::BuiltIn::ECS::FontComponent& font) {
                        font.text = L"";    // 描画するテキスト
                    });
                }
            }
            break;
        case GameState::Playing:
            if(isGameOver) {
                m_currentState       = GameState::GameOver;
                const auto& fontView = registry.View<Tsukino::BuiltIn::ECS::FontComponent>();
                fontView.each([&](entt::entity entity, Tsukino::BuiltIn::ECS::FontComponent& font) {
                    font.text = L"GameOver";    // 描画するテキスト
                });

                const auto& ballView = registry.View<BlockBreakingSample::ECS::BallComponent>();
                ballView.each([&](entt::entity entity, BlockBreakingSample::ECS::BallComponent& ball) { registry.DestroyEntity(entity); });
            }
            // クリア条件の例: ブロックが全て壊れたらクリア
            {
                auto view = registry.View<BlockBreakingSample::ECS::BrickComponent>();
                if(view.empty()) {
                    m_currentState       = GameState::Clear;
                    const auto& fontView = registry.View<Tsukino::BuiltIn::ECS::FontComponent>();
                    fontView.each([&](entt::entity entity, Tsukino::BuiltIn::ECS::FontComponent& font) {
                        font.text            = L"Clear";    // 描画するテキスト
                        const auto& ballView = registry.View<BlockBreakingSample::ECS::BallComponent>();
                        ballView.each([&](entt::entity entity, BlockBreakingSample::ECS::BallComponent& ball) { registry.DestroyEntity(entity); });
                    });
                }
            }
            break;
        case GameState::Clear:
        case GameState::GameOver:
            if(context->inputSystem->IsKeyPressed(Tsukino::Input::KeyCode::Space)) {
                context->gameSceneManager->ChangeScene(std::make_unique<Tsukino::Sandbox::BlockBreakingSampleScene>());
            }
            break;
        }
        m_scene.Update(deltaTime);
    }

    //-------------------------------------------------------------
    //! @brief  シーンの終了処理
    //-------------------------------------------------------------
    void BlockBreakingSampleScene::OnExit() {
        // シーン終了時の解放処理などが必要な場合はここに記述します
    }

}    // namespace Tsukino::Sandbox
