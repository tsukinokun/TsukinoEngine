//----------------------------------------------------------------------------
//! @file   RenderResources.cpp
//! @brief  描画で共有する資源の実装
//----------------------------------------------------------------------------
#include <Tsukino/Renderer/DX11/RenderResourcesGpu.hpp>
#include <Tsukino/Renderer/DX11/GraphicsContext.hpp>
#include <Tsukino/Renderer/DX11/MeshBufferGpu.hpp>

#include <Tsukino/Renderer/DX11/Texture/DX11Texture2D.hpp>

#include <Tsukino/Engine/Asset/Texture/TextureAsset.hpp>

#include <Tsukino/GraphicsCommon/Mesh/MeshData.hpp>
#include <Tsukino/GraphicsCommon/Mesh/MeshPrimitives.hpp>

#include <Tsukino/Core/Log.hpp>

#include "DX11/TextureFormatDxgi.hpp"

#include <algorithm>
#include <cstring>
#include <string>

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {
    //------------------------------------------------------------------------
    //! 資源を作成します。
    //------------------------------------------------------------------------
    bool RenderResources::Gpu::Initialize(const GraphicsContext& graphicsContext) {
        ID3D11Device* device = graphicsContext.GetDevice();
        m_device             = device;
        m_context            = graphicsContext.GetContext();

        // デバイスが準備できたので、Factory を構築して optional に代入（遅延DI）
        m_pipelineFactory.emplace(graphicsContext);

        // DirectXTK の共通ステート
        m_commonStates = std::make_unique<DirectX::CommonStates>(device);

        //--------------------------------------------------------------------
        // 切り取り用のラスタライザ。CommonStates::CullNone と同じ設定にシザーだけを足す
        // （UI は裏返しで描かれることもあるのでカリングしない）
        //--------------------------------------------------------------------
        D3D11_RASTERIZER_DESC scissorDesc = {};
        scissorDesc.FillMode              = D3D11_FILL_SOLID;
        scissorDesc.CullMode              = D3D11_CULL_NONE;
        scissorDesc.DepthClipEnable       = TRUE;
        scissorDesc.MultisampleEnable     = TRUE;
        scissorDesc.ScissorEnable         = TRUE;
        if(FAILED(device->CreateRasterizerState(&scissorDesc, m_scissorRasterizer.GetAddressOf()))) {
            Tsukino::Core::Log::Error("Failed to create scissor rasterizer state.");
            return false;
        }

        CreatePrimitiveMeshes();

        if(!CreateSamplers())
            return false;

        if(!CreateDefaultTextures())
            return false;

        return true;
    }

    //------------------------------------------------------------------------
    //! テクスチャアセットのハンドルを取得します（初回はGPUへ転送してキャッシュします）。
    //------------------------------------------------------------------------
    TextureHandle RenderResources::Gpu::GetTexture(Tsukino::Asset::TextureAsset& textureAsset) {
        u64 handleValue = textureAsset.GetHandle().Value();

        // すでにキャッシュにあればそれを返す
        auto it = m_textureCache.find(handleValue);
        if(it != m_textureCache.end()) {
            return it->second;
        }

        // なければ新しく作成してキャッシュへ保存
        std::unique_ptr<DX11Texture2D> texture;
        const DXGI_FORMAT              format = Detail::ToDxgiFormat(textureAsset.format);
        if(textureAsset.mips.size() > 1) {
            // ファイルが持っている段をそのまま全部送る（圧縮済みの絵はGPUで縮められないため）
            std::vector<D3D11_SUBRESOURCE_DATA> mips(textureAsset.mips.size());
            for(size_t i = 0; i < mips.size(); ++i) {
                mips[i].pSysMem     = textureAsset.pixels.data() + textureAsset.mips[i].offset;
                mips[i].SysMemPitch = textureAsset.mips[i].rowPitch;
            }
            texture = std::make_unique<DX11Texture2D>(textureAsset.width, textureAsset.height, format, mips.data(),
                                                      static_cast<u32>(mips.size()), m_device);
        }
        else {
            texture = std::make_unique<DX11Texture2D>(textureAsset.width, textureAsset.height, format,
                                                      textureAsset.pixels.data(), m_device, textureAsset.generateMips);
        }

        // 表がビューの参照を持つので、テクスチャ本体は DX11Texture2D を手放しても残る
        const TextureHandle handle = AddTextureSlot(texture->GetSRV());
        if(!handle.IsValid())
            return handle;
        m_textureCache.emplace(handleValue, handle);

        // GPU へ送り終えたので、CPU 側の画素は手放す（以降はキャッシュのテクスチャだけを使う）
        textureAsset.ReleasePixels();

        return handle;
    }

    //------------------------------------------------------------------------
    //! 書き換えられる 2D テクスチャ（ミップマップ無し）を作成します。
    //------------------------------------------------------------------------
    TextureHandle RenderResources::Gpu::CreateTexture(u32 width, u32 height, Tsukino::GraphicsCommon::TextureFormat format, const void* data, u32 rowPitch) {
        const DXGI_FORMAT dxgiFormat = Detail::ToDxgiFormat(format);
        if(dxgiFormat == DXGI_FORMAT_UNKNOWN || Detail::BytesPerPixel(format) == 0 || width == 0 || height == 0) {
            Tsukino::Core::Log::Error("RenderResources: Invalid texture description.");
            return {};
        }

        D3D11_TEXTURE2D_DESC desc{};
        desc.Width            = width;
        desc.Height           = height;
        desc.MipLevels        = 1;
        desc.ArraySize        = 1;
        desc.Format           = dxgiFormat;
        desc.SampleDesc.Count = 1;
        desc.Usage            = D3D11_USAGE_DEFAULT;
        desc.BindFlags        = D3D11_BIND_SHADER_RESOURCE;

        //--------------------------------------------------------------------
        // 中身を渡されなかったときは 0 で埋める。初期データ無しで作ったテクスチャの内容は
        // D3D11 の仕様上未定義で、書き込んでいない所のゴミを縁のサンプリングで拾ってしまうため
        //--------------------------------------------------------------------
        std::vector<u8> zeros;
        if(!data) {
            rowPitch = width * Detail::BytesPerPixel(format);
            zeros.assign(static_cast<size_t>(rowPitch) * height, 0);
            data = zeros.data();
        }

        D3D11_SUBRESOURCE_DATA initData{};
        initData.pSysMem     = data;
        initData.SysMemPitch = rowPitch;

        Microsoft::WRL::ComPtr<ID3D11Texture2D> texture;
        if(FAILED(m_device->CreateTexture2D(&desc, &initData, texture.GetAddressOf()))) {
            Tsukino::Core::Log::Error("RenderResources: Failed to create texture.");
            return {};
        }

        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> view;
        if(FAILED(m_device->CreateShaderResourceView(texture.Get(), nullptr, view.GetAddressOf()))) {
            Tsukino::Core::Log::Error("RenderResources: Failed to create texture view.");
            return {};
        }

        return AddTextureSlot(std::move(view));
    }

    //------------------------------------------------------------------------
    //! CreateTexture() で作ったテクスチャの一部を書き換えます。
    //------------------------------------------------------------------------
    void RenderResources::Gpu::UpdateTexture(TextureHandle texture, u32 x, u32 y, u32 width, u32 height, const void* data, u32 rowPitch) {
        ID3D11ShaderResourceView* view = ResolveTexture(texture);
        if(!view || !data)
            return;

        Microsoft::WRL::ComPtr<ID3D11Resource> resource;
        view->GetResource(resource.GetAddressOf());

        D3D11_BOX box{};
        box.left   = x;
        box.top    = y;
        box.front  = 0;
        box.right  = x + width;
        box.bottom = y + height;
        box.back   = 1;

        m_context->UpdateSubresource(resource.Get(), 0, &box, data, rowPitch, 0);
    }

    //------------------------------------------------------------------------
    //! テクスチャを破棄します。
    //------------------------------------------------------------------------
    void RenderResources::Gpu::DestroyTexture(TextureHandle texture) {
        if(!texture.IsValid() || texture.index >= m_textures.size() || !m_textures[texture.index])
            return;

        m_textures[texture.index].Reset();
        m_freeTextures.push_back(texture.index);

        // アセットのキャッシュに残っていれば外す（次に描くときに作り直される）
        std::erase_if(m_textureCache, [texture](const auto& entry) { return entry.second == texture; });
    }

    //------------------------------------------------------------------------
    //! 外で作られたテクスチャのビューを、ハンドルで指せるように登録します。
    //------------------------------------------------------------------------
    TextureHandle RenderResources::Gpu::RegisterExternalTexture(ID3D11ShaderResourceView* view) {
        if(!view)
            return {};
        return AddTextureSlot(view);
    }

    //------------------------------------------------------------------------
    //! ハンドルが指すテクスチャのビューを取得します。
    //------------------------------------------------------------------------
    ID3D11ShaderResourceView* RenderResources::Gpu::ResolveTexture(TextureHandle texture) const {
        if(!texture.IsValid() || texture.index >= m_textures.size())
            return nullptr;
        return m_textures[texture.index].Get();
    }

    //------------------------------------------------------------------------
    //! インスタンス描画用のバッファを作成します。
    //------------------------------------------------------------------------
    BufferHandle RenderResources::Gpu::CreateInstanceBuffer(u32 stride, u32 capacity) {
        BufferSlot slot;
        slot.instance = Tsukino::Renderer::CreateInstanceBuffer(m_device, stride, capacity);
        if(!slot.instance.IsValid())
            return {};
        return AddBufferSlot(std::move(slot));
    }

    //------------------------------------------------------------------------
    //! インスタンス描画用のバッファの中身を書き換えます。
    //------------------------------------------------------------------------
    u32 RenderResources::Gpu::UpdateInstanceBuffer(BufferHandle buffer, const void* data, u32 count) {
        if(!FindBufferSlot(buffer) || !m_buffers[buffer.index].instance.IsValid())
            return 0;
        return Tsukino::Renderer::UpdateInstanceBuffer(m_context, m_buffers[buffer.index].instance, data, count);
    }

    //------------------------------------------------------------------------
    //! ゲーム定義の定数バッファを作成します。
    //------------------------------------------------------------------------
    BufferHandle RenderResources::Gpu::CreateUserConstantBuffer(u32 byteSize) {
        BufferSlot slot;
        slot.constant = Tsukino::Renderer::CreateUserConstantBuffer(m_device, byteSize);
        if(!slot.constant.IsValid())
            return {};
        return AddBufferSlot(std::move(slot));
    }

    //------------------------------------------------------------------------
    //! ゲーム定義の定数バッファの中身を書き換えます。
    //------------------------------------------------------------------------
    void RenderResources::Gpu::UpdateUserConstantBuffer(BufferHandle buffer, const void* data, u32 byteSize) {
        const BufferSlot* slot = FindBufferSlot(buffer);
        if(!slot || !slot->constant.IsValid())
            return;
        Tsukino::Renderer::UpdateUserConstantBuffer(m_context, slot->constant, data, byteSize);
    }

    //------------------------------------------------------------------------
    //! バッファを破棄します。
    //------------------------------------------------------------------------
    void RenderResources::Gpu::DestroyBuffer(BufferHandle buffer) {
        if(!FindBufferSlot(buffer))
            return;
        m_buffers[buffer.index] = BufferSlot{};
        m_freeBuffers.push_back(buffer.index);
    }

    //------------------------------------------------------------------------
    //! インスタンス描画用のバッファのビューを取得します。
    //------------------------------------------------------------------------
    ID3D11ShaderResourceView* RenderResources::Gpu::ResolveInstanceBuffer(BufferHandle buffer) const {
        const BufferSlot* slot = FindBufferSlot(buffer);
        return slot ? slot->instance.srv.Get() : nullptr;
    }

    //------------------------------------------------------------------------
    //! ゲーム定義の定数バッファを取得します。
    //------------------------------------------------------------------------
    ID3D11Buffer* RenderResources::Gpu::ResolveUserConstantBuffer(BufferHandle buffer) const {
        const BufferSlot* slot = FindBufferSlot(buffer);
        return slot ? slot->constant.buffer.Get() : nullptr;
    }

    //------------------------------------------------------------------------
    //! テクスチャのビューを表へ入れ、ハンドルを割り当てます。
    //------------------------------------------------------------------------
    TextureHandle RenderResources::Gpu::AddTextureSlot(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> view) {
        TextureHandle handle;
        if(!m_freeTextures.empty()) {
            handle.index = m_freeTextures.back();
            m_freeTextures.pop_back();
            m_textures[handle.index] = std::move(view);
            return handle;
        }

        // 最後の番号は「無効」を表すので使わない
        if(m_textures.size() >= TextureHandle::kInvalidIndex) {
            Tsukino::Core::Log::Error("RenderResources: Too many textures.");
            return handle;
        }

        handle.index = static_cast<u16>(m_textures.size());
        m_textures.push_back(std::move(view));
        return handle;
    }

    //------------------------------------------------------------------------
    //! バッファを表へ入れ、ハンドルを割り当てます。
    //------------------------------------------------------------------------
    BufferHandle RenderResources::Gpu::AddBufferSlot(BufferSlot slot) {
        BufferHandle handle;
        if(!m_freeBuffers.empty()) {
            handle.index = m_freeBuffers.back();
            m_freeBuffers.pop_back();
            m_buffers[handle.index] = std::move(slot);
            return handle;
        }

        // 最後の番号は「無効」を表すので使わない
        if(m_buffers.size() >= BufferHandle::kInvalidIndex) {
            Tsukino::Core::Log::Error("RenderResources: Too many buffers.");
            return handle;
        }

        handle.index = static_cast<u16>(m_buffers.size());
        m_buffers.push_back(std::move(slot));
        return handle;
    }

    //------------------------------------------------------------------------
    //! ハンドルが指すバッファの枠を取得します。
    //------------------------------------------------------------------------
    const RenderResources::Gpu::BufferSlot* RenderResources::Gpu::FindBufferSlot(BufferHandle buffer) const {
        if(!buffer.IsValid() || buffer.index >= m_buffers.size())
            return nullptr;
        const BufferSlot& slot = m_buffers[buffer.index];
        if(!slot.instance.IsValid() && !slot.constant.IsValid())
            return nullptr;
        return &slot;
    }

    //------------------------------------------------------------------------
    //! スプライトフォントを作成します。
    //------------------------------------------------------------------------
    std::unique_ptr<DirectX::SpriteFont> RenderResources::Gpu::CreateSpriteFont(const u8* data, size_t size) const {
        return std::make_unique<DirectX::SpriteFont>(m_device, data, size);
    }

    //------------------------------------------------------------------------
    //! スプライトバッチを作成します。
    //------------------------------------------------------------------------
    std::unique_ptr<DirectX::SpriteBatch> RenderResources::Gpu::CreateSpriteBatch() const {
        return std::make_unique<DirectX::SpriteBatch>(m_context);
    }

    //------------------------------------------------------------------------
    //! サンプラーを作成します。
    //------------------------------------------------------------------------
    bool RenderResources::Gpu::CreateSamplers() {
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
    void RenderResources::Gpu::CreatePrimitiveMeshes() {
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
    bool RenderResources::Gpu::CreateDefaultTextures() {
        // 白 (1,1,1,1)：アルベド/MR/エミッシブ/AOの未設定時。乗算で恒等元になる
        if(!Create1x1Texture(0xFFFFFFFF, m_whiteTexture, "white"))
            return false;

        // フラット法線：接空間の(0,0,1) → R=0x80, G=0x80, B=0xFF, A=0xFF
        if(!Create1x1Texture(0xFFFF8080, m_flatNormalTexture, "flat normal"))
            return false;

        return true;
    }

    //------------------------------------------------------------------------
    //! 1x1 のテクスチャを作成します。
    //------------------------------------------------------------------------
    bool RenderResources::Gpu::Create1x1Texture(u32 rgba, TextureHandle& outTexture, const char* debugName) {
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

        Microsoft::WRL::ComPtr<ID3D11Texture2D> texture;
        if(FAILED(m_device->CreateTexture2D(&desc, &initData, texture.GetAddressOf()))) {
            Tsukino::Core::Log::Error(std::string("Failed to create default texture: ") + debugName + ".");
            return false;
        }

        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> view;
        if(FAILED(m_device->CreateShaderResourceView(texture.Get(), nullptr, view.GetAddressOf()))) {
            Tsukino::Core::Log::Error(std::string("Failed to create default texture SRV: ") + debugName + ".");
            return false;
        }

        outTexture = AddTextureSlot(std::move(view));
        return outTexture.IsValid();
    }

    //------------------------------------------------------------------------
    //! メッシュデータを GPU へ送り、メッシュバッファを作成します。
    //------------------------------------------------------------------------
    MeshBuffer RenderResources::Gpu::CreateMesh(const Tsukino::GraphicsCommon::MeshData& meshData) const {
        return CreateMeshBuffer(m_device, meshData);
    }

    //========================================================================
    // RenderResources（公開関数は Gpu へそのまま委ねる）
    //========================================================================

    //------------------------------------------------------------------------
    //! 資源を持たない状態で作ります。
    //------------------------------------------------------------------------
    RenderResources::RenderResources()
        : m_gpu(std::make_unique<Gpu>()) {}

    //------------------------------------------------------------------------
    //! 資源を破棄します。
    //------------------------------------------------------------------------
    RenderResources::~RenderResources() = default;

    //------------------------------------------------------------------------
    //! 資源を作成します。
    //------------------------------------------------------------------------
    bool RenderResources::Initialize(const GraphicsContext& graphicsContext) {
        return m_gpu->Initialize(graphicsContext);
    }

    //------------------------------------------------------------------------
    //! パイプラインの生成器を取得します。
    //------------------------------------------------------------------------
    PipelineFactory* RenderResources::GetPipelineFactory() {
        return m_gpu->GetPipelineFactory();
    }

    //------------------------------------------------------------------------
    //! プリミティブメッシュを取得します。
    //------------------------------------------------------------------------
    MeshBuffer* RenderResources::GetPrimitiveMesh(Tsukino::GraphicsCommon::PrimitiveType type) {
        return m_gpu->GetPrimitiveMesh(type);
    }

    //------------------------------------------------------------------------
    //! メッシュデータを GPU へ送り、メッシュバッファを作成します。
    //------------------------------------------------------------------------
    MeshBuffer RenderResources::CreateMesh(const Tsukino::GraphicsCommon::MeshData& meshData) const {
        return m_gpu->CreateMesh(meshData);
    }

    //------------------------------------------------------------------------
    //! テクスチャアセットのハンドルを取得します。
    //------------------------------------------------------------------------
    TextureHandle RenderResources::GetTexture(Tsukino::Asset::TextureAsset& textureAsset) {
        return m_gpu->GetTexture(textureAsset);
    }

    //------------------------------------------------------------------------
    //! 白テクスチャのハンドルを取得します。
    //------------------------------------------------------------------------
    TextureHandle RenderResources::GetWhiteTexture() const {
        return m_gpu->GetWhiteTexture();
    }

    //------------------------------------------------------------------------
    //! フラット法線テクスチャのハンドルを取得します。
    //------------------------------------------------------------------------
    TextureHandle RenderResources::GetFlatNormalTexture() const {
        return m_gpu->GetFlatNormalTexture();
    }

    //------------------------------------------------------------------------
    //! 書き換えられる 2D テクスチャを作成します。
    //------------------------------------------------------------------------
    TextureHandle RenderResources::CreateTexture(u32 width, u32 height, Tsukino::GraphicsCommon::TextureFormat format, const void* data, u32 rowPitch) {
        return m_gpu->CreateTexture(width, height, format, data, rowPitch);
    }

    //------------------------------------------------------------------------
    //! テクスチャの一部を書き換えます。
    //------------------------------------------------------------------------
    void RenderResources::UpdateTexture(TextureHandle texture, u32 x, u32 y, u32 width, u32 height, const void* data, u32 rowPitch) {
        m_gpu->UpdateTexture(texture, x, y, width, height, data, rowPitch);
    }

    //------------------------------------------------------------------------
    //! テクスチャを破棄します。
    //------------------------------------------------------------------------
    void RenderResources::DestroyTexture(TextureHandle texture) {
        m_gpu->DestroyTexture(texture);
    }

    //------------------------------------------------------------------------
    //! インスタンス描画用のバッファを作成します。
    //------------------------------------------------------------------------
    BufferHandle RenderResources::CreateInstanceBuffer(u32 stride, u32 capacity) {
        return m_gpu->CreateInstanceBuffer(stride, capacity);
    }

    //------------------------------------------------------------------------
    //! インスタンス描画用のバッファの中身を書き換えます。
    //------------------------------------------------------------------------
    u32 RenderResources::UpdateInstanceBuffer(BufferHandle buffer, const void* data, u32 count) {
        return m_gpu->UpdateInstanceBuffer(buffer, data, count);
    }

    //------------------------------------------------------------------------
    //! ゲーム定義の定数バッファを作成します。
    //------------------------------------------------------------------------
    BufferHandle RenderResources::CreateUserConstantBuffer(u32 byteSize) {
        return m_gpu->CreateUserConstantBuffer(byteSize);
    }

    //------------------------------------------------------------------------
    //! ゲーム定義の定数バッファの中身を書き換えます。
    //------------------------------------------------------------------------
    void RenderResources::UpdateUserConstantBuffer(BufferHandle buffer, const void* data, u32 byteSize) {
        m_gpu->UpdateUserConstantBuffer(buffer, data, byteSize);
    }

    //------------------------------------------------------------------------
    //! バッファを破棄します。
    //------------------------------------------------------------------------
    void RenderResources::DestroyBuffer(BufferHandle buffer) {
        m_gpu->DestroyBuffer(buffer);
    }
}    // namespace Tsukino::Renderer
