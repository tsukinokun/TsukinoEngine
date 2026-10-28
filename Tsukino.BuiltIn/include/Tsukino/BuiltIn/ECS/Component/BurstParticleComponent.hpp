//----------------------------------------------------------------------------
//! @file   BurstParticleComponent.hpp
//! @brief  一度に撒く粒子1つ分の状態のコンポーネント
//----------------------------------------------------------------------------
#pragma once
#include <hlsl++.h>

// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {

    //! 一度に撒く粒子1つ分の状態です。
    //! @note   粒子は World 空間のスプライト（カメラを向く板）で、TransformComponent と SpriteComponent と一緒に付く。
    //!         BurstParticleSystem が動かし、大きさと色を寿命に沿って変え、寿命が来たら破棄する。
    //!         作るときは BurstEmitter::Emit を使う
    struct BurstParticleComponent {
        hlslpp::float3 velocity      = hlslpp::float3(0.0f, 0.0f, 0.0f);    // 速度（1秒あたり）
        float          gravity       = 0.0f;     // 下向きの加速度（負なら浮き上がる）
        float          drag          = 0.0f;     // 速度の減衰（1秒あたりの割合）
        float          age           = 0.0f;     // 経過時間（秒。負の間は出番待ちで見えない）
        float          life          = 1.0f;     // 寿命（秒）
        float          startSize     = 8.0f;     // 生まれたときの大きさ（ワールド単位）
        float          endSize       = 0.0f;     // 消えるときの大きさ
        hlslpp::float4 startColor    = hlslpp::float4(1.0f, 1.0f, 1.0f, 1.0f);    // 生まれたときの色（HDR 可）
        hlslpp::float4 endColor      = hlslpp::float4(1.0f, 1.0f, 1.0f, 0.0f);    // 消えるときの色
        float          texturePixels = 64.0f;    // テクスチャの一辺のピクセル数（大きさをワールド単位にする換算に使う）
    };
}    // namespace Tsukino::BuiltIn::ECS
