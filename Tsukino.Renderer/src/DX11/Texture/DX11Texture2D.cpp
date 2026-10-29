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
    namespace {
        //------------------------------------------------------------
        //! @brief  1行のバイト数（ブロック圧縮フォーマットはブロック単位で数える）
        //------------------------------------------------------------
        UINT RowPitch(DXGI_FORMAT format, u32 width) {
            switch(format) {
                case DXGI_FORMAT_BC1_UNORM:
                case DXGI_FORMAT_BC1_UNORM_SRGB:
                    return std::max(1u, (width + 3) / 4) * 8;    // BC1は8バイト/ブロック
                case DXGI_FORMAT_BC3_UNORM:
                case DXGI_FORMAT_BC3_UNORM_SRGB:
                case DXGI_FORMAT_BC7_UNORM:
                case DXGI_FORMAT_BC7_UNORM_SRGB:
                    return std::max(1u, (width + 3) / 4) * 16;    // BC3/BC7は16バイト/ブロック
                default:
                    return width * 4;    // RGBA8など非圧縮
            }
        }

        //------------------------------------------------------------
        //! @brief  GPUにミップマップを作らせられるフォーマットか
        //------------------------------------------------------------
        bool CanGenerateMips(ID3D11Device* device, DXGI_FORMAT format) {
            UINT support = 0;
            if(FAILED(device->CheckFormatSupport(format, &support)))
                return false;
            return (support & D3D11_FORMAT_SUPPORT_MIP_AUTOGEN) && (support & D3D11_FORMAT_SUPPORT_RENDER_TARGET);
        }
    }    // namespace

    //------------------------------------------------------------
    //! @brief コンストラクタ
    //------------------------------------------------------------
    DX11Texture2D::DX11Texture2D(u32 width, u32 height, DXGI_FORMAT format, const void* data, ID3D11Device* device, bool generateMips)
        : m_width(width)
        , m_height(height) {
        //------------------------------------------------------------
        // ミップマップはGPUに作らせる（要求があり、フォーマットが対応しているときだけ）。
        // CPUで縮めると、カードを配るときなど一度に何枚も転送するフレームが重くなる。
        // GPUはsRGBフォーマットの色を線形にしてから縮めるので、縮んでも暗くならない
        //------------------------------------------------------------
        const bool buildMips = generateMips && data && width > 0 && height > 0 && CanGenerateMips(device, format);

        // テクスチャ設定
        D3D11_TEXTURE2D_DESC desc = {};
        desc.Width                = width;
        desc.Height               = height;
        desc.MipLevels            = buildMips ? 0 : 1;    // 0は1x1までの全段
        desc.ArraySize            = 1;
        desc.Format               = format;
        desc.SampleDesc.Count     = 1;
        desc.Usage                = D3D11_USAGE_DEFAULT;
        desc.BindFlags            = D3D11_BIND_SHADER_RESOURCE | (buildMips ? D3D11_BIND_RENDER_TARGET : 0);
        desc.MiscFlags            = buildMips ? D3D11_RESOURCE_MISC_GENERATE_MIPS : 0;

        // テクスチャ作成（ミップを作るときは、全段の初期データを渡せないので最上段をあとから書く）
        D3D11_SUBRESOURCE_DATA top = {};
        top.pSysMem                = data;
        top.SysMemPitch            = RowPitch(format, width);
        if(FAILED(device->CreateTexture2D(&desc, buildMips || !data ? nullptr : &top, m_texture.GetAddressOf())))
            return;

        // SRV設定
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format                          = format;
        srvDesc.ViewDimension                   = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels             = static_cast<UINT>(-1);    // テクスチャの全段

        // SRV作成
        device->CreateShaderResourceView(m_texture.Get(), &srvDesc, m_srv.GetAddressOf());

        // 最上段を書き、残りの段をGPUで縮めて作る
        if(buildMips && m_srv) {
            Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;
            device->GetImmediateContext(context.GetAddressOf());
            context->UpdateSubresource(m_texture.Get(), 0, nullptr, data, top.SysMemPitch, 0);
            context->GenerateMips(m_srv.Get());
        }
    }

    //------------------------------------------------------------
    //! @brief バインド
    //------------------------------------------------------------
    void DX11Texture2D::Bind(ID3D11DeviceContext* context, u32 slot) const {
        context->PSSetShaderResources(slot, 1, m_srv.GetAddressOf());
    }

}    // namespace Tsukino::Renderer
