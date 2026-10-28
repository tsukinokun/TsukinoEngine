//----------------------------------------------------------------------------
//! @file   BurstEmitter.hpp
//! @brief  粒子を一度に撒く関数と、その撒き方
//! @detail 「着地の砂埃」「正解の火花」のように、ある瞬間にまとめて飛び散る粒子を作ります。
//!         粒子の動きは BurstParticleSystem が担当します。撒き方（BurstParams）は
//!         BurstParamsSerialization.hpp で JSON から読めるので、場面ごとのプリセットをデータに置けます。
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/BuiltIn/ECS/Component/BurstParticleComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/SpriteComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>

#include <Tsukino/Engine/Asset/AssetHandle.hpp>
#include <Tsukino/Engine/ECS/Scene.hpp>

#include <hlsl++.h>

#include <cmath>
#include <random>

// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {

    //! 粒子1種類の撒き方です。範囲で書いた値は、粒子ごとに最小〜最大の間で乱数を振ります。
    struct BurstParams {
        int            count         = 16;        // 粒子の数
        float          speedMin      = 40.0f;     // 速さ（1秒あたり）の最小
        float          speedMax      = 120.0f;    // 速さの最大
        float          upwardBias    = 0.5f;      // 上向き成分の強さ（0 で水平に広がる）
        float          spread        = 1.0f;      // 水平方向の広がり（0 で真上だけ）
        float          lifeMin       = 0.5f;      // 寿命（秒）の最小
        float          lifeMax       = 1.0f;      // 寿命の最大
        float          sizeMin       = 6.0f;      // 生まれたときの大きさ（ワールド単位）の最小
        float          sizeMax       = 12.0f;     // 生まれたときの大きさの最大
        float          endSizeScale  = 0.2f;      // 消えるときの大きさの倍率
        float          gravity       = 120.0f;    // 下向きの加速度（負なら浮き上がる）
        float          drag          = 2.0f;      // 速度の減衰（1秒あたりの割合）
        float          radius        = 10.0f;     // 撒き始めの位置のばらつき（水平の半径）
        float          delayMax      = 0.0f;      // 出るタイミングのばらつき（秒）
        hlslpp::float4 colorA        = hlslpp::float4(1.0f, 1.0f, 1.0f, 1.0f);    // 色の候補 A
        hlslpp::float4 colorB        = hlslpp::float4(1.0f, 1.0f, 1.0f, 1.0f);    // 色の候補 B（A との間で乱数）
        float          endAlpha      = 0.0f;      // 消えるときの不透明度
        bool           additive      = true;      // 加算合成で光らせるか
        float          texturePixels = 64.0f;     // テクスチャの一辺のピクセル数
    };

    // 名前空間 : Tsukino::BuiltIn::ECS::BurstEmitter
    namespace BurstEmitter {

        //! 粒子を一度に撒きます。
        //! @param  [in]     scene    粒子を作るシーン
        //! @param  [in]     texture  粒子のテクスチャ
        //! @param  [in]     params   撒き方
        //! @param  [in]     position 撒く中心（ワールド座標）
        //! @param  [in,out] rng      ばらつき用の乱数
        inline void Emit(Tsukino::ECS::Scene& scene, Tsukino::Asset::AssetHandle texture, const BurstParams& params, const hlslpp::float3& position,
                         std::mt19937& rng) {
            constexpr float                       kTwoPi = 6.2831853f;
            std::uniform_real_distribution<float> unit(0.0f, 1.0f);
            auto                                  range = [&](float a, float b) { return a + (b - a) * unit(rng); };

            Tsukino::ECS::Registry& registry = scene.GetRegistry();
            for(int i = 0; i < params.count; ++i) {
                const float angle  = range(0.0f, kTwoPi);
                const float radius = range(0.0f, params.radius);
                const float speed  = range(params.speedMin, params.speedMax);

                // 水平に散る向きに、上向きの成分を足す（真上だけのときも正規化できるよう、ほんの少し上へずらす）
                hlslpp::float3 direction(std::cos(angle) * params.spread, range(0.3f, 1.0f) * params.upwardBias, std::sin(angle) * params.spread);
                direction = hlslpp::normalize(direction + hlslpp::float3(0.0f, 0.001f, 0.0f));

                const Tsukino::ECS::Entity entity    = scene.CreateEntity();
                auto&                      transform = registry.AddComponent<TransformComponent>(entity);
                transform.position                   = position + hlslpp::float3(std::cos(angle) * radius, 0.0f, std::sin(angle) * radius);
                transform.scale                      = hlslpp::float3(0.0f, 0.0f, 1.0f);

                auto& sprite         = registry.AddComponent<SpriteComponent>(entity);
                sprite.textureHandle = texture;
                sprite.space         = SpriteSpace::World;
                sprite.blendMode     = params.additive ? SpriteBlendMode::Additive : SpriteBlendMode::Alpha;
                sprite.raycastTarget = false;

                auto& particle         = registry.AddComponent<BurstParticleComponent>(entity);
                particle.velocity      = direction * speed;
                particle.gravity       = params.gravity;
                particle.drag          = params.drag;
                particle.age           = -range(0.0f, params.delayMax);
                particle.life          = range(params.lifeMin, params.lifeMax);
                particle.startSize     = range(params.sizeMin, params.sizeMax);
                particle.endSize       = particle.startSize * params.endSizeScale;
                particle.startColor    = hlslpp::lerp(params.colorA, params.colorB, hlslpp::float4(unit(rng)));
                particle.endColor      = hlslpp::float4(particle.startColor.xyz, params.endAlpha);
                particle.texturePixels = params.texturePixels;
                sprite.tintColor       = particle.startColor;
            }
        }
    }    // namespace BurstEmitter
}    // namespace Tsukino::BuiltIn::ECS
