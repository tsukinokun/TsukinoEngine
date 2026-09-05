//-------------------------------------------------------------
//! @file    DeferredLightSampleScene.cpp
//! @brief   ディファードレンダリングの多光源ショーケースシーンの実装
//! @author  山﨑愛
//-------------------------------------------------------------
#include <Tsukino/Sandbox/Scene/DeferredLightSampleScene.hpp>

#ifdef _DEBUG
#include <Tsukino/Sandbox/DebugTools/ECS/System/LightStressTestSystem.hpp>
#include <Tsukino/Sandbox/DebugTools/ECS/Component/LightStressTestComponent.hpp>
#endif

#include <Tsukino/EngineIntegration/EngineAPI.hpp>
#include <Tsukino/EngineIntegration/EngineContext.hpp>
#include <Tsukino/Engine/Asset/AssetManager.hpp>

#include <Tsukino/EngineIntegration/ECS/System/TransformSystem.hpp>
#include <Tsukino/EngineIntegration/ECS/System/CameraSystem.hpp>
#include <Tsukino/EngineIntegration/ECS/System/ModelSystem.hpp>
#include <Tsukino/EngineIntegration/ECS/System/FontRendererSystem.hpp>
#include <Tsukino/EngineIntegration/ECS/System/LightSystem.hpp>
#include <Tsukino/EngineIntegration/ECS/System/SkyAtmosphereSystem.hpp>

#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/CameraComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/ModelComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/FontComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/DirectionalLightComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/PointLightComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/SpotLightComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/SkyAtmosphereComponent.hpp>

#include <Tsukino/Core/Path.hpp>
#include <Tsukino/Core/Log.hpp>

#include <entt/entt.hpp>
#include <hlsl++.h>

#include <cmath>

// 名前空間 : Tsukino::Sandbox
namespace Tsukino::Sandbox {
    namespace {
        constexpr float kPi = 3.14159265358979323846f;

        //! @brief 1ユニット≒1cm規約。他シーンに合わせる
        constexpr float kGroundY = 0.0f;
    }    // namespace

