//----------------------------------------------------------------------------
//! @file   SphereColorGradePassGpu.hpp
//! @brief  SphereColorGradePass の DX11 の中身（Renderer の内部用）
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/Core/WindowsLean.hpp>
#include <Tsukino/Renderer/SphereColorGradePass.hpp>

#include <d3d11.h>
#include <wrl/client.h>

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {
    //------------------------------------------------------------------------
    //! SphereColorGradePass の GPU の資源です。
    //------------------------------------------------------------------------
    struct SphereColorGradePass::Gpu {
        Microsoft::WRL::ComPtr<ID3D11PixelShader> ps;                 // 色調補正用ピクセルシェーダー
        Microsoft::WRL::ComPtr<ID3D11Buffer>      buffer;             // パラメータ用バッファ (b8)
        Microsoft::WRL::ComPtr<ID3D11BlendState>  blendState;         // dest * src1 + src0 のデュアルソースブレンド
    };
}    // namespace Tsukino::Renderer
