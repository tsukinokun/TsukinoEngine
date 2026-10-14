//------------------------------------------------------------
//! @file   DX11Texture2D.cpp
//! @brief  DirectX11用の2Dテクスチャクラスの実装
//! @author 山﨑愛
//------------------------------------------------------------
#define NOMINMAX
#include <Tsukino/Renderer/DX11/Texture/DX11Texture2D.hpp>
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <vector>

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {
    namespace {
        //------------------------------------------------------------
        //! @brief  2x2平均でミップマップを作れるフォーマットか（非圧縮の1画素4バイト）
        //------------------------------------------------------------
        bool CanGenerateMips(DXGI_FORMAT format) {
            switch(format) {
                case DXGI_FORMAT_R8G8B8A8_UNORM:
                case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
                case DXGI_FORMAT_B8G8R8A8_UNORM:
                case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
                    return true;
                default:
                    return false;
            }
        }

        //------------------------------------------------------------
        //! @brief  1段上の絵を2x2平均で半分に縮める
        //! @note   奇数の辺は端の画素を繰り返して読む（1x1まで縮められるように）。
        //!         sRGBフォーマットでも値のまま平均する（線形化はしない）。
        //!         わずかに暗く縮むが、遠景のちらつきを抑える目的には十分
        //------------------------------------------------------------
        std::vector<std::uint8_t> Downsample(const std::uint8_t* src, u32 srcW, u32 srcH, u32 dstW, u32 dstH) {
            std::vector<std::uint8_t> dst(static_cast<size_t>(dstW) * dstH * 4);
            for(u32 y = 0; y < dstH; ++y) {
                const u32 y0 = std::min(y * 2, srcH - 1);
                const u32 y1 = std::min(y * 2 + 1, srcH - 1);
                for(u32 x = 0; x < dstW; ++x) {
                    const u32 x0 = std::min(x * 2, srcW - 1);
                    const u32 x1 = std::min(x * 2 + 1, srcW - 1);
                    for(u32 c = 0; c < 4; ++c) {
                        const u32 sum = src[(static_cast<size_t>(y0) * srcW + x0) * 4 + c] + src[(static_cast<size_t>(y0) * srcW + x1) * 4 + c]
                                        + src[(static_cast<size_t>(y1) * srcW + x0) * 4 + c] + src[(static_cast<size_t>(y1) * srcW + x1) * 4 + c];
                        dst[(static_cast<size_t>(y) * dstW + x) * 4 + c] = static_cast<std::uint8_t>((sum + 2) / 4);
                    }
                }
            }
            return dst;
        }
    }    // namespace

    //------------------------------------------------------------
    //! @brief コンストラクタ
    //------------------------------------------------------------
    DX11Texture2D::DX11Texture2D(u32 width, u32 height, DXGI_FORMAT format, const void* data, ID3D11Device* device, bool generateMips)
        : m_width(width)
        , m_height(height) {
        //------------------------------------------------------------
        // ミップマップの段を作る（要求があり、フォーマットが対応しているときだけ）。
        // 最上段は渡されたdataをそのまま使い、2段目以降をここで縮めて持つ
        //------------------------------------------------------------
        const bool                             buildMips = generateMips && data && CanGenerateMips(format) && width > 0 && height > 0;
        std::vector<std::vector<std::uint8_t>> mipPixels;    // 2段目以降の画素（[0]が2段目）
        std::vector<D3D11_SUBRESOURCE_DATA>    subresources;

        // BC3などブロック圧縮フォーマットの場合はPitchの計算が異なる
        D3D11_SUBRESOURCE_DATA top = {};
        top.pSysMem                = data;
        if(format == DXGI_FORMAT_BC1_UNORM || format == DXGI_FORMAT_BC1_UNORM_SRGB) {
            top.SysMemPitch = std::max(1u, (width + 3) / 4) * 8;    // BC1は8バイト/ブロック
        } else if(format == DXGI_FORMAT_BC3_UNORM || format == DXGI_FORMAT_BC3_UNORM_SRGB || format == DXGI_FORMAT_BC7_UNORM
                  || format == DXGI_FORMAT_BC7_UNORM_SRGB) {
            top.SysMemPitch = std::max(1u, (width + 3) / 4) * 16;    // BC3/BC7は16バイト/ブロック
        } else {
            top.SysMemPitch = width * 4;    // RGBA8など非圧縮
        }
        subresources.push_back(top);

        if(buildMips) {
            const std::uint8_t* prev  = static_cast<const std::uint8_t*>(data);
            u32                 prevW = width;
            u32                 prevH = height;
            while(prevW > 1 || prevH > 1) {
                const u32 w = std::max(1u, prevW / 2);
                const u32 h = std::max(1u, prevH / 2);
                mipPixels.push_back(Downsample(prev, prevW, prevH, w, h));
                prev  = mipPixels.back().data();
                prevW = w;
                prevH = h;
            }
            // vectorの再確保で先頭ポインタが動くので、全段を作り終えてから指す
            for(size_t i = 0; i < mipPixels.size(); ++i) {
                D3D11_SUBRESOURCE_DATA level = {};
                level.pSysMem                = mipPixels[i].data();
                level.SysMemPitch            = std::max(1u, width >> (i + 1)) * 4;
                subresources.push_back(level);
            }
        }
        const UINT mipLevels = static_cast<UINT>(subresources.size());

        // テクスチャ設定
        D3D11_TEXTURE2D_DESC desc = {};
        desc.Width                = width;
        desc.Height               = height;
        desc.MipLevels            = mipLevels;
        desc.ArraySize            = 1;
        desc.Format               = format;
        desc.SampleDesc.Count     = 1;
        desc.Usage                = D3D11_USAGE_DEFAULT;
        desc.BindFlags            = D3D11_BIND_SHADER_RESOURCE;

        // テクスチャ作成
        device->CreateTexture2D(&desc, subresources.data(), m_texture.GetAddressOf());

        // SRV設定
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format                          = format;
        srvDesc.ViewDimension                   = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels             = mipLevels;

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
