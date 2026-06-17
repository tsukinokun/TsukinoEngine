//--------------------------------------------------------------
//! @file   DirectionalLightSystem.cpp
//! @brief  ディレクショナルライトシステムの実装
//! @author 山﨑愛
//--------------------------------------------------------------
#include <Tsukino/EngineIntegration/ECS/System/DirectionalLightSystem.hpp>
#include <Tsukino/EngineIntegration/EngineContext.hpp>
#include <Tsukino/BuiltIn/BuiltInAssets.hpp>
#include <Tsukino/BuiltIn/ECS/Component/DirectionalLightComponent.hpp>
#include <Tsukino/Engine/Asset/AssetManager.hpp>
#include <Tsukino/Engine/Asset/Shader/ShaderAsset.hpp>
#include <Tsukino/Renderer/Renderer.hpp>

#include <Tsukino/GraphicsCommon/Vertex/VertexFormat.hpp>

#include <entt/entt.hpp>

// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {
    //--------------------------------------------------------------
    //! @brief システムの更新
    //--------------------------------------------------------------
    void DirectionalLightSystem::Update(Tsukino::ECS::Registry& registry, float deltaTime) {
        auto* ctx = registry.GetContext<Tsukino::EngineIntegration::EngineContext*>();
        if(!ctx || !ctx->renderer)
            return;

        auto view = registry.View<DirectionalLightComponent>();
        view.each([&](entt::entity entity, const DirectionalLightComponent& light) {
            if(!light.castShadow)
                return;

            // パラメータの転送に専念する
            ctx->renderer->SetDirectionalLight(light.direction, light.color, light.intensity);
        });
    }
}    // namespace Tsukino::BuiltIn::ECS
