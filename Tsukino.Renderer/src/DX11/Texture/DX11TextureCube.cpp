//------------------------------------------------------------
//! @file   DX11TextureCube.cpp
//! @brief  DirectX11用のキューブマップレンダーターゲットクラスの実装
//! @author 山﨑愛
//------------------------------------------------------------
#include <Tsukino/Renderer/DX11/Texture/DX11TextureCube.hpp>
#include <Tsukino/Core/Log.hpp>

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {

    //------------------------------------------------------------
    //! @brief コンストラクタ
    //------------------------------------------------------------
    DX11TextureCube::DX11TextureCube(u32 baseSize, u32 mipLevels, DXGI_FORMAT format, ID3D11Device* device)
        : m_baseSize(baseSize)
        , m_mipLevels(mipLevels < 1 ? 1 : mipLevels) {
        //------------------------------------------------------------
        // キューブマップ本体（ArraySize=6, MiscFlags=TEXTURECUBE）
        // RENDER_TARGETとSHADER_RESOURCEの両方を立てる：面・mipごとに描画し、
        // 描き終わったら1枚のTextureCubeとしてシェーダーから読むため
        //------------------------------------------------------------
        D3D11_TEXTURE2D_DESC desc = {};
        desc.Width                = baseSize;
        desc.Height               = baseSize;
        desc.MipLevels            = m_mipLevels;
        desc.ArraySize            = 6;
        desc.Format               = format;
        desc.SampleDesc.Count     = 1;
        desc.Usage                = D3D11_USAGE_DEFAULT;
        desc.BindFlags            = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
        desc.MiscFlags            = D3D11_RESOURCE_MISC_TEXTURECUBE;

        HRESULT hr = device->CreateTexture2D(&desc, nullptr, m_texture.GetAddressOf());
        if(FAILED(hr)) {
            Tsukino::Core::Log::Error("DX11TextureCube: failed to create cube texture.");
            return;
        }

        //------------------------------------------------------------
        // シェーダー読み取り用SRV（1枚のTextureCubeとして全mip・全面をまとめて見る）
        //------------------------------------------------------------
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format                          = format;
        srvDesc.ViewDimension                   = D3D11_SRV_DIMENSION_TEXTURECUBE;
        srvDesc.TextureCube.MipLevels            = m_mipLevels;
        srvDesc.TextureCube.MostDetailedMip      = 0;

        hr = device->CreateShaderResourceView(m_texture.Get(), &srvDesc, m_srv.GetAddressOf());
        if(FAILED(hr)) {
            Tsukino::Core::Log::Error("DX11TextureCube: failed to create cube SRV.");
            m_srv.Reset();
            return;
        }

        //------------------------------------------------------------
        // 面×mipごとのRTV。TEXTURE2DARRAYビューでFirstArraySlice=面を指定することで、
        // キューブの1面だけを通常の2Dレンダーターゲットのように扱える
        //------------------------------------------------------------
        m_faceRTV.resize(static_cast<size_t>(m_mipLevels) * 6);

        for(u32 mip = 0; mip < m_mipLevels; ++mip) {
            for(u32 face = 0; face < 6; ++face) {
                D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
                rtvDesc.Format                        = format;
                rtvDesc.ViewDimension                 = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
                rtvDesc.Texture2DArray.MipSlice        = mip;
                rtvDesc.Texture2DArray.FirstArraySlice = face;
                rtvDesc.Texture2DArray.ArraySize       = 1;

                const size_t index = static_cast<size_t>(mip) * 6 + face;
                hr = device->CreateRenderTargetView(m_texture.Get(), &rtvDesc, m_faceRTV[index].GetAddressOf());
                if(FAILED(hr)) {
                    Tsukino::Core::Log::Error("DX11TextureCube: failed to create face RTV.");
                    return;
                }
            }
        }
    }

}    // namespace Tsukino::Renderer
