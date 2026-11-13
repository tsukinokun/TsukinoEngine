//----------------------------------------------------------------------------
//! @file   TextureStages.hpp
//! @brief  bgfx 版のテクスチャの番号（ステージ）（Renderer の内部用）
//! @detail bgfx のテクスチャは 16 枚（0〜15）までなので、DX11 版の t8〜t20 を詰め直しています。
//!         シェーダー側の Shaders/bgfx/TextureStages.sh と必ず揃えてください。
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/Core/typedef.hpp>

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer::BgfxStage {

    // マテリアル（DX11 版の t0〜t4。SRVSlot と同じ番号）
    inline constexpr u8 Albedo            = 0;
    inline constexpr u8 NormalMap         = 1;
    inline constexpr u8 MetallicRoughness = 2;
    inline constexpr u8 EmissiveMap       = 3;
    inline constexpr u8 AOMap             = 4;

    // 影（DX11 版の t8）
    inline constexpr u8 ShadowMap = 5;

    // G-Buffer（DX11 版の t9〜t14。albedo・法線・材質・発光・深度・ワールド座標の順）
    inline constexpr u8 GBufferAlbedo   = 6;
    inline constexpr u8 GBufferNormal   = 7;
    inline constexpr u8 GBufferMaterial = 8;
    inline constexpr u8 GBufferEmissive = 9;
    inline constexpr u8 GBufferDepth    = 10;
    inline constexpr u8 GBufferWorldPos = 11;

    // IBL（DX11 版の t17〜t19）
    inline constexpr u8 IBLIrradiance = 12;
    inline constexpr u8 IBLSpecular   = 13;
    inline constexpr u8 IBLBrdfLut    = 14;

}    // namespace Tsukino::Renderer::BgfxStage
