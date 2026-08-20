//--------------------------------------------------------------
//! @file   DirectionalLightSystem.cpp
//! @brief  ライトシステムの実装
//! @author 山﨑愛
//--------------------------------------------------------------
#include <Tsukino/EngineIntegration/ECS/System/DirectionalLightSystem.hpp>
#include <Tsukino/EngineIntegration/EngineContext.hpp>
#include <Tsukino/BuiltIn/BuiltInAssets.hpp>
#include <Tsukino/BuiltIn/ECS/Component/DirectionalLightComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/PointLightComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/SpotLightComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>
#include <Tsukino/Engine/Asset/AssetManager.hpp>
#include <Tsukino/Engine/Asset/Shader/ShaderAsset.hpp>
#include <Tsukino/Renderer/Renderer.hpp>
#include <Tsukino/Renderer/ConstantBuffer.hpp>
#include <Tsukino/Core/Log.hpp>
#include <Tsukino/Core/typedef.hpp>

#include <Tsukino/GraphicsCommon/Vertex/VertexFormat.hpp>

#include <entt/entt.hpp>

#include <cmath>
#include <vector>

// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {
    namespace {
        //! @brief 度からラジアンへの変換係数
        constexpr float kDegToRad = 3.14159265358979323846f / 180.0f;
    }    // namespace

    //--------------------------------------------------------------
    //! @brief システムの更新
    //--------------------------------------------------------------
    void LightSystem::Update(Tsukino::ECS::Registry& registry, float deltaTime) {
        auto* ctx = registry.GetContext<Tsukino::EngineIntegration::EngineContext*>();
        if(!ctx || !ctx->renderer)
            return;

        //--------------------------------------------------------------
        // ディレクショナルライト（影付き、複数あれば最後の1つで上書き）
        //--------------------------------------------------------------
        auto dirView = registry.View<DirectionalLightComponent>();
        dirView.each([&](entt::entity entity, const DirectionalLightComponent& light) {
            if(!light.castShadow)
                return;

            ctx->renderer->SetDirectionalLight(light.direction, light.color, light.intensity);
        });

        //--------------------------------------------------------------
        // 点光源・スポットライト（影なし、ディファードLightingパス用の配列にまとめる）
        //--------------------------------------------------------------
        std::vector<Tsukino::Renderer::GPULight> gpuLights;
        gpuLights.reserve(Tsukino::Renderer::MAX_LIGHTS);

        auto pointView = registry.View<TransformComponent, PointLightComponent>();
        pointView.each([&](entt::entity entity, const TransformComponent& transform, const PointLightComponent& light) {
            if(!light.enabled)
                return;

            Tsukino::Renderer::GPULight gpuLight{};
            gpuLight.positionRange  = hlslpp::float4(transform.position.x, transform.position.y, transform.position.z, light.range);
            gpuLight.colorIntensity = hlslpp::float4(light.color.x, light.color.y, light.color.z, light.intensity);
            gpuLight.directionType  = hlslpp::float4(0.0f, 0.0f, 0.0f, 0.0f);    // 0 = Point
            gpuLight.spotParams     = hlslpp::float4(0.0f, 0.0f, 0.0f, 0.0f);

            gpuLights.push_back(gpuLight);
        });

        auto spotView = registry.View<TransformComponent, SpotLightComponent>();
        spotView.each([&](entt::entity entity, const TransformComponent& transform, const SpotLightComponent& light) {
            if(!light.enabled)
                return;

            hlslpp::float3 forward = hlslpp::mul(transform.rotation, hlslpp::float3(0.0f, 0.0f, 1.0f));

            float cosInner = std::cos(light.innerConeDeg * kDegToRad);
            float cosOuter = std::cos(light.outerConeDeg * kDegToRad);

            Tsukino::Renderer::GPULight gpuLight{};
            gpuLight.positionRange  = hlslpp::float4(transform.position.x, transform.position.y, transform.position.z, light.range);
            gpuLight.colorIntensity = hlslpp::float4(light.color.x, light.color.y, light.color.z, light.intensity);
            gpuLight.directionType  = hlslpp::float4(forward.x, forward.y, forward.z, 1.0f);    // 1 = Spot
            gpuLight.spotParams     = hlslpp::float4(cosInner, cosOuter, 0.0f, 0.0f);

            gpuLights.push_back(gpuLight);
        });

        ctx->renderer->SetLights(gpuLights.data(), static_cast<u32>(gpuLights.size()));
    }
}    // namespace Tsukino::BuiltIn::ECS
