//----------------------------------------------------------------------------
//! @file   BloomPassGpu.hpp
//! @brief  BloomPass の DX11 の中身（Renderer の内部用）
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/Core/WindowsLean.hpp>
#include <Tsukino/Renderer/BloomPass.hpp>

#include <d3d11.h>
#include <wrl/client.h>

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {
    //------------------------------------------------------------------------
    //! BloomPass の GPU の資源です。
    //------------------------------------------------------------------------
    struct BloomPass::Gpu {
        //! 縮小段1枚分のテクスチャ
        struct MipTarget {
            Microsoft::WRL::ComPtr<ID3D11Texture2D>          texture;
            Microsoft::WRL::ComPtr<ID3D11RenderTargetView>   rtv;
            Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv;
            u32                                              width  = 0;
            u32                                              height = 0;
        };

        Microsoft::WRL::ComPtr<ID3D11PixelShader> prefilterPS;     // 明部抽出 + 最初の縮小
        Microsoft::WRL::ComPtr<ID3D11PixelShader> downsamplePS;    // 縮小
        Microsoft::WRL::ComPtr<ID3D11PixelShader> upsamplePS;      // 拡大と合成
        Microsoft::WRL::ComPtr<ID3D11Buffer>      buffer;          // パラメータ用バッファ（CBSlot::Bloom）
        std::array<MipTarget, kMaxMipCount> mips{};             // 縮小段（[0] が 1/2）
    };
}    // namespace Tsukino::Renderer
