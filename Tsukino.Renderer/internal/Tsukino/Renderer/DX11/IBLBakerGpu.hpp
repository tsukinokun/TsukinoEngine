//----------------------------------------------------------------------------
//! @file   IBLBakerGpu.hpp
//! @brief  IBLBaker の DX11 の中身（Renderer の内部用）
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/Core/WindowsLean.hpp>
#include <Tsukino/Renderer/IBLBaker.hpp>
#include <Tsukino/Renderer/DX11/Texture/DX11TextureCube.hpp>

#include <d3d11.h>
#include <wrl/client.h>

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {
    //------------------------------------------------------------------------
    //! IBLBaker の GPU の資源です。
    //------------------------------------------------------------------------
    struct IBLBaker::Gpu {
        std::unique_ptr<DX11TextureCube> captureCube;                // スカイを焼いたキャプチャキューブ（一発ベイクの中間結果）
        std::unique_ptr<DX11TextureCube> irradianceCube;             // 拡散IBL用 irradiance キューブ（t17 で読む）
        std::unique_ptr<DX11TextureCube> prefilteredSpecularCube;    // 鏡面IBL用プレフィルタ済みキューブ（t18 で読む）
        Microsoft::WRL::ComPtr<ID3D11Texture2D>          brdfLUTTex;    // split-sum 用 BRDF 積分 LUT 本体
        Microsoft::WRL::ComPtr<ID3D11RenderTargetView>   brdfLUTRTV;    // 生成時（起動時1回だけ）に使う RTV
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> brdfLUTSRV;    // シェーダー読み取り用 SRV（t19）
        Microsoft::WRL::ComPtr<ID3D11PixelShader> irradiancePS;           // irradiance 畳み込み用 PS（VS はフルスクリーン三角形用を共用）
        Microsoft::WRL::ComPtr<ID3D11PixelShader> specularPrefilterPS;    // スペキュラプレフィルタ用 PS（同上）
        Microsoft::WRL::ComPtr<ID3D11PixelShader> brdfLUTPS;              // BRDF LUT 生成用 PS（同上）
        Microsoft::WRL::ComPtr<ID3D11Buffer> buffer;        // CBufferIBL 用バッファ (b10)
        Microsoft::WRL::ComPtr<ID3D11Buffer> bakeBuffer;    // CBufferIBLBake 用バッファ (b11, ベイク中だけ使う一時バッファ)
    };
}    // namespace Tsukino::Renderer
