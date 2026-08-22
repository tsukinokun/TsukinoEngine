//--------------------------------------------------------------
//! @file   FogSystem.cpp
//! @brief  フォグシステムの実装
//! @author 山﨑愛
//--------------------------------------------------------------
#include <Tsukino/EngineIntegration/ECS/System/FogSystem.hpp>
#include <Tsukino/EngineIntegration/EngineContext.hpp>
#include <Tsukino/BuiltIn/ECS/Component/FogComponent.hpp>
#include <Tsukino/Renderer/Renderer.hpp>
#include <Tsukino/Renderer/ConstantBuffer.hpp>

#include <entt/entt.hpp>

// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {
    //--------------------------------------------------------------
    //! @brief システムの更新
    //--------------------------------------------------------------
    void FogSystem::Update(Tsukino::ECS::Registry& registry, float deltaTime) {
        Tsukino::EngineIntegration::EngineContext* ctx = registry.GetContext<Tsukino::EngineIntegration::EngineContext*>();
        if(!ctx || !ctx->renderer)
            return;

        //--------------------------------------------------------------
        // FogComponent を探す
        // 複数あった場合は最初の1つだけ使用する（SkyAtmosphereSystemと同じ流儀）
        //--------------------------------------------------------------
        const FogComponent* activeFog = nullptr;

        auto fogView = registry.View<FogComponent>();
        fogView.each([&](entt::entity entity, const FogComponent& fog) {
            if(!activeFog && fog.enabled)
                activeFog = &fog;
        });

        //--------------------------------------------------------------
        // コンポーネントが無い／無効なら明示的に切る
        //
        // Renderer 側のフラグはフレーム単位で自動的に false へ戻るので
        // 本来ここは省略できるが、「有効化しない＝無効」という暗黙の
        // 挙動に頼らず意思を明示しておく（MotionBlurSystemと同じ）。
        //--------------------------------------------------------------
        if(!activeFog) {
            ctx->renderer->SetFogEnabled(false);
            return;
        }

        //--------------------------------------------------------------
        // ノイズを流すための経過時間を進める
        //--------------------------------------------------------------
        m_time += deltaTime;

        //--------------------------------------------------------------
        // 風向きはここで正規化しておく（シェーダー側での正規化を省く）
        // ゼロベクトルが入っていたら風なしとして扱う
        //--------------------------------------------------------------
        hlslpp::float3 wind      = activeFog->windDirection;
        float          windLenSq = hlslpp::dot(wind, wind);
        if(windLenSq > 1.0e-8f) {
            wind = hlslpp::normalize(wind);
        } else {
            wind = hlslpp::float3(0.0f, 0.0f, 0.0f);
        }

        //--------------------------------------------------------------
        // パラメータを Renderer へ転送
        //--------------------------------------------------------------
        Tsukino::Renderer::CBufferFog params{};
        params.color          = hlslpp::float4(activeFog->color.x, activeFog->color.y, activeFog->color.z, activeFog->density);
        params.distanceParams = hlslpp::float4(activeFog->startDistance, activeFog->maxOpacity, activeFog->heightFogEnabled ? 1.0f : 0.0f, 0.0f);
        params.heightParams   = hlslpp::float4(activeFog->height, activeFog->heightFalloff, activeFog->heightDensity, 0.0f);
        params.sunColor       = hlslpp::float4(activeFog->sunColor.x, activeFog->sunColor.y, activeFog->sunColor.z, activeFog->sunScatterPower);
        params.noiseParams    = hlslpp::float4(activeFog->noiseScale, activeFog->noiseIntensity, m_time, activeFog->noiseEnabled ? 1.0f : 0.0f);
        params.windParams     = hlslpp::float4(wind.x, wind.y, wind.z, activeFog->windSpeed);

        ctx->renderer->SetFogParameters(params);
        ctx->renderer->SetFogEnabled(true);
    }

}    // namespace Tsukino::BuiltIn::ECS
