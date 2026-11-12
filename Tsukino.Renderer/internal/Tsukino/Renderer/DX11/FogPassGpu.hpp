//----------------------------------------------------------------------------
//! @file   FogPassGpu.hpp
//! @brief  FogPass の DX11 の中身（Renderer の内部用）
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/Core/WindowsLean.hpp>
#include <Tsukino/Renderer/FogPass.hpp>

#include <d3d11.h>
#include <wrl/client.h>

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {
    //------------------------------------------------------------------------
    //! FogPass の GPU の資源です。
    //------------------------------------------------------------------------
    struct FogPass::Gpu {
        Microsoft::WRL::ComPtr<ID3D11PixelShader> ps;                 // フォグ用ピクセルシェーダー
        Microsoft::WRL::ComPtr<ID3D11Buffer>      buffer;             // フォグパラメータ用バッファ (b8)
        Microsoft::WRL::ComPtr<ID3D11Buffer>      forwardBuffer;      // 半透明モデル用（distanceParams.w=1）のバッファ (b8)
    };
}    // namespace Tsukino::Renderer
