//----------------------------------------------------------------------------
//! @file   RenderResources.cpp
//! @brief  bgfx 版の RenderResources・MeshBuffer・PipelineFactory の実装
//! @detail テクスチャ（アセットの画像・書き換えられるテクスチャ）とメッシュのバッファを
//!         bgfx で作り、エンジンのハンドルで引けるように表に持ちます。
//!         パイプラインは、変換済みの bgfx のシェーダー（.sc.bin）と描画ステートの組です。
//!         インスタンス描画のバッファとゲーム定義の定数バッファは、まだハンドルを配るだけです。
//----------------------------------------------------------------------------
#include <Tsukino/Renderer/Bgfx/RenderResourcesGpu.hpp>
#include <Tsukino/Renderer/Bgfx/GraphicsContext.hpp>

#include <Tsukino/Engine/Asset/Shader/ShaderAsset.hpp>
#include <Tsukino/Engine/Asset/Texture/TextureAsset.hpp>

#include <Tsukino/GraphicsCommon/Mesh/MeshData.hpp>
#include <Tsukino/GraphicsCommon/Mesh/MeshPrimitives.hpp>

#include <Tsukino/Core/Log.hpp>

#include <algorithm>
#include <cmath>
#include <cstring>

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {

    namespace {
        using Tsukino::GraphicsCommon::TextureFormat;

        //--------------------------------------------------------------------
        //! エンジンのテクスチャの形式を bgfx で扱うための情報です。
        //--------------------------------------------------------------------
        struct BgfxFormatInfo {
            bgfx::TextureFormat::Enum format     = bgfx::TextureFormat::Unknown;    // bgfx の形式
            bool                      srgb       = false;                           // sRGB として読むか（BGFX_TEXTURE_SRGB）
            u32                       blockBytes = 0;                               // 1ブロック（非圧縮は1画素）のバイト数
            u32                       blockSize  = 1;                               // 1ブロックの一辺の画素数（BC は 4）
        };

        //--------------------------------------------------------------------
        //! エンジンのテクスチャの形式を bgfx の形式に直します。
        //! @param  [in] format エンジンの形式
        //! @return bgfx で扱うための情報（対応していなければ format が Unknown）
        //--------------------------------------------------------------------
        BgfxFormatInfo ToBgfxFormat(TextureFormat format) {
            using F = bgfx::TextureFormat;
            switch(format) {
            case TextureFormat::R8:         return {F::R8, false, 1, 1};
            case TextureFormat::RG8:        return {F::RG8, false, 2, 1};
            case TextureFormat::RGBA8:      return {F::RGBA8, false, 4, 1};
            case TextureFormat::RGBA8_sRGB: return {F::RGBA8, true, 4, 1};
            case TextureFormat::BGRA8:      return {F::BGRA8, false, 4, 1};
            case TextureFormat::BGRA8_sRGB: return {F::BGRA8, true, 4, 1};
            case TextureFormat::R16:        return {F::R16, false, 2, 1};
            case TextureFormat::RGBA16:     return {F::RGBA16, false, 8, 1};
            case TextureFormat::R16F:       return {F::R16F, false, 2, 1};
            case TextureFormat::RGBA16F:    return {F::RGBA16F, false, 8, 1};
            case TextureFormat::R32F:       return {F::R32F, false, 4, 1};
            case TextureFormat::RGBA32F:    return {F::RGBA32F, false, 16, 1};
            case TextureFormat::RGB10A2:    return {F::RGB10A2, false, 4, 1};
            case TextureFormat::BC1:        return {F::BC1, false, 8, 4};
            case TextureFormat::BC1_sRGB:   return {F::BC1, true, 8, 4};
            case TextureFormat::BC3:        return {F::BC3, false, 16, 4};
            case TextureFormat::BC3_sRGB:   return {F::BC3, true, 16, 4};
            case TextureFormat::BC4:        return {F::BC4, false, 8, 4};
            case TextureFormat::BC5:        return {F::BC5, false, 16, 4};
            case TextureFormat::BC7:        return {F::BC7, false, 16, 4};
            case TextureFormat::BC7_sRGB:   return {F::BC7, true, 16, 4};
            default:                        return {};
            }
        }

        //--------------------------------------------------------------------
        //! 1段（ミップ）ぶんの、すき間なく詰めたときの1行のバイト数を求めます。
        //! @param  [in] info  形式の情報
        //! @param  [in] width その段の幅
        //! @return 1行（BC はブロック1行）のバイト数
        //--------------------------------------------------------------------
        u32 PackedRowPitch(const BgfxFormatInfo& info, u32 width) {
            return std::max(1u, (width + info.blockSize - 1) / info.blockSize) * info.blockBytes;
        }

        //--------------------------------------------------------------------
        //! 1段（ミップ）ぶんの行数を求めます。
        //! @param  [in] info   形式の情報
        //! @param  [in] height その段の高さ
        //! @return 行数（BC はブロックの行数）
        //--------------------------------------------------------------------
        u32 RowCount(const BgfxFormatInfo& info, u32 height) {
            return std::max(1u, (height + info.blockSize - 1) / info.blockSize);
        }

        //--------------------------------------------------------------------
        //! 縦横から、1×1 までのミップの段数を求めます。
        //! @param  [in] width  幅
        //! @param  [in] height 高さ
        //! @return 段数（1 以上）
        //--------------------------------------------------------------------
        u32 FullMipCount(u32 width, u32 height) {
            u32 count = 1;
            for(u32 size = std::max(width, height); size > 1; size >>= 1)
                ++count;
            return count;
        }

        //--------------------------------------------------------------------
        //! シェーダーのバイナリが bgfx の形式（shaderc の出力）かを確かめます。
        //! @param  [in] binary シェーダーのバイナリ
        //! @return bgfx の形式なら true（頭が "VSH" / "FSH" / "CSH"）
        //--------------------------------------------------------------------
        bool IsBgfxShaderBinary(const std::vector<u8>& binary) {
            if(binary.size() < 4)
                return false;
            const bool known = binary[0] == 'V' || binary[0] == 'F' || binary[0] == 'C';
            return known && binary[1] == 'S' && binary[2] == 'H';
        }

        //--------------------------------------------------------------------
        //! メッシュデータから、bgfx のバッファを持ったメッシュバッファを作ります。
        //! @param  [in] meshData メッシュデータ
        //! @return メッシュバッファ（対応していない頂点の形式なら、バッファは無効のまま）
        //--------------------------------------------------------------------
        MeshBuffer MakeMesh(const Tsukino::GraphicsCommon::MeshData& meshData) {
            MeshBuffer buffer{};
            buffer.gpu         = std::make_shared<MeshBuffer::Gpu>();
            buffer.vertexCount = meshData.vertexCount;
            buffer.indexCount  = meshData.indexCount;
            buffer.stride      = meshData.vertexStride;

            //----------------------------------------------------------------
            // 外接球（DX11 版と同じ求め方）
            //----------------------------------------------------------------
            const float extentX = (meshData.bounds.max.x - meshData.bounds.min.x) * 0.5f;
            const float extentY = (meshData.bounds.max.y - meshData.bounds.min.y) * 0.5f;
            const float extentZ = (meshData.bounds.max.z - meshData.bounds.min.z) * 0.5f;
            const float radius  = std::sqrt(extentX * extentX + extentY * extentY + extentZ * extentZ);
            buffer.boundsCenter = hlslpp::float3((meshData.bounds.max.x + meshData.bounds.min.x) * 0.5f,
                                                 (meshData.bounds.max.y + meshData.bounds.min.y) * 0.5f,
                                                 (meshData.bounds.max.z + meshData.bounds.min.z) * 0.5f);
            buffer.boundsRadius = (std::isfinite(radius) && radius > 0.0f) ? radius : 0.0f;

            //----------------------------------------------------------------
            // 頂点の形式が bgfx の頂点レイアウトに直せるものだけ、GPU へ送る。
            // スキニングなど、まだ直せない形式は送らない（そのメッシュは描かれない）
            //----------------------------------------------------------------
            const bgfx::VertexLayout& layout = RenderResources::Gpu::GetVertexLayout(meshData.format);
            if(layout.getStride() == 0 || layout.getStride() != meshData.vertexStride || meshData.vertexData.empty()) {
                return buffer;
            }

            buffer.gpu->vertexBuffer =
                bgfx::createVertexBuffer(bgfx::copy(meshData.vertexData.data(), static_cast<u32>(meshData.vertexData.size())), layout);
            if(!meshData.indices.empty()) {
                buffer.gpu->indexBuffer = bgfx::createIndexBuffer(
                    bgfx::copy(meshData.indices.data(), static_cast<u32>(meshData.indices.size() * sizeof(u32))), BGFX_BUFFER_INDEX32);
            }
            return buffer;
        }

        //--------------------------------------------------------------------
        //! 1×1 の RGBA8 のテクスチャを作ります。
        //! @param  [in] rgba 画素（R, G, B, A の順のバイト）
        //! @return bgfx のテクスチャ
        //--------------------------------------------------------------------
        bgfx::TextureHandle CreateSolidTexture(const u8 (&rgba)[4]) {
            return bgfx::createTexture2D(1, 1, false, 1, bgfx::TextureFormat::RGBA8, BGFX_TEXTURE_NONE, bgfx::copy(rgba, sizeof(rgba)));
        }
    }    // namespace

    //------------------------------------------------------------------------
    //! プログラムを破棄します。
    //------------------------------------------------------------------------
    PipelineState::~PipelineState() {
        if(GraphicsContext::IsBgfxAlive() && bgfx::isValid(program))
            bgfx::destroy(program);
    }

    //------------------------------------------------------------------------
    //! バッファを破棄します。
    //------------------------------------------------------------------------
    MeshBuffer::Gpu::~Gpu() {
        if(!GraphicsContext::IsBgfxAlive())
            return;
        if(bgfx::isValid(vertexBuffer))
            bgfx::destroy(vertexBuffer);
        if(bgfx::isValid(indexBuffer))
            bgfx::destroy(indexBuffer);
    }

    //------------------------------------------------------------------------
    //! 持っているテクスチャを全部破棄します。
    //------------------------------------------------------------------------
    RenderResources::Gpu::~Gpu() {
        if(!GraphicsContext::IsBgfxAlive())
            return;
        for(const TextureEntry& entry : textures) {
            if(bgfx::isValid(entry.texture))
                bgfx::destroy(entry.texture);
        }
    }

    //------------------------------------------------------------------------
    //! bgfx のテクスチャを表に入れて、エンジンのハンドルを配ります。
    //------------------------------------------------------------------------
    TextureHandle RenderResources::Gpu::AddTexture(const TextureEntry& entry) {
        TextureHandle handle;
        if(!freeTextureSlots.empty()) {
            handle.index = freeTextureSlots.back();
            freeTextureSlots.pop_back();
            textures[handle.index] = entry;
            return handle;
        }
        if(textures.size() >= TextureHandle::kInvalidIndex) {
            Tsukino::Core::Log::Error("RenderResources: Too many textures; the texture is discarded.");
            if(bgfx::isValid(entry.texture))
                bgfx::destroy(entry.texture);
            return handle;
        }
        handle.index = static_cast<u16>(textures.size());
        textures.push_back(entry);
        return handle;
    }

    //------------------------------------------------------------------------
    //! エンジンのハンドルから、表の1行を引きます。
    //------------------------------------------------------------------------
    const RenderResources::Gpu::TextureEntry* RenderResources::Gpu::FindTexture(TextureHandle handle) const {
        if(!handle.IsValid() || handle.index >= textures.size())
            return nullptr;
        const TextureEntry& entry = textures[handle.index];
        return bgfx::isValid(entry.texture) ? &entry : nullptr;
    }

    //------------------------------------------------------------------------
    //! メッシュの頂点の形式から、bgfx の頂点レイアウトを返します。
    //------------------------------------------------------------------------
    const bgfx::VertexLayout& RenderResources::Gpu::GetVertexLayout(Tsukino::GraphicsCommon::VertexFormat format) {
        using Tsukino::GraphicsCommon::VertexFormat;

        // 位置と UV（VertexPUV。静的メッシュとスプライト）
        static const bgfx::VertexLayout kPositionUV = [] {
            bgfx::VertexLayout layout;
            layout.begin()
                .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
                .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
                .end();
            return layout;
        }();

        // 位置・法線・UV（VertexPNUV。ふつうの 3D メッシュ）
        static const bgfx::VertexLayout kPositionNormalUV = [] {
            bgfx::VertexLayout layout;
            layout.begin()
                .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
                .add(bgfx::Attrib::Normal, 3, bgfx::AttribType::Float)
                .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
                .end();
            return layout;
        }();

        // まだ直していない形式（スキニング・パーティクル）。ストライドが 0 なので描かない
        static const bgfx::VertexLayout kEmpty{};

        switch(format) {
        case VertexFormat::PositionUV:
        case VertexFormat::Sprite:           return kPositionUV;
        case VertexFormat::PositionNormalUV: return kPositionNormalUV;
        default:                             return kEmpty;
        }
    }

    //------------------------------------------------------------------------
    //! サンプラーの種類から、bgfx のサンプラーのフラグを返します。
    //------------------------------------------------------------------------
    u32 RenderResources::Gpu::GetSamplerFlags(Tsukino::GraphicsCommon::SamplerType type) {
        using Tsukino::GraphicsCommon::SamplerType;
        constexpr u32 kClamp = BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP | BGFX_SAMPLER_W_CLAMP;
        constexpr u32 kPoint = BGFX_SAMPLER_MIN_POINT | BGFX_SAMPLER_MAG_POINT | BGFX_SAMPLER_MIP_POINT;
        constexpr u32 kAniso = BGFX_SAMPLER_MIN_ANISOTROPIC | BGFX_SAMPLER_MAG_ANISOTROPIC;

        switch(type) {
        case SamplerType::PointWrap:        return kPoint;
        case SamplerType::PointClamp:       return kPoint | kClamp;
        case SamplerType::LinearClamp:      return kClamp;
        case SamplerType::AnisotropicWrap:  return kAniso;
        case SamplerType::AnisotropicClamp: return kAniso | kClamp;
        case SamplerType::LinearWrap:
        default:                            return BGFX_SAMPLER_NONE;    // bgfx の既定 = なめらか・リピート
        }
    }

    //------------------------------------------------------------------------
    //! パイプラインを作ります。
    //------------------------------------------------------------------------
    std::shared_ptr<PipelineState> PipelineFactory::Create(const Tsukino::Asset::ShaderAsset& vs, const Tsukino::Asset::ShaderAsset& ps,
                                                           Tsukino::GraphicsCommon::VertexFormat format, DepthMode depthMode,
                                                           BlendMode blendMode, CullMode cullMode) {
        const PipelineKey key{vs.GetHandle().Value(), ps.GetHandle().Value(), format, depthMode, blendMode, cullMode};
        if(auto it = m_cache.find(key); it != m_cache.end())
            return it->second;

        //--------------------------------------------------------------------
        // bgfx 版へまだ移していないシェーダー（HLSL を DX11 用にコンパイルしたもの）は
        // bgfx では使えない。作れなかったことも覚えておき、毎フレーム作り直したりログを出し続けたりしない
        //--------------------------------------------------------------------
        if(!IsBgfxShaderBinary(vs.binary) || !IsBgfxShaderBinary(ps.binary)) {
            Tsukino::Core::Log::Warn("PipelineFactory: Not a bgfx shader binary (not ported to .sc yet); draws with it are skipped: "
                                     + vs.filePath + " / " + ps.filePath);
            m_cache.emplace(key, nullptr);
            return nullptr;
        }

        const bgfx::ShaderHandle vsHandle = bgfx::createShader(bgfx::copy(vs.binary.data(), static_cast<u32>(vs.binary.size())));
        const bgfx::ShaderHandle psHandle = bgfx::createShader(bgfx::copy(ps.binary.data(), static_cast<u32>(ps.binary.size())));

        auto pipeline     = std::make_shared<PipelineState>();
        pipeline->program = bgfx::createProgram(vsHandle, psHandle, true);    // シェーダーはプログラムと一緒に破棄する
        pipeline->format  = format;
        if(!bgfx::isValid(pipeline->program)) {
            Tsukino::Core::Log::Error("PipelineFactory: Failed to create a bgfx program: " + vs.filePath + " / " + ps.filePath);
            m_cache.emplace(key, nullptr);
            return nullptr;
        }

        //--------------------------------------------------------------------
        // 描画ステート。DX11 版の PipelineFactory と同じ組み合わせにする
        //--------------------------------------------------------------------
        u64 state = 0;

        // 色の書き込みとブレンド
        switch(blendMode) {
        case BlendMode::Opaque:
            state |= BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A;
            break;
        case BlendMode::Alpha:
            state |= BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A
                     | BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_SRC_ALPHA, BGFX_STATE_BLEND_INV_SRC_ALPHA);
            break;
        case BlendMode::Additive:
            // アルファは書き換えない（DX11 版と同じ。SrcBlendAlpha=ZERO, DestBlendAlpha=ONE）
            state |= BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A
                     | BGFX_STATE_BLEND_FUNC_SEPARATE(BGFX_STATE_BLEND_SRC_ALPHA, BGFX_STATE_BLEND_ONE, BGFX_STATE_BLEND_ZERO,
                                                      BGFX_STATE_BLEND_ONE);
            break;
        case BlendMode::DepthOnly:
            break;    // 色は書かない
        }

        // 深度（リバース Z。近いほど大きい）
        switch(depthMode) {
        case DepthMode::None:          break;
        case DepthMode::ReadWrite:     state |= BGFX_STATE_DEPTH_TEST_GEQUAL | BGFX_STATE_WRITE_Z; break;
        case DepthMode::ReadOnly:      state |= BGFX_STATE_DEPTH_TEST_GEQUAL; break;
        case DepthMode::EqualReadOnly: state |= BGFX_STATE_DEPTH_TEST_EQUAL; break;
        }

        // カリング。DX11 版は時計回りが表（FrontCounterClockwise = FALSE）
        switch(cullMode) {
        case CullMode::None:  break;
        case CullMode::Back:  state |= BGFX_STATE_CULL_CCW; break;
        case CullMode::Front: state |= BGFX_STATE_CULL_CW; break;
        }

        pipeline->state = state;
        m_cache.emplace(key, pipeline);
        return pipeline;
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
            m_gpu->primitiveMeshes[i] = MakeMesh(Tsukino::GraphicsCommon::CreatePrimitiveMeshData(static_cast<PrimitiveType>(i)));
        }

        // 既定のテクスチャ（白と、法線マップが無いときの平らな法線）
        constexpr u8 kWhite[4]      = {255, 255, 255, 255};
        constexpr u8 kFlatNormal[4] = {128, 128, 255, 255};
        m_gpu->white      = m_gpu->AddTexture({CreateSolidTexture(kWhite), 1, 1, TextureFormat::RGBA8});
        m_gpu->flatNormal = m_gpu->AddTexture({CreateSolidTexture(kFlatNormal), 1, 1, TextureFormat::RGBA8});
        return m_gpu->white.IsValid() && m_gpu->flatNormal.IsValid();
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
    //! メッシュデータからメッシュバッファを作成します。
    //------------------------------------------------------------------------
    MeshBuffer RenderResources::CreateMesh(const Tsukino::GraphicsCommon::MeshData& meshData) const {
        return MakeMesh(meshData);
    }

    //------------------------------------------------------------------------
    //! テクスチャアセットを GPU へ送り、ハンドルを取得します。
    //------------------------------------------------------------------------
    TextureHandle RenderResources::GetTexture(Tsukino::Asset::TextureAsset& textureAsset) {
        const u64 key = textureAsset.GetHandle().Value();
        if(auto it = m_gpu->textureCache.find(key); it != m_gpu->textureCache.end())
            return it->second;

        const BgfxFormatInfo info = ToBgfxFormat(textureAsset.format);
        if(info.format == bgfx::TextureFormat::Unknown || textureAsset.width == 0 || textureAsset.height == 0
           || textureAsset.pixels.empty()) {
            Tsukino::Core::Log::Error("RenderResources: Unsupported or empty texture; the white texture is used instead.");
            return m_gpu->white;
        }

        //--------------------------------------------------------------------
        // ミップの段。bgfx は 1×1 までそろった段か、1段だけを受け付ける。
        // ファイルの段がそろっていなければ、いちばん上の段だけを送る。
        // 1段だけの画像の generateMips（GPU で縮小を作る）は、bgfx 版ではまだしない
        //--------------------------------------------------------------------
        const u32  fileMips = static_cast<u32>(textureAsset.mips.size());
        const bool hasMips  = fileMips > 1 && fileMips == FullMipCount(textureAsset.width, textureAsset.height);
        const u32  mipCount = hasMips ? fileMips : 1;

        //--------------------------------------------------------------------
        // 段ごとに行のすき間を詰めて1つの塊にする（bgfx が受け取る並び）
        //--------------------------------------------------------------------
        u32 totalSize = 0;
        for(u32 mip = 0; mip < mipCount; ++mip) {
            const u32    width       = std::max(1u, textureAsset.width >> mip);
            const u32    height      = std::max(1u, textureAsset.height >> mip);
            const u32    packedPitch = PackedRowPitch(info, width);
            const u32    rows        = RowCount(info, height);
            const size_t srcOffset   = fileMips > 0 ? textureAsset.mips[mip].offset : 0;
            const u32    srcPitch    = (fileMips > 0 && textureAsset.mips[mip].rowPitch != 0) ? textureAsset.mips[mip].rowPitch : packedPitch;

            // 画素が足りなければ送らない（alloc した塊は bgfx に渡すまで手放せないので、先に確かめる）
            if(srcPitch < packedPitch || srcOffset + static_cast<size_t>(srcPitch) * (rows - 1) + packedPitch > textureAsset.pixels.size()) {
                Tsukino::Core::Log::Error("RenderResources: Texture data is shorter than its size; the white texture is used instead.");
                return m_gpu->white;
            }
            totalSize += packedPitch * rows;
        }

        const bgfx::Memory* memory = bgfx::alloc(totalSize);
        u8*                 dst    = memory->data;
        for(u32 mip = 0; mip < mipCount; ++mip) {
            const u32    width       = std::max(1u, textureAsset.width >> mip);
            const u32    height      = std::max(1u, textureAsset.height >> mip);
            const u32    packedPitch = PackedRowPitch(info, width);
            const u32    rows        = RowCount(info, height);
            const size_t srcOffset   = fileMips > 0 ? textureAsset.mips[mip].offset : 0;
            const u32    srcPitch    = (fileMips > 0 && textureAsset.mips[mip].rowPitch != 0) ? textureAsset.mips[mip].rowPitch : packedPitch;
            for(u32 row = 0; row < rows; ++row) {
                std::memcpy(dst, textureAsset.pixels.data() + srcOffset + static_cast<size_t>(srcPitch) * row, packedPitch);
                dst += packedPitch;
            }
        }

        const u64 flags = info.srgb ? BGFX_TEXTURE_SRGB : BGFX_TEXTURE_NONE;
        const bgfx::TextureHandle texture =
            bgfx::createTexture2D(static_cast<u16>(textureAsset.width), static_cast<u16>(textureAsset.height), hasMips, 1, info.format, flags, memory);
        if(!bgfx::isValid(texture)) {
            Tsukino::Core::Log::Error("RenderResources: Failed to create a bgfx texture (the texture limit may be reached).");
            return m_gpu->white;
        }

        const TextureHandle handle = m_gpu->AddTexture(
            {texture, static_cast<u16>(textureAsset.width), static_cast<u16>(textureAsset.height), textureAsset.format});
        if(!handle.IsValid())
            return m_gpu->white;
        m_gpu->textureCache.emplace(key, handle);

        // DX11 版と同じく、送り終えたので CPU 側の画素は手放す
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
    //! 書き換えられる 2D テクスチャを作成します。
    //------------------------------------------------------------------------
    TextureHandle RenderResources::CreateTexture(u32 width, u32 height, Tsukino::GraphicsCommon::TextureFormat format, const void* data,
                                                 u32 rowPitch) {
        const BgfxFormatInfo info = ToBgfxFormat(format);
        if(info.format == bgfx::TextureFormat::Unknown || width == 0 || height == 0 || width > 0xFFFF || height > 0xFFFF) {
            Tsukino::Core::Log::Error("RenderResources: Invalid texture description.");
            return {};
        }

        // 中身は後から書き換えるので、初期データ無しで作る（書き換えられるテクスチャになる）
        const u64 flags = info.srgb ? BGFX_TEXTURE_SRGB : BGFX_TEXTURE_NONE;
        const bgfx::TextureHandle texture =
            bgfx::createTexture2D(static_cast<u16>(width), static_cast<u16>(height), false, 1, info.format, flags, nullptr);
        if(!bgfx::isValid(texture)) {
            Tsukino::Core::Log::Error("RenderResources: Failed to create a bgfx texture.");
            return {};
        }

        const TextureHandle handle = m_gpu->AddTexture({texture, static_cast<u16>(width), static_cast<u16>(height), format});
        if(!handle.IsValid())
            return handle;

        //--------------------------------------------------------------------
        // 中身を渡されなかったときは 0 で埋める（DX11 版と同じ。初期データ無しの中身は決まっていない）
        //--------------------------------------------------------------------
        if(data) {
            UpdateTexture(handle, 0, 0, width, height, data, rowPitch);
        } else {
            const u32           packedPitch = PackedRowPitch(info, width);
            const bgfx::Memory* zeros       = bgfx::alloc(packedPitch * RowCount(info, height));
            std::memset(zeros->data, 0, zeros->size);
            bgfx::updateTexture2D(texture, 0, 0, 0, 0, static_cast<u16>(width), static_cast<u16>(height), zeros, static_cast<u16>(packedPitch));
        }
        return handle;
    }

    //------------------------------------------------------------------------
    //! テクスチャの一部を書き換えます。
    //------------------------------------------------------------------------
    void RenderResources::UpdateTexture(TextureHandle texture, u32 x, u32 y, u32 width, u32 height, const void* data, u32 rowPitch) {
        const Gpu::TextureEntry* entry = m_gpu->FindTexture(texture);
        if(!entry || !data || width == 0 || height == 0)
            return;

        const BgfxFormatInfo info  = ToBgfxFormat(entry->format);
        const u32            pitch = rowPitch != 0 ? rowPitch : PackedRowPitch(info, width);
        const u32            rows  = RowCount(info, height);

        // 最後の行は詰めた幅までしか読まない（呼び出し側の画素の終わりを越えないように）
        const u32 size = pitch * (rows - 1) + PackedRowPitch(info, width);
        bgfx::updateTexture2D(entry->texture, 0, 0, static_cast<u16>(x), static_cast<u16>(y), static_cast<u16>(width), static_cast<u16>(height),
                              bgfx::copy(data, size), static_cast<u16>(pitch));
    }

    //------------------------------------------------------------------------
    //! テクスチャを破棄します。
    //------------------------------------------------------------------------
    void RenderResources::DestroyTexture(TextureHandle texture) {
        if(!texture.IsValid() || texture.index >= m_gpu->textures.size() || texture == m_gpu->white || texture == m_gpu->flatNormal)
            return;

        Gpu::TextureEntry& entry = m_gpu->textures[texture.index];
        if(!bgfx::isValid(entry.texture))
            return;
        bgfx::destroy(entry.texture);
        entry = Gpu::TextureEntry{};
        m_gpu->freeTextureSlots.push_back(texture.index);
    }

    //------------------------------------------------------------------------
    //! インスタンス描画用のバッファを作成します（bgfx 版はまだハンドルを配るだけ）。
    //------------------------------------------------------------------------
    BufferHandle RenderResources::CreateInstanceBuffer(u32 /*stride*/, u32 /*capacity*/) {
        BufferHandle handle;
        if(m_gpu->nextBuffer < BufferHandle::kInvalidIndex)
            handle.index = m_gpu->nextBuffer++;
        return handle;
    }

    //------------------------------------------------------------------------
    //! インスタンス描画用のバッファの中身を書き換えます（bgfx 版はまだ何もしない）。
    //------------------------------------------------------------------------
    u32 RenderResources::UpdateInstanceBuffer(BufferHandle /*buffer*/, const void* /*data*/, u32 count) {
        return count;
    }

    //------------------------------------------------------------------------
    //! ゲーム定義の定数バッファを作成します（bgfx 版はまだハンドルを配るだけ）。
    //------------------------------------------------------------------------
    BufferHandle RenderResources::CreateUserConstantBuffer(u32 /*byteSize*/) {
        BufferHandle handle;
        if(m_gpu->nextBuffer < BufferHandle::kInvalidIndex)
            handle.index = m_gpu->nextBuffer++;
        return handle;
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
