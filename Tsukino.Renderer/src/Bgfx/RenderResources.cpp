//----------------------------------------------------------------------------
//! @file   RenderResources.cpp
//! @brief  bgfx 版の RenderResources の実装（作りかけ）
//! @detail 今はハンドルを配るだけで、GPU の資源はまだ作りません。テクスチャは
//!         アセットごとに同じハンドルを返し、メッシュは頂点数や外接球だけを持ちます。
//!         描画を bgfx へ移すときに、ここで bgfx の資源を作るようにします。
//----------------------------------------------------------------------------
#include <Tsukino/Renderer/RenderResources.hpp>
#include <Tsukino/Renderer/Bgfx/GraphicsContext.hpp>

#include <Tsukino/Engine/Asset/Texture/TextureAsset.hpp>

#include <Tsukino/GraphicsCommon/Mesh/MeshData.hpp>
#include <Tsukino/GraphicsCommon/Mesh/MeshPrimitives.hpp>

#include <array>
#include <cmath>
#include <optional>
#include <unordered_map>

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {

    //------------------------------------------------------------------------
    //! パイプラインの中身です（bgfx 版はまだ空）。
    //------------------------------------------------------------------------
    struct PipelineState {};

    //------------------------------------------------------------------------
    //! メッシュの GPU の資源です（bgfx 版はまだ空）。
    //------------------------------------------------------------------------
    struct MeshBuffer::Gpu {};

    //------------------------------------------------------------------------
    //! RenderResources の中身です。
    //------------------------------------------------------------------------
    struct RenderResources::Gpu {
        std::optional<PipelineFactory> pipelineFactory;    // パイプラインの生成器
        std::array<MeshBuffer, static_cast<size_t>(Tsukino::GraphicsCommon::PrimitiveType::Count)> primitiveMeshes;    // プリミティブ

        std::unordered_map<u64, TextureHandle> textureCache;    // アセットのハンドル → テクスチャのハンドル
        u16                                    nextTexture = 0;    // 次に配るテクスチャの番号
        u16                                    nextBuffer  = 0;    // 次に配るバッファの番号
        TextureHandle                          white;              // 白テクスチャ
        TextureHandle                          flatNormal;         // フラット法線テクスチャ

        //! テクスチャの番号を1つ配ります。
        //! @return テクスチャのハンドル（配り切ったら無効なハンドル）
        TextureHandle NewTexture() {
            TextureHandle handle;
            if(nextTexture < TextureHandle::kInvalidIndex)
                handle.index = nextTexture++;
            return handle;
        }

        //! バッファの番号を1つ配ります。
        //! @return バッファのハンドル（配り切ったら無効なハンドル）
        BufferHandle NewBuffer() {
            BufferHandle handle;
            if(nextBuffer < BufferHandle::kInvalidIndex)
                handle.index = nextBuffer++;
            return handle;
        }
    };

    namespace {
        //! メッシュデータから、GPU の資源を持たないメッシュバッファを作ります。
        //! @param  [in] meshData メッシュデータ
        //! @return 頂点数・外接球だけを埋めたメッシュバッファ
        MeshBuffer MakeMeshInfo(const Tsukino::GraphicsCommon::MeshData& meshData) {
            MeshBuffer buffer{};
            buffer.gpu         = std::make_shared<MeshBuffer::Gpu>();
            buffer.vertexCount = meshData.vertexCount;
            buffer.indexCount  = meshData.indexCount;
            buffer.stride      = meshData.vertexStride;

            const float extentX = (meshData.bounds.max.x - meshData.bounds.min.x) * 0.5f;
            const float extentY = (meshData.bounds.max.y - meshData.bounds.min.y) * 0.5f;
            const float extentZ = (meshData.bounds.max.z - meshData.bounds.min.z) * 0.5f;
            const float radius  = std::sqrt(extentX * extentX + extentY * extentY + extentZ * extentZ);
            buffer.boundsCenter = hlslpp::float3((meshData.bounds.max.x + meshData.bounds.min.x) * 0.5f,
                                                 (meshData.bounds.max.y + meshData.bounds.min.y) * 0.5f,
                                                 (meshData.bounds.max.z + meshData.bounds.min.z) * 0.5f);
            buffer.boundsRadius = (std::isfinite(radius) && radius > 0.0f) ? radius : 0.0f;
            return buffer;
        }
    }    // namespace

    //------------------------------------------------------------------------
    //! パイプラインを作ります（bgfx 版はまだ空の中身を返すだけ）。
    //------------------------------------------------------------------------
    std::shared_ptr<PipelineState> PipelineFactory::Create(const Tsukino::Asset::ShaderAsset& /*vs*/, const Tsukino::Asset::ShaderAsset& /*ps*/,
                                                           Tsukino::GraphicsCommon::VertexFormat /*format*/, DepthMode /*depthMode*/,
                                                           BlendMode /*blendMode*/, CullMode /*cullMode*/) {
        return std::make_shared<PipelineState>();
    }

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
        m_gpu->pipelineFactory.emplace(graphicsContext);

        using Tsukino::GraphicsCommon::PrimitiveType;
        for(size_t i = 0; i < m_gpu->primitiveMeshes.size(); ++i) {
            m_gpu->primitiveMeshes[i] = MakeMeshInfo(Tsukino::GraphicsCommon::CreatePrimitiveMeshData(static_cast<PrimitiveType>(i)));
        }

        m_gpu->white      = m_gpu->NewTexture();
        m_gpu->flatNormal = m_gpu->NewTexture();
        return true;
    }

    //------------------------------------------------------------------------
    //! パイプラインの生成器を取得します。
    //------------------------------------------------------------------------
    PipelineFactory* RenderResources::GetPipelineFactory() {
        return m_gpu->pipelineFactory ? &m_gpu->pipelineFactory.value() : nullptr;
    }

    //------------------------------------------------------------------------
    //! プリミティブメッシュを取得します。
    //------------------------------------------------------------------------
    MeshBuffer* RenderResources::GetPrimitiveMesh(Tsukino::GraphicsCommon::PrimitiveType type) {
        return &m_gpu->primitiveMeshes[static_cast<size_t>(type)];
    }

    //------------------------------------------------------------------------
    //! メッシュデータからメッシュバッファを作成します（bgfx 版は GPU へまだ送らない）。
    //------------------------------------------------------------------------
    MeshBuffer RenderResources::CreateMesh(const Tsukino::GraphicsCommon::MeshData& meshData) const {
        return MakeMeshInfo(meshData);
    }

    //------------------------------------------------------------------------
    //! テクスチャアセットのハンドルを取得します（bgfx 版は GPU へまだ送らない）。
    //------------------------------------------------------------------------
    TextureHandle RenderResources::GetTexture(Tsukino::Asset::TextureAsset& textureAsset) {
        const u64 key = textureAsset.GetHandle().Value();
        if(auto it = m_gpu->textureCache.find(key); it != m_gpu->textureCache.end())
            return it->second;

        const TextureHandle handle = m_gpu->NewTexture();
        m_gpu->textureCache.emplace(key, handle);

        // DX11 版と同じく、送り終えた扱いにして CPU 側の画素は手放す
        textureAsset.ReleasePixels();
        return handle;
    }

    //------------------------------------------------------------------------
    //! 白テクスチャのハンドルを取得します。
    //------------------------------------------------------------------------
    TextureHandle RenderResources::GetWhiteTexture() const {
        return m_gpu->white;
    }

    //------------------------------------------------------------------------
    //! フラット法線テクスチャのハンドルを取得します。
    //------------------------------------------------------------------------
    TextureHandle RenderResources::GetFlatNormalTexture() const {
        return m_gpu->flatNormal;
    }

    //------------------------------------------------------------------------
    //! 書き換えられる 2D テクスチャを作成します（bgfx 版はハンドルを配るだけ）。
    //------------------------------------------------------------------------
    TextureHandle RenderResources::CreateTexture(u32 /*width*/, u32 /*height*/, Tsukino::GraphicsCommon::TextureFormat /*format*/,
                                                 const void* /*data*/, u32 /*rowPitch*/) {
        return m_gpu->NewTexture();
    }

    //------------------------------------------------------------------------
    //! テクスチャの一部を書き換えます（bgfx 版はまだ何もしない）。
    //------------------------------------------------------------------------
    void RenderResources::UpdateTexture(TextureHandle /*texture*/, u32 /*x*/, u32 /*y*/, u32 /*width*/, u32 /*height*/,
                                        const void* /*data*/, u32 /*rowPitch*/) {}

    //------------------------------------------------------------------------
    //! テクスチャを破棄します（bgfx 版はまだ何もしない）。
    //------------------------------------------------------------------------
    void RenderResources::DestroyTexture(TextureHandle /*texture*/) {}

    //------------------------------------------------------------------------
    //! インスタンス描画用のバッファを作成します（bgfx 版はハンドルを配るだけ）。
    //------------------------------------------------------------------------
    BufferHandle RenderResources::CreateInstanceBuffer(u32 /*stride*/, u32 /*capacity*/) {
        return m_gpu->NewBuffer();
    }

    //------------------------------------------------------------------------
    //! インスタンス描画用のバッファの中身を書き換えます（bgfx 版はまだ何もしない）。
    //------------------------------------------------------------------------
    u32 RenderResources::UpdateInstanceBuffer(BufferHandle /*buffer*/, const void* /*data*/, u32 count) {
        return count;
    }

    //------------------------------------------------------------------------
    //! ゲーム定義の定数バッファを作成します（bgfx 版はハンドルを配るだけ）。
    //------------------------------------------------------------------------
    BufferHandle RenderResources::CreateUserConstantBuffer(u32 /*byteSize*/) {
        return m_gpu->NewBuffer();
    }

    //------------------------------------------------------------------------
    //! ゲーム定義の定数バッファの中身を書き換えます（bgfx 版はまだ何もしない）。
    //------------------------------------------------------------------------
    void RenderResources::UpdateUserConstantBuffer(BufferHandle /*buffer*/, const void* /*data*/, u32 /*byteSize*/) {}

    //------------------------------------------------------------------------
    //! バッファを破棄します（bgfx 版はまだ何もしない）。
    //------------------------------------------------------------------------
    void RenderResources::DestroyBuffer(BufferHandle /*buffer*/) {}

}    // namespace Tsukino::Renderer
