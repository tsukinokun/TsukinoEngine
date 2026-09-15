//----------------------------------------------------------------------------
//! @file   RenderResources.cpp
//! @brief  描画で共有する資源の実装
//----------------------------------------------------------------------------
#include <Tsukino/Renderer/RenderResources.hpp>

#include <Tsukino/Engine/Asset/Texture/TextureAsset.hpp>

#include <Tsukino/GraphicsCommon/Mesh/MeshData.hpp>
#include <Tsukino/GraphicsCommon/Mesh/MeshPrimitives.hpp>

#include <Tsukino/Core/Log.hpp>

#include <string>

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {
    //------------------------------------------------------------------------
    //! 資源を作成します。
    //------------------------------------------------------------------------
    bool RenderResources::Initialize(ID3D11Device* device, ID3D11DeviceContext* context) {
        m_device  = device;
        m_context = context;

        // デバイスが準備できたので、Factory を構築して optional に代入（遅延DI）
        m_pipelineFactory.emplace(device);

        // DirectXTK の共通ステート
        m_commonStates = std::make_unique<DirectX::CommonStates>(device);

        CreatePrimitiveMeshes();

        if(!CreateSamplers())
            return false;

        if(!CreateDefaultTextures())
            return false;

        return true;
    }

    //------------------------------------------------------------------------
    //! テクスチャの SRV を取得します（初回はGPUへ転送してキャッシュします）。
    //------------------------------------------------------------------------
    ID3D11ShaderResourceView* RenderResources::GetTextureSRV(const Tsukino::Asset::TextureAsset& textureAsset) {
        u64 handleValue = textureAsset.GetHandle().Value();

        // すでにキャッシュにあればそれを返す
        auto it = m_textureCache.find(handleValue);
        if(it != m_textureCache.end()) {
            return it->second->GetSRV();
        }

        // なければ新しく作成してキャッシュへ保存
        std::unique_ptr<DX11Texture2D> texture =
            std::make_unique<DX11Texture2D>(textureAsset.width, textureAsset.height, textureAsset.format, textureAsset.pixels.data(), m_device);

        ID3D11ShaderResourceView* srv = texture->GetSRV();
        m_textureCache.emplace(handleValue, std::move(texture));

        return srv;
    }

    //------------------------------------------------------------------------
    //! スプライトフォントを作成します。
    //------------------------------------------------------------------------
    std::unique_ptr<DirectX::SpriteFont> RenderResources::CreateSpriteFont(const u8* data, size_t size) const {
        return std::make_unique<DirectX::SpriteFont>(m_device, data, size);
    }

    //------------------------------------------------------------------------
    //! スプライトバッチを作成します。
    //------------------------------------------------------------------------
    std::unique_ptr<DirectX::SpriteBatch> RenderResources::CreateSpriteBatch() const {
        return std::make_unique<DirectX::SpriteBatch>(m_context);
    }

    //------------------------------------------------------------------------
    //! サンプラーを作成します。
    //------------------------------------------------------------------------
    bool RenderResources::CreateSamplers() {
        using Tsukino::GraphicsCommon::SamplerType;

        D3D11_SAMPLER_DESC desc{};

        // 共通設定
        desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
        desc.MinLOD         = 0;
        desc.MaxLOD         = D3D11_FLOAT32_MAX;

        // 指定した種類のサンプラーを今の desc で作る処理
        auto create = [&](SamplerType type) {
            return SUCCEEDED(m_device->CreateSamplerState(&desc, m_samplers[static_cast<size_t>(type)].GetAddressOf()));
        };

        // --- PointWrap / PointClamp ---
        desc.Filter   = D3D11_FILTER_MIN_MAG_MIP_POINT;
        desc.AddressU = desc.AddressV = desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
        if(!create(SamplerType::PointWrap))
            return false;

        desc.AddressU = desc.AddressV = desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        if(!create(SamplerType::PointClamp))
            return false;

        // --- LinearWrap / LinearClamp ---
        desc.Filter   = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        desc.AddressU = desc.AddressV = desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
        if(!create(SamplerType::LinearWrap))
            return false;

        desc.AddressU = desc.AddressV = desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        if(!create(SamplerType::LinearClamp))
            return false;

        // --- AnisotropicWrap / AnisotropicClamp ---
        desc.Filter        = D3D11_FILTER_ANISOTROPIC;
        desc.MaxAnisotropy = 16;
        desc.AddressU = desc.AddressV = desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
        if(!create(SamplerType::AnisotropicWrap))
            return false;

        desc.AddressU = desc.AddressV = desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        if(!create(SamplerType::AnisotropicClamp))
            return false;

        return true;
    }

    //------------------------------------------------------------------------
    //! プリミティブメッシュを作成します。
    //------------------------------------------------------------------------
    void RenderResources::CreatePrimitiveMeshes() {
        using namespace Tsukino::GraphicsCommon;

        for(size_t i = 0; i < static_cast<size_t>(PrimitiveType::Count); ++i) {
            // CPU 側で形状を作り、GPU へ転送
            MeshData meshData    = CreatePrimitiveMeshData(static_cast<PrimitiveType>(i));
            m_primitiveMeshes[i] = CreateMeshBuffer(m_device, meshData);
        }
    }

    //------------------------------------------------------------------------
    //! マテリアル用の既定テクスチャ（白・フラット法線）を作成します。
    //------------------------------------------------------------------------
    bool RenderResources::CreateDefaultTextures() {
        // 白 (1,1,1,1)：アルベド/MR/エミッシブ/AOの未設定時。乗算で恒等元になる
        if(!Create1x1Texture(0xFFFFFFFF, m_whiteTex, m_whiteSRV, "white"))
            return false;

        // フラット法線：接空間の(0,0,1) → R=0x80, G=0x80, B=0xFF, A=0xFF
        if(!Create1x1Texture(0xFFFF8080, m_flatNormalTex, m_flatNormalSRV, "flat normal"))
            return false;

        return true;
    }

    //------------------------------------------------------------------------
    //! 1x1 のテクスチャを作成します。
    //------------------------------------------------------------------------
    bool RenderResources::Create1x1Texture(u32                                               rgba,
                                           Microsoft::WRL::ComPtr<ID3D11Texture2D>&          outTex,
                                           Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& outSRV,
                                           const char*                                       debugName) {
        D3D11_TEXTURE2D_DESC desc{};
        desc.Width            = 1;
        desc.Height           = 1;
        desc.MipLevels        = 1;
        desc.ArraySize        = 1;
        desc.Format           = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.Usage            = D3D11_USAGE_IMMUTABLE;
        desc.BindFlags        = D3D11_BIND_SHADER_RESOURCE;

        D3D11_SUBRESOURCE_DATA initData{&rgba, 4, 0};

        if(FAILED(m_device->CreateTexture2D(&desc, &initData, outTex.GetAddressOf()))) {
            Tsukino::Core::Log::Error(std::string("Failed to create default texture: ") + debugName + ".");
            return false;
        }

        if(FAILED(m_device->CreateShaderResourceView(outTex.Get(), nullptr, outSRV.GetAddressOf()))) {
            Tsukino::Core::Log::Error(std::string("Failed to create default texture SRV: ") + debugName + ".");
            return false;
        }

        return true;
    }
}    // namespace Tsukino::Renderer
