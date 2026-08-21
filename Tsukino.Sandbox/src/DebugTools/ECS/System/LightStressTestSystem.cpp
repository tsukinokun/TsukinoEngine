//-------------------------------------------------------------
//! @file   LightStressTestSystem.cpp
//! @brief  多光源ストレステストシステムの実装
//! @author 山﨑愛
//-------------------------------------------------------------
#include <Tsukino/Sandbox/DebugTools/ECS/System/LightStressTestSystem.hpp>
#include <Tsukino/Sandbox/DebugTools/ECS/Component/LightStressTestComponent.hpp>

#include <Tsukino/EngineIntegration/EngineContext.hpp>

#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/PointLightComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/FontComponent.hpp>

#include <Tsukino/Core/Input/InputSystem.hpp>
#include <Tsukino/Core/Input/KeyCodes.hpp>
#include <Tsukino/Core/Log.hpp>

#include <entt/entt.hpp>

#include <cmath>
#include <string>

// 名前空間 : Tsukino::Sandbox::DebugTools::ECS
namespace Tsukino::Sandbox::DebugTools::ECS {
    namespace {
        //! @brief 円周率
        constexpr float kPi = 3.14159265358979323846f;

        //! @brief ライトを配置するリングの半径（ワールドユニット）
        constexpr float kOrbitRadius = 320.0f;

        //! @brief ライトの高さ範囲
        constexpr float kHeightMin = 40.0f;
        constexpr float kHeightMax = 260.0f;

        //--------------------------------------------------------------
        //! @brief 減衰は逆二乗（intensity / (d^2 + 1)）なので、
        //!        届かせたい放射輝度に距離の二乗を掛けた値が必要になる。
        //!        リング半径320からプレイヤー付近を照らす想定で d^2 ≒ 10万。
        //--------------------------------------------------------------
        constexpr float kLightIntensity = 45000.0f;
        constexpr float kLightRange     = 700.0f;

        //--------------------------------------------------------------
        //! @brief インデックスから色相をずらして虹色に配る
        //!        （多灯であることが一目で分かるようにするため）
        //--------------------------------------------------------------
        hlslpp::float3 RainbowColor(int index, int total) {
            float h = (total > 0) ? (static_cast<float>(index) / static_cast<float>(total)) : 0.0f;
            float r = 0.5f + 0.5f * std::cos(2.0f * kPi * (h + 0.0f / 3.0f));
            float g = 0.5f + 0.5f * std::cos(2.0f * kPi * (h + 1.0f / 3.0f));
            float b = 0.5f + 0.5f * std::cos(2.0f * kPi * (h + 2.0f / 3.0f));
            return hlslpp::float3(r, g, b);
        }
    }    // namespace