    //-------------------------------------------------------------
    //! @brief  シーン固有の初期化処理
    //-------------------------------------------------------------
    void DeferredLightSampleScene::OnInitialize(Tsukino::EngineIntegration::EngineAPI& api) {
        Tsukino::EngineIntegration::EngineContext* context = m_scene.GetRegistry().GetContext<Tsukino::EngineIntegration::EngineContext*>();

        //--------------------------------------------------------------
        // 空を暗めにして、点光源の効果が見えやすい「夜」の絵作りにする
        //--------------------------------------------------------------
        context->renderer->SetClearColor(0.0f, 0.0f, 0.0f, 0.0f);

        //--------------------------------------------------------------
        // システムの生成と追加
        //--------------------------------------------------------------
        enum class SystemPriority : int {
            LightStressTest = -1,    // ライトのスポーン/移動はworldMatrix確定前に行う
            Transform       = 0,
            Camera,
            Light,    // worldMatrixから位置を取るのでTransform系より後
            SkyAtmosphere,
            Font,
            Render,
        };

#ifdef _DEBUG
        m_scene.AddSystem(std::make_shared<DebugTools::ECS::LightStressTestSystem>(), (int)SystemPriority::LightStressTest);
#endif
        m_scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::TransformSystem>(), (int)SystemPriority::Transform);
        m_scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::CameraSystem>(), (int)SystemPriority::Camera);
        m_scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::LightSystem>(), (int)SystemPriority::Light);
        m_scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::SkyAtmosphereSystem>(), (int)SystemPriority::SkyAtmosphere);
        m_scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::FontRendererSystem>(), (int)SystemPriority::Font);
        m_scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::ModelSystem>(), (int)SystemPriority::Render);

        //--------------------------------------------------------------
        // アセットのロード
        //--------------------------------------------------------------
        Tsukino::Asset::AssetHandle blockHandle =
            context->assetManager->Load(Tsukino::Core::Path("Tsukino.Sandbox/Assets/Models/Block.fbx"));
        Tsukino::Asset::AssetHandle characterHandle =
            context->assetManager->Load(Tsukino::Core::Path("Tsukino.Sandbox/Assets/Models/CharaTest.fbx"));
        Tsukino::Asset::AssetHandle centerPieceHandle =
            context->assetManager->Load(Tsukino::Core::Path("Tsukino.Sandbox/Assets/Models/Ball.fbx"));

        Tsukino::ECS::Registry& registry = m_scene.GetRegistry();

        //--------------------------------------------------------------
        // 床（Block.fbxを平たく引き伸ばす）
        // 点光源の減衰が床に落ちる様子が、多光源の効きを一番分かりやすく見せる
        //--------------------------------------------------------------
        {
            Tsukino::ECS::Entity                       e         = m_scene.CreateEntity();
            Tsukino::BuiltIn::ECS::TransformComponent& transform = registry.AddComponent<Tsukino::BuiltIn::ECS::TransformComponent>(e);
            transform.position                                   = hlslpp::float3(0.0f, kGroundY - 5.0f, 0.0f);
            transform.rotation                                   = hlslpp::quaternion(0.0f, 0.0f, 0.0f, 1.0f);
            transform.scale                                      = hlslpp::float3(40.0f, 0.2f, 40.0f);
            transform.dirty                                      = true;
            transform.parent                                     = entt::null;

            Tsukino::BuiltIn::ECS::ModelComponent& model = registry.AddComponent<Tsukino::BuiltIn::ECS::ModelComponent>(e);
            model.modelHandle                            = blockHandle;
            model.visible                                = true;
        }

        //--------------------------------------------------------------
        // キャラクターを円周上に数体（法線マップ付きなので陰影の差が出やすい）
        //--------------------------------------------------------------
        {
            constexpr int   kCharacterCount = 5;
            constexpr float kRingRadius  = 170.0f;

            for(int i = 0; i < kCharacterCount; ++i) {
                float angle = 2.0f * kPi * static_cast<float>(i) / static_cast<float>(kCharacterCount);

                Tsukino::ECS::Entity                       e         = m_scene.CreateEntity();
                Tsukino::BuiltIn::ECS::TransformComponent& transform = registry.AddComponent<Tsukino::BuiltIn::ECS::TransformComponent>(e);
                transform.position = hlslpp::float3(std::cos(angle) * kRingRadius, kGroundY, std::sin(angle) * kRingRadius);
                // 中心を向かせる（モデルのローカル+Zが正面）
                transform.rotation = hlslpp::quaternion::rotation_y(-angle + kPi * 0.5f);
                transform.scale    = hlslpp::float3(1.0f, 1.0f, 1.0f);
                transform.dirty    = true;
                transform.parent   = entt::null;

                Tsukino::BuiltIn::ECS::ModelComponent& model = registry.AddComponent<Tsukino::BuiltIn::ECS::ModelComponent>(e);
                model.modelHandle                            = characterHandle;
                model.visible                                = true;
            }
        }

        //--------------------------------------------------------------
        // 中央に球を1つ置く（曲面なので多光源のスペキュラの差が一番見やすい）
        //--------------------------------------------------------------
        {
            Tsukino::ECS::Entity                       e         = m_scene.CreateEntity();
            Tsukino::BuiltIn::ECS::TransformComponent& transform = registry.AddComponent<Tsukino::BuiltIn::ECS::TransformComponent>(e);
            transform.position                                   = hlslpp::float3(0.0f, kGroundY, 0.0f);
            transform.rotation                                   = hlslpp::quaternion(0.0f, 0.0f, 0.0f, 1.0f);
            transform.scale                                      = hlslpp::float3(2.0f, 2.0f, 2.0f);
            transform.dirty                                      = true;
            transform.parent                                     = entt::null;

            Tsukino::BuiltIn::ECS::ModelComponent& model = registry.AddComponent<Tsukino::BuiltIn::ECS::ModelComponent>(e);
            model.modelHandle                            = centerPieceHandle;
            model.visible                                = true;
        }

        //--------------------------------------------------------------
        // ディレクショナルライト（影付き1灯）
        // 点光源を主役にするため、あえて弱めにして夜の雰囲気にする
        //--------------------------------------------------------------
        {
            Tsukino::ECS::Entity                              e     = m_scene.CreateEntity();
            Tsukino::BuiltIn::ECS::DirectionalLightComponent& light = registry.AddComponent<Tsukino::BuiltIn::ECS::DirectionalLightComponent>(e);
            light.direction                                         = hlslpp::float3(0.3f, -0.8f, -0.5f);
            light.color                                             = hlslpp::float3(0.5f, 0.6f, 0.9f);
            light.intensity                                         = 0.8f;
            light.castShadow                                        = true;
        }

        //--------------------------------------------------------------
        // 常設の点光源（F1のストレステストとは別に、起動直後から効果が見える分）
        //
        // intensityの単位に注意：減衰が逆二乗（intensity / (d^2 + 1)）なので、
        // 距離200前後で効かせるには d^2 = 4万 のオーダーの値が要る。
        //--------------------------------------------------------------
        {
            struct Spec {
                hlslpp::float3 position;
                hlslpp::float3 color;
                float          intensity;
                float          range;
            };

            const Spec specs[] = {
                {hlslpp::float3(0.0f, 120.0f, 0.0f), hlslpp::float3(1.0f, 0.85f, 0.6f), 40000.0f, 600.0f},        // 中央上（暖色）
                {hlslpp::float3(260.0f, 80.0f, 0.0f), hlslpp::float3(1.0f, 0.25f, 0.2f), 55000.0f, 700.0f},       // 赤
                {hlslpp::float3(-260.0f, 80.0f, 0.0f), hlslpp::float3(0.2f, 0.5f, 1.0f), 55000.0f, 700.0f},       // 青
                {hlslpp::float3(0.0f, 80.0f, 260.0f), hlslpp::float3(0.3f, 1.0f, 0.45f), 55000.0f, 700.0f},       // 緑
                {hlslpp::float3(0.0f, 80.0f, -260.0f), hlslpp::float3(1.0f, 0.5f, 1.0f), 55000.0f, 700.0f},       // 紫
            };

            for(const auto& spec : specs) {
                Tsukino::ECS::Entity                       e         = m_scene.CreateEntity();
                Tsukino::BuiltIn::ECS::TransformComponent& transform = registry.AddComponent<Tsukino::BuiltIn::ECS::TransformComponent>(e);
                transform.position                                   = spec.position;
                transform.rotation                                   = hlslpp::quaternion(0.0f, 0.0f, 0.0f, 1.0f);
                transform.scale                                      = hlslpp::float3(1.0f, 1.0f, 1.0f);
                transform.dirty                                      = true;
                transform.parent                                     = entt::null;

                Tsukino::BuiltIn::ECS::PointLightComponent& light = registry.AddComponent<Tsukino::BuiltIn::ECS::PointLightComponent>(e);
                light.color                                      = spec.color;
                light.intensity                                  = spec.intensity;
                light.range                                      = spec.range;
                light.enabled                                    = true;
            }
        }

        //--------------------------------------------------------------
        // スポットライト（真上から中央の剣を照らす。円錐の減衰を見せる）
        // 向きはローカル+Z。X軸に+90度回すと+Zが真下(-Y)を向く
        //--------------------------------------------------------------
        {
            Tsukino::ECS::Entity                       e         = m_scene.CreateEntity();
            Tsukino::BuiltIn::ECS::TransformComponent& transform = registry.AddComponent<Tsukino::BuiltIn::ECS::TransformComponent>(e);
            transform.position                                   = hlslpp::float3(0.0f, 420.0f, 0.0f);
            transform.rotation                                   = hlslpp::quaternion::rotation_x(1.5708f);
            transform.scale                                      = hlslpp::float3(1.0f, 1.0f, 1.0f);
            transform.dirty                                      = true;
            transform.parent                                     = entt::null;

            Tsukino::BuiltIn::ECS::SpotLightComponent& light = registry.AddComponent<Tsukino::BuiltIn::ECS::SpotLightComponent>(e);
            light.color                                     = hlslpp::float3(1.0f, 0.98f, 0.9f);
            light.intensity                                 = 300000.0f;    // 距離420から照らすため d^2≒18万 を見込む
            light.range                                     = 1000.0f;
            light.innerConeDeg                              = 14.0f;
            light.outerConeDeg                              = 26.0f;
            light.enabled                                   = true;
        }

        //--------------------------------------------------------------
        // 大気散乱（空）
        //--------------------------------------------------------------
        {
            Tsukino::ECS::Entity e = m_scene.CreateEntity();
            registry.AddComponent<Tsukino::BuiltIn::ECS::SkyAtmosphereComponent>(e);
        }

        //--------------------------------------------------------------
        // カメラ
        // DebugCameraSystemは_DEBUGビルドにしか存在しないため使わず、
        // OnUpdateで中心の周りを自動公転させる。Release/Debugで同じ絵が出るうえ、
        // 全方位から光の当たり方が見えるのでショーケースとしても都合がよい。
        //--------------------------------------------------------------
        {
            m_cameraEntity                               = m_scene.CreateEntity();
            Tsukino::BuiltIn::ECS::TransformComponent& t = registry.AddComponent<Tsukino::BuiltIn::ECS::TransformComponent>(m_cameraEntity);
            t.position                                   = hlslpp::float3(0.0f, 260.0f, 480.0f);
            t.rotation                                   = hlslpp::quaternion(0.0f, 0.0f, 0.0f, 1.0f);
            t.scale                                      = hlslpp::float3(1.0f, 1.0f, 1.0f);
            t.dirty                                      = true;
            t.parent                                     = entt::null;

            Tsukino::BuiltIn::ECS::CameraComponent& cam = registry.AddComponent<Tsukino::BuiltIn::ECS::CameraComponent>(m_cameraEntity);
            cam.useLookAt                               = true;
            cam.lookAtTarget                            = hlslpp::float3(0.0f, 90.0f, 0.0f);
            cam.nearZ                                   = 1.0f;
            cam.farZ                                    = 10000.0f;
            cam.isPrimary                               = true;
        }

