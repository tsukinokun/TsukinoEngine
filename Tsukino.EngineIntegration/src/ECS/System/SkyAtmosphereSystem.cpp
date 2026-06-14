//--------------------------------------------------------------
//! @file   SkyAtmosphereSystem.cpp
//! @brief  大気散乱システムの実装
//! @author 山﨑愛
//--------------------------------------------------------------
#include <Tsukino/EngineIntegration/ECS/System/SkyAtmosphereSystem.hpp>
#include <Tsukino/EngineIntegration/EngineContext.hpp>
#include <Tsukino/BuiltIn/BuiltInAssets.hpp>
#include <Tsukino/BuiltIn/ECS/Component/SkyAtmosphereComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/DirectionalLightComponent.hpp>
#include <Tsukino/Engine/Asset/AssetManager.hpp>
#include <Tsukino/Engine/Asset/Shader/ShaderAsset.hpp>
#include <Tsukino/Renderer/Renderer.hpp>
#include <Tsukino/Renderer/ConstantBuffer.hpp>

#include <entt/entt.hpp>

// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {
    //--------------------------------------------------------------
    //! @brief システムの更新
    //--------------------------------------------------------------
    void SkyAtmosphereSystem::Update(Tsukino::ECS::Registry& registry, float deltaTime) {
        Tsukino::EngineIntegration::EngineContext* ctx = registry.GetContext<Tsukino::EngineIntegration::EngineContext*>();
        if(!ctx || !ctx->renderer)
            return;

        //--------------------------------------------------------------
        // 初回のみスカイパイプラインを構築
        //--------------------------------------------------------------
        if(!m_pipelineInitialized) {
            auto vsAsset = std::static_pointer_cast<Tsukino::Asset::ShaderAsset>(ctx->assetManager->Get(ctx->builtinAssets->shaders.skyVS));
            auto psAsset = std::static_pointer_cast<Tsukino::Asset::ShaderAsset>(ctx->assetManager->Get(ctx->builtinAssets->shaders.skyPS));

            if(vsAsset && psAsset) {
                ctx->renderer->SetSkyPipeline(vsAsset.get(), psAsset.get());
                m_pipelineInitialized = true;
            }
        }

        //--------------------------------------------------------------
        // SkyAtmosphereComponentを持つエンティティを探す
        // 複数あった場合は最初の1つだけ使用する
        //--------------------------------------------------------------
        auto skyView = registry.View<SkyAtmosphereComponent>();
        skyView.each([&](entt::entity entity, const SkyAtmosphereComponent& sky) {
            //----------------------------------------------------------
            // 太陽方向を DirectionalLightComponent から取得
            //----------------------------------------------------------
            hlslpp::float3 sunDir = hlslpp::float3(0.0f, 1.0f, 0.0f);    // デフォルト：真上

            auto lightView = registry.View<DirectionalLightComponent>();
            lightView.each([&](entt::entity lightEntity, const DirectionalLightComponent& light) {
                // ライトの方向は「光が向かう方向」なので、太陽方向は逆向き
                sunDir = -hlslpp::normalize(light.direction);
            });

            //----------------------------------------------------------
            // CBufferSkyに詰めてRendererへ転送
            //----------------------------------------------------------
            Tsukino::Renderer::CBufferSky skyData{};
            skyData.rayleighScattering = sky.rayleighScattering;
            skyData.mieScattering      = sky.mieScattering;
            skyData.mieAnisotropy      = sky.mieAnisotropy;
            skyData.sunIntensity       = sky.sunIntensity;
            skyData.atmosphereHeight   = sky.atmosphereHeight;
            skyData.planetRadius       = sky.planetRadius;
            skyData.sunDiskSize        = sky.sunDiskSize;
            skyData.padding0           = 0.0f;
            skyData.groundColor        = hlslpp::float4(sky.groundColor.x, sky.groundColor.y, sky.groundColor.z, 0.0f);
            skyData.sunDirection       = hlslpp::float4(sunDir.x, sunDir.y, sunDir.z, 0.0f);

            ctx->renderer->SetSkyParameters(skyData);
        });
    }

}    // namespace Tsukino::BuiltIn::ECS
