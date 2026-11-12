//----------------------------------------------------------------------------
//! @file   AmbientParticlePassGpu.hpp
//! @brief  AmbientParticlePass の DX11 の中身（Renderer の内部用）
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/Core/WindowsLean.hpp>
#include <Tsukino/Renderer/AmbientParticlePass.hpp>

#include <d3d11.h>
#include <wrl/client.h>

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {
    //------------------------------------------------------------------------
    //! AmbientParticlePass の GPU の資源です。
    //------------------------------------------------------------------------
    struct AmbientParticlePass::Gpu {
        Microsoft::WRL::ComPtr<ID3D11VertexShader> vs;                 // 環境パーティクル用頂点シェーダー
        Microsoft::WRL::ComPtr<ID3D11PixelShader>  ps;                 // 環境パーティクル用ピクセルシェーダー
        Microsoft::WRL::ComPtr<ID3D11Buffer>       buffer;             // 環境パーティクルパラメータ用バッファ (b10)
    };
}    // namespace Tsukino::Renderer
