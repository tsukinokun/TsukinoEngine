//------------------------------------------------------------
//! @file   DX11Texture2D.cpp
//! @brief  DirectX11用の2Dテクスチャクラスの実装
//! @author 山﨑愛
//------------------------------------------------------------
#define NOMINMAX
#include <Tsukino/Renderer/DX11/Texture/DX11Texture2D.hpp>
#include <algorithm>

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {

    //------------------------------------------------------------
    //! @brief コンストラクタ
    //------------------------------------------------------------
    DX11Texture2D::DX11Texture2D(u32 width, u32 height, DXGI_FORMAT format, const void* data, ID3D11Device* device)
        : m_width(width)
        , m_height(height) {
        // テクスチャ設定
        D3D11_TEXTURE2D_DESC desc = {};
        desc.Width                = width;
        desc.Height               = height;
        desc.MipLevels            = 1;
        desc.ArraySize            = 1;
        desc.Format               = format;
        desc.SampleDesc.Count     = 1;
        desc.Usage                = D3D11_USAGE_DEFAULT;
        desc.BindFlags            = D3D11_BIND_SHADER_RESOURCE;

        // 初期データ
        D3D11_SUBRESOURCE_DATA initData = {};
        initData.pSysMem                = data;

        // BC3などブロック圧縮フォーマットの場合はPitchの計算が異なる
        if(format == DXGI_FORMAT_BC1_UNORM || format == DXGI_FORMAT_BC1_UNORM_SRGB) {
            initData.SysMemPitch = std::max(1u, (width + 3) / 4) * 8;    // BC1は8バイト/ブロック
        } else if(format == DXGI_FORMAT_BC3_UNORM || format == DXGI_FORMAT_BC3_UNORM_SRGB || format == DXGI_FORMAT_BC7_UNORM
                  || format == DXGI_FORMAT_BC7_UNORM_SRGB) {
            initData.SysMemPitch = std::max(1u, (width + 3) / 4) * 16;    // BC3/BC7は16バイト/ブロック
        } else {
            initData.SysMemPitch = width * 4;    // RGBA8など非圧縮
        }

        // テクスチャ作成
        device->CreateTexture2D(&desc, &initData, m_texture.GetAddressOf());

        // SRV設定
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format                          = format;
        srvDesc.ViewDimension                   = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels             = 1;

        // SRV作成
        device->CreateShaderResourceView(m_texture.Get(), &srvDesc, m_srv.GetAddressOf());
    }

    //------------------------------------------------------------
    //! @brief バインド
    //------------------------------------------------------------
    void DX11Texture2D::Bind(ID3D11DeviceContext* context, u32 slot) const {
        context->PSSetShaderResources(slot, 1, m_srv.GetAddressOf());
    }

}    // namespace Tsukino::Renderer
