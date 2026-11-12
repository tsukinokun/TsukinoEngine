//----------------------------------------------------------------------------
//! @file   FrameConstantsGpu.hpp
//! @brief  FrameConstants の DX11 の中身（Renderer の内部用）
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/Core/WindowsLean.hpp>
#include <Tsukino/Renderer/FrameConstants.hpp>

#include <d3d11.h>
#include <wrl/client.h>

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {
    //------------------------------------------------------------------------
    //! FrameConstants の GPU の資源です。
    //------------------------------------------------------------------------
    struct FrameConstants::Gpu {
        //! b0 の定数バッファを取得します。
        //! @return 定数バッファ
        [[nodiscard]]
        ID3D11Buffer* GetSceneBuffer() const noexcept {
            return sceneBuffer.Get();
        }

        //! b0 の定数バッファのアドレスを取得します（〜SetConstantBuffers に渡す形）。
        //! @return 定数バッファへのポインタのアドレス
        [[nodiscard]]
        ID3D11Buffer* const* GetSceneBufferAddress() const noexcept {
            return sceneBuffer.GetAddressOf();
        }

        Microsoft::WRL::ComPtr<ID3D11Buffer> sceneBuffer;                  // b0 の定数バッファ
    };
}    // namespace Tsukino::Renderer