    //-------------------------------------------------------------
    //! @brief システムの更新
    //-------------------------------------------------------------
    void LightStressTestSystem::Update(Tsukino::ECS::Registry& registry, float deltaTime) {
        auto* ctx = registry.GetContext<Tsukino::EngineIntegration::EngineContext*>();
        if(!ctx || !ctx->inputSystem)
            return;

        //---------------------------------------------------------
        // F1でライト数を切り替える
        //---------------------------------------------------------
        if(ctx->inputSystem->IsKeyPressed(Tsukino::Input::KeyCode::F1)) {
            m_stepIndex = (m_stepIndex + 1) % static_cast<int>(std::size(kLightCountSteps));

            DestroyLights(registry);
            SpawnLights(registry, kLightCountSteps[m_stepIndex]);

            Tsukino::Core::Log::Info("LightStressTest: point light count = " + std::to_string(kLightCountSteps[m_stepIndex]));
        }

        //---------------------------------------------------------
        // 公転アニメーション
        // 静止していると本当に多灯出ているのか判別しづらいため動かす
        //---------------------------------------------------------
        m_orbitPhase += deltaTime * 0.35f;
        if(m_orbitPhase > 2.0f * kPi)
            m_orbitPhase -= 2.0f * kPi;

        const int lightCount = static_cast<int>(m_lights.size());
        for(int i = 0; i < lightCount; ++i) {
            Tsukino::ECS::Entity e = m_lights[i];
            if(!registry.HasComponent<Tsukino::BuiltIn::ECS::TransformComponent>(e))
                continue;

            auto& transform = registry.GetComponent<Tsukino::BuiltIn::ECS::TransformComponent>(e);

            // リング上に等間隔で並べ、全体をゆっくり回す
            float angle  = (2.0f * kPi * static_cast<float>(i) / static_cast<float>(lightCount)) + m_orbitPhase;
            float height = kHeightMin + (kHeightMax - kHeightMin) * (0.5f + 0.5f * std::sin(angle * 3.0f + m_orbitPhase));

            transform.position = hlslpp::float3(std::cos(angle) * kOrbitRadius, height, std::sin(angle) * kOrbitRadius);
            transform.dirty    = true;    // TransformSystemにworldMatrixを再計算させる
        }

        //---------------------------------------------------------
        // フレーム時間の移動平均（直近1秒）
        //---------------------------------------------------------
        m_frameTimeAccum += deltaTime;
        ++m_frameCount;
        if(m_frameTimeAccum >= 1.0f) {
            m_avgFrameMs     = (m_frameTimeAccum / static_cast<float>(m_frameCount)) * 1000.0f;
            m_frameTimeAccum = 0.0f;
            m_frameCount     = 0;
        }

        //---------------------------------------------------------
        // HUDへ反映
        //---------------------------------------------------------
        auto hudView = registry.View<LightStressTestHudComponent, Tsukino::BuiltIn::ECS::FontComponent>();
        hudView.each([&](entt::entity entity, const LightStressTestHudComponent&, Tsukino::BuiltIn::ECS::FontComponent& font) {
            wchar_t buf[128]{};
            swprintf_s(buf,
                       L"[F1] PointLights: %d   %.2f ms (%.0f fps)",
                       kLightCountSteps[m_stepIndex],
                       m_avgFrameMs,
                       (m_avgFrameMs > 0.0f) ? (1000.0f / m_avgFrameMs) : 0.0f);
            font.text = buf;
        });
    }

    //-------------------------------------------------------------
    //! @brief 現在のライトをすべて破棄する
    //-------------------------------------------------------------
    void LightStressTestSystem::DestroyLights(Tsukino::ECS::Registry& registry) {
        // System の中からの破棄は必ず QueueDestroy を使う（即時破棄はイテレータを壊す）
        for(Tsukino::ECS::Entity e : m_lights) {
            registry.QueueDestroy(e);
        }
        m_lights.clear();
    }

    //-------------------------------------------------------------
    //! @brief 指定個数の点光源を生成する
    //-------------------------------------------------------------
    void LightStressTestSystem::SpawnLights(Tsukino::ECS::Registry& registry, int count) {
        m_lights.reserve(static_cast<size_t>(count));

        for(int i = 0; i < count; ++i) {
            Tsukino::ECS::Entity e = registry.CreateEntity();

            float angle  = 2.0f * kPi * static_cast<float>(i) / static_cast<float>(count > 0 ? count : 1);
            float height = kHeightMin + (kHeightMax - kHeightMin) * (0.5f + 0.5f * std::sin(angle * 3.0f));

            // PointLightComponentはTransformComponentとセットで初めてLightSystemに拾われる
            auto& transform    = registry.AddComponent<Tsukino::BuiltIn::ECS::TransformComponent>(e);
            transform.position = hlslpp::float3(std::cos(angle) * kOrbitRadius, height, std::sin(angle) * kOrbitRadius);
            transform.rotation = hlslpp::quaternion(0.0f, 0.0f, 0.0f, 1.0f);
            transform.scale    = hlslpp::float3(1.0f, 1.0f, 1.0f);
            transform.dirty    = true;
            transform.parent   = entt::null;

            auto& light     = registry.AddComponent<Tsukino::BuiltIn::ECS::PointLightComponent>(e);
            light.color     = RainbowColor(i, count);
            light.intensity = kLightIntensity;
            light.range     = kLightRange;
            light.enabled   = true;

            m_lights.push_back(e);
        }
    }

}    // namespace Tsukino::Sandbox::DebugTools::ECS
