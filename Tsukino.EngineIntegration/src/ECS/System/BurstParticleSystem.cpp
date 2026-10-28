//----------------------------------------------------------------------------
//! @file   BurstParticleSystem.cpp
//! @brief  一度に撒いた粒子を動かし、寿命が来たら破棄するシステムの実装
//----------------------------------------------------------------------------
#include <Tsukino/EngineIntegration/ECS/System/BurstParticleSystem.hpp>

#include <Tsukino/BuiltIn/ECS/Component/BurstParticleComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/SpriteComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>
#include <Tsukino/Core/ECS/Registry/Registry.hpp>

#include <algorithm>

// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {

    //----------------------------------------------------------------------------
    //! 粒子を更新します。
    //----------------------------------------------------------------------------
    void BurstParticleSystem::Update(Tsukino::ECS::Registry& registry, float deltaTime) {
        registry.View<TransformComponent, SpriteComponent, BurstParticleComponent>().each(
            [&](Tsukino::ECS::Entity entity, TransformComponent& transform, SpriteComponent& sprite, BurstParticleComponent& particle) {
                particle.age += deltaTime;
                if(particle.age < 0.0f) {
                    // 出番待ちの間は見せない
                    sprite.tintColor = hlslpp::float4(0.0f, 0.0f, 0.0f, 0.0f);
                    return;
                }
                if(particle.age >= particle.life) {
                    // View の反復中なので、即時ではなく破棄の予約にする
                    registry.QueueDestroy(entity);
                    return;
                }

                particle.velocity.y -= particle.gravity * deltaTime;
                particle.velocity *= std::max(0.0f, 1.0f - particle.drag * deltaTime);
                transform.position += particle.velocity * deltaTime;

                const float t     = particle.age / particle.life;
                const float size  = particle.startSize + (particle.endSize - particle.startSize) * t;
                const float scale = size / std::max(particle.texturePixels, 1.0f);
                transform.scale   = hlslpp::float3(scale, scale, 1.0f);
                transform.dirty   = true;
                sprite.tintColor  = hlslpp::lerp(particle.startColor, particle.endColor, hlslpp::float4(t));
            });
    }
}    // namespace Tsukino::BuiltIn::ECS
