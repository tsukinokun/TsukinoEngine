//--------------------------------------------------------------
//! @file   BloomSystem.cpp
//! @brief  ブルームシステムの実装
//! @author 山﨑愛
//--------------------------------------------------------------
#include <Tsukino/EngineIntegration/ECS/System/BloomSystem.hpp>
#include <Tsukino/EngineIntegration/EngineContext.hpp>
#include <Tsukino/BuiltIn/ECS/Component/BloomComponent.hpp>
#include <Tsukino/Renderer/Renderer.hpp>
#include <Tsukino/Renderer/BloomPass.hpp>

#include <entt/entt.hpp>

#include <algorithm>

// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {
    //--------------------------------------------------------------
    //! @brief システムの更新
    //--------------------------------------------------------------
    void BloomSystem::Update(Tsukino::ECS::Registry& registry, float deltaTime) {
        (void)deltaTime;

        Tsukino::EngineIntegration::EngineContext* ctx = registry.GetContext<Tsukino::EngineIntegration::EngineContext*>();
        if(!ctx || !ctx->renderer)
            return;

        //--------------------------------------------------------------
        // BloomComponent を探す
        // 複数あった場合は最初の1つだけ使用する（MotionBlurSystemと同じ流儀）
        //--------------------------------------------------------------
        const BloomComponent* activeBloom = nullptr;

        auto bloomView = registry.View<BloomComponent>();
        bloomView.each([&](entt::entity, const BloomComponent& bloom) {
            if(!activeBloom && bloom.enabled)
                activeBloom = &bloom;
        });

        //--------------------------------------------------------------
        // コンポーネントが無い／無効なら明示的に切る（MotionBlurSystemと同じ）
        //--------------------------------------------------------------
        if(!activeBloom) {
            ctx->renderer->GetBloom().SetEnabled(false);
            return;
        }

        //--------------------------------------------------------------
        // パラメータを Renderer へ転送
        //--------------------------------------------------------------
        Tsukino::Renderer::BloomParameters params{};
        params.threshold = std::max(activeBloom->threshold, 0.0f);
        params.softKnee  = std::clamp(activeBloom->softKnee, 0.0f, 1.0f);
        params.intensity = std::max(activeBloom->intensity, 0.0f);
        params.radius    = std::max(activeBloom->radius, 0.0f);
        params.tintR     = activeBloom->tint.x;
        params.tintG     = activeBloom->tint.y;
        params.tintB     = activeBloom->tint.z;

        ctx->renderer->GetBloom().SetParameters(params);
        ctx->renderer->GetBloom().SetEnabled(true);
    }

}    // namespace Tsukino::BuiltIn::ECS
