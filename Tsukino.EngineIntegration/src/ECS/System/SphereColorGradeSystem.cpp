//--------------------------------------------------------------
//! @file   SphereColorGradeSystem.cpp
//! @brief  球形の色調補正システムの実装
//--------------------------------------------------------------
#include <Tsukino/EngineIntegration/ECS/System/SphereColorGradeSystem.hpp>
#include <Tsukino/EngineIntegration/EngineContext.hpp>
#include <Tsukino/BuiltIn/ECS/Component/SphereColorGradeComponent.hpp>
#include <Tsukino/Renderer/Renderer.hpp>
#include <Tsukino/Renderer/ConstantBuffer.hpp>

#include <entt/entt.hpp>

// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {
    //--------------------------------------------------------------
    //! @brief システムの更新
    //--------------------------------------------------------------
    void SphereColorGradeSystem::Update(Tsukino::ECS::Registry& registry, float /*deltaTime*/) {
        Tsukino::EngineIntegration::EngineContext* ctx = registry.GetContext<Tsukino::EngineIntegration::EngineContext*>();
        if(!ctx || !ctx->renderer)
            return;

        //--------------------------------------------------------------
        // 有効なものを1つだけ使う（FogSystemと同じ流儀）。
        // 半径や効きが0なら画面は変わらないので、描画ごと省く
        //--------------------------------------------------------------
        const SphereColorGradeComponent* active = nullptr;

        auto view = registry.View<SphereColorGradeComponent>();
        view.each([&](entt::entity, const SphereColorGradeComponent& grade) {
            if(!active && grade.enabled && grade.radius > 0.0f && grade.strength > 0.0f)
                active = &grade;
        });

        if(!active) {
            ctx->renderer->GetSphereColorGrade().SetEnabled(false);
            return;
        }

        Tsukino::Renderer::CBufferSphereColorGrade params{};
        params.sphere     = hlslpp::float4(active->center.x, active->center.y, active->center.z, active->radius);
        params.tint       = hlslpp::float4(active->tint.x, active->tint.y, active->tint.z, active->strength);
        params.fogColor   = hlslpp::float4(active->fogColor.x, active->fogColor.y, active->fogColor.z, active->fogDensity);
        params.edgeColor  = hlslpp::float4(active->edgeColor.x, active->edgeColor.y, active->edgeColor.z, active->edgeWidth);
        params.edgeParams = hlslpp::float4(active->edgeSoftness, 0.0f, 0.0f, 0.0f);

        ctx->renderer->GetSphereColorGrade().SetParameters(params);
        ctx->renderer->GetSphereColorGrade().SetEnabled(true);
    }

}    // namespace Tsukino::BuiltIn::ECS
