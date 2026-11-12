//----------------------------------------------------------------------------
//! @file   MotionBlurPassGpu.hpp
//! @brief  MotionBlurPass の DX11 の中身（Renderer の内部用）
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/Core/WindowsLean.hpp>
#include <Tsukino/Renderer/MotionBlurPass.hpp>

#include <d3d11.h>
#include <wrl/client.h>

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {
    //------------------------------------------------------------------------
    //! MotionBlurPass の GPU の資源です。
    //------------------------------------------------------------------------
    struct MotionBlurPass::Gpu {
        Microsoft::WRL::ComPtr<ID3D11PixelShader> ps;                 // モーションブラー用ピクセルシェーダー
        Microsoft::WRL::ComPtr<ID3D11Buffer>      buffer;             // モーションブラーパラメータ用バッファ (b8)
    };
}    // namespace Tsukino::Renderer