#ifdef _DEBUG
        //--------------------------------------------------------------
        // ストレステストHUD（F1でライト数切り替え、フレーム時間表示）
        //--------------------------------------------------------------
        {
            Tsukino::ECS::Entity                       e = m_scene.CreateEntity();
            Tsukino::BuiltIn::ECS::TransformComponent& t = registry.AddComponent<Tsukino::BuiltIn::ECS::TransformComponent>(e);
            t.position                                   = hlslpp::float3(10.0f, 10.0f, 0.0f);    // 画面左上（生スクリーンピクセル座標）
            t.scale                                      = hlslpp::float3(1.0f, 1.0f, 1.0f);
            t.dirty                                      = true;

            Tsukino::BuiltIn::ECS::FontComponent& font = registry.AddComponent<Tsukino::BuiltIn::ECS::FontComponent>(e);
            font.text                                  = L"";
            font.color                                 = hlslpp::float4(0.4f, 1.0f, 0.6f, 1.0f);
            font.origin                                = hlslpp::float2(0.0f, 0.0f);

            registry.AddComponent<DebugTools::ECS::LightStressTestHudComponent>(e);
        }
#endif    // _DEBUG

        Tsukino::Core::Log::Info("DeferredLightSampleScene: initialized. Press F1 to cycle point light count (0/1/16/64).");
    }

    //-------------------------------------------------------------
    //! @brief  シーンの更新
    //-------------------------------------------------------------
    void DeferredLightSampleScene::OnUpdate(Tsukino::EngineIntegration::EngineAPI& api, float deltaTime) {
        //--------------------------------------------------------------
        // カメラをゆっくり公転させる
        // m_scene.Update() より前に書き込むことで、同じフレームの
        // TransformSystem → CameraSystem に今回の位置が反映される
        //--------------------------------------------------------------
        Tsukino::ECS::Registry& registry = m_scene.GetRegistry();
        if(m_cameraEntity != entt::null && registry.HasComponent<Tsukino::BuiltIn::ECS::TransformComponent>(m_cameraEntity)) {
            m_cameraAngle += deltaTime * 0.15f;
            if(m_cameraAngle > 2.0f * kPi)
                m_cameraAngle -= 2.0f * kPi;

            constexpr float kOrbitRadius = 520.0f;
            constexpr float kOrbitHeight = 260.0f;

            auto& t    = registry.GetComponent<Tsukino::BuiltIn::ECS::TransformComponent>(m_cameraEntity);
            t.position = hlslpp::float3(std::sin(m_cameraAngle) * kOrbitRadius, kOrbitHeight, std::cos(m_cameraAngle) * kOrbitRadius);
            t.dirty    = true;
        }

        m_scene.Update(deltaTime);
    }

    //-------------------------------------------------------------
    //! @brief  シーンの終了処理
    //-------------------------------------------------------------
    void DeferredLightSampleScene::OnExit() {
    }

}    // namespace Tsukino::Sandbox
