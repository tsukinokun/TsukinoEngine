//----------------------------------------------------------------------------
//! @file   SkyPassGpu.hpp
//! @brief  SkyPass の DX11 の中身（Renderer の内部用）
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/Core/WindowsLean.hpp>
#include <Tsukino/Renderer/SkyPass.hpp>

#include <d3d11.h>
#include <wrl/client.h>

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {
    //------------------------------------------------------------------------
    //! SkyPass の GPU の資源です。
    //------------------------------------------------------------------------
    struct SkyPass::Gpu {
        //! 頂点シェーダーを取得します（IBL のキャプチャが借りる）。
        //! @return 頂点シェーダー
        [[nodiscard]]
        ID3D11VertexShader* GetVertexShader() const noexcept {
            return vs.Get();
        }

        //! ピクセルシェーダーを取得します（同上）。
        //! @return ピクセルシェーダー
        [[nodiscard]]
        ID3D11PixelShader* GetPixelShader() const noexcept {
            return ps.Get();
        }

        //! 定数バッファのアドレスを取得します（同上）。
        //! @return 定数バッファへのポインタのアドレス
        [[nodiscard]]
        ID3D11Buffer* const* GetBufferAddress() const noexcept {
            return buffer.GetAddressOf();
        }

        Microsoft::WRL::ComPtr<ID3D11VertexShader> vs;        // スカイ用頂点シェーダー
        Microsoft::WRL::ComPtr<ID3D11PixelShader>  ps;        // スカイ用ピクセルシェーダー
        Microsoft::WRL::ComPtr<ID3D11Buffer>       buffer;    // スカイ定数バッファ (b4)
    };
}    // namespace Tsukino::Renderer
