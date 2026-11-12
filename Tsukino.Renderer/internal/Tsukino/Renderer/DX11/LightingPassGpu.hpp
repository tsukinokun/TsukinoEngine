//----------------------------------------------------------------------------
//! @file   LightingPassGpu.hpp
//! @brief  LightingPass の DX11 の中身（Renderer の内部用）
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/Core/WindowsLean.hpp>
#include <Tsukino/Renderer/LightingPass.hpp>

#include <d3d11.h>
#include <wrl/client.h>

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {
    //------------------------------------------------------------------------
    //! LightingPass の GPU の資源です。
    //------------------------------------------------------------------------
    struct LightingPass::Gpu {
        Microsoft::WRL::ComPtr<ID3D11PixelShader> ps;                        // ライティング用ピクセルシェーダー
        Microsoft::WRL::ComPtr<ID3D11Buffer>      lightsBuffer;              // 点光源・スポットライト配列用定数バッファ (b6)
    };
}    // namespace Tsukino::Renderer
