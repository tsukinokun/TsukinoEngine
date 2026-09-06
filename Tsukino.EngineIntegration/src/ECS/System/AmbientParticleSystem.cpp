//--------------------------------------------------------------
//! @file   AmbientParticleSystem.cpp
//! @brief  環境パーティクルシステムの実装
//! @author 山﨑愛
//--------------------------------------------------------------
#include <Tsukino/EngineIntegration/ECS/System/AmbientParticleSystem.hpp>
#include <Tsukino/EngineIntegration/EngineContext.hpp>
#include <Tsukino/BuiltIn/ECS/Component/AmbientParticleComponent.hpp>
#include <Tsukino/Renderer/Renderer.hpp>
#include <Tsukino/Renderer/ConstantBuffer.hpp>
#include <Tsukino/Core/Log.hpp>

#include <entt/entt.hpp>

#include <algorithm>

// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {
    //--------------------------------------------------------------
    //! 更新処理を行います。
    //--------------------------------------------------------------
    void AmbientParticleSystem::Update(Tsukino::ECS::Registry& registry, float deltaTime) {
        Tsukino::EngineIntegration::EngineContext* ctx = registry.GetContext<Tsukino::EngineIntegration::EngineContext*>();
        if(!ctx || !ctx->renderer)
            return;

        //--------------------------------------------------------------
        // AmbientParticleComponent を探す
        // 複数あった場合は最初の1つだけ使用する（FogSystemと同じ流儀）
        //--------------------------------------------------------------
        const AmbientParticleComponent* activeParticle = nullptr;

        auto particleView = registry.View<AmbientParticleComponent>();
        particleView.each([&](entt::entity entity, const AmbientParticleComponent& particle) {
            if(!activeParticle && particle.enabled)
                activeParticle = &particle;
        });

        //--------------------------------------------------------------
        // コンポーネントが無い／無効なら明示的に切る
        //
        // Renderer 側のフラグはフレーム単位で自動的に false へ戻るので
        // 本来ここは省略できるが、「有効化しない＝無効」という暗黙の
        // 挙動に頼らず意思を明示しておく（FogSystemと同じ）。
        //--------------------------------------------------------------
        if(!activeParticle) {
            ctx->renderer->SetAmbientParticleEnabled(false);
            return;
        }

        //--------------------------------------------------------------
        // 粒子を漂わせるための経過時間を進める
        //--------------------------------------------------------------
        m_time += deltaTime;

        //--------------------------------------------------------------
        // 粒子数の上限チェック（超過分は切り捨て、初回のみ警告する）
        //--------------------------------------------------------------
        u32 count = activeParticle->count;
        if(count > Tsukino::Renderer::kMaxAmbientParticles) {
            count = Tsukino::Renderer::kMaxAmbientParticles;
            if(!m_countOverflowWarned) {
                Tsukino::Core::Log::Error("AmbientParticleSystem - particle count exceeds the limit. Extra particles are dropped.");
                m_countOverflowWarned = true;
            }
        }

        //--------------------------------------------------------------
        // ボリュームは各軸とも0以下だとシェーダー側の除算が壊れるため下限を入れる
        //--------------------------------------------------------------
        hlslpp::float3 volume = hlslpp::float3(std::max(static_cast<float>(activeParticle->volumeSize.x), 1.0f),
                                               std::max(static_cast<float>(activeParticle->volumeSize.y), 1.0f),
                                               std::max(static_cast<float>(activeParticle->volumeSize.z), 1.0f));

        //--------------------------------------------------------------
        // 境界フェードの開始比率は1未満でなければ折り返しが見えてしまう
        //--------------------------------------------------------------
        float edgeFadeStart = std::clamp(activeParticle->edgeFadeStart, 0.0f, 0.99f);

        //--------------------------------------------------------------
        // シードはfloatに載せて渡すので、仮数部で正確に表せる24ビットに丸める
        //--------------------------------------------------------------
        float seed = static_cast<float>(activeParticle->seed & 0x00ffffffu);

        //--------------------------------------------------------------
        // パラメータを Renderer へ転送
        //--------------------------------------------------------------
        Tsukino::Renderer::CBufferAmbientParticle params{};
        params.volumeParams = hlslpp::float4(volume.x, volume.y, volume.z, m_time);
        params.fadeParams   = hlslpp::float4(edgeFadeStart, std::max(activeParticle->nearFadeDistance, 0.0f), seed, 0.0f);
        params.sizeParams   = hlslpp::float4(activeParticle->minSize, activeParticle->maxSize, activeParticle->minBrightness,
                                             activeParticle->maxBrightness);
        params.driftParams  = hlslpp::float4(activeParticle->driftVelocity.x, activeParticle->driftVelocity.y, activeParticle->driftVelocity.z,
                                             activeParticle->swayFrequency);
        params.swayParams   = hlslpp::float4(activeParticle->swayAmplitude, activeParticle->minSpeedScale, activeParticle->maxSpeedScale,
                                             std::clamp(activeParticle->twinkle, 0.0f, 1.0f));
        params.colorParams  = hlslpp::float4(activeParticle->color.x, activeParticle->color.y, activeParticle->color.z, activeParticle->intensity);

        ctx->renderer->SetAmbientParticleParameters(params, count);
        ctx->renderer->SetAmbientParticleEnabled(true);
    }

}    // namespace Tsukino::BuiltIn::ECS
