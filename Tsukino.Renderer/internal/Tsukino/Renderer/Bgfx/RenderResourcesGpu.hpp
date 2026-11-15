//----------------------------------------------------------------------------
//! @file   RenderResourcesGpu.hpp
//! @brief  bgfx 版の RenderResources・MeshBuffer・PipelineState の中身（Renderer の内部用）
//! @detail エンジンのハンドル（TextureHandle）から bgfx のテクスチャを引く表、
//!         メッシュの頂点・インデックスバッファ、パイプライン（シェーダーと描画ステート）を持ちます。
//!         描画コマンドを実行する DrawCommandExecutor がここから bgfx の資源を取り出します。
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/Renderer/MeshBuffer.hpp>
#include <Tsukino/Renderer/PipelineFactory.hpp>
#include <Tsukino/Renderer/RenderResources.hpp>

#include <Tsukino/GraphicsCommon/Mesh/PrimitiveType.hpp>
#include <Tsukino/GraphicsCommon/State/SamplerType.hpp>
#include <Tsukino/GraphicsCommon/Vertex/VertexFormat.hpp>

#include <bgfx/bgfx.h>

#include <array>
#include <optional>
#include <unordered_map>
#include <vector>

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {

    //------------------------------------------------------------------------
    //! パイプライン（シェーダーと描画ステート）です。
    //! @note  PipelineFactory::Create が作る。
    //!        Material が指して、描画コマンドの実行で使う
    //------------------------------------------------------------------------
    struct PipelineState {
        bgfx::ProgramHandle                   program = BGFX_INVALID_HANDLE;    // 頂点・ピクセルシェーダーの組
        u64                                   state   = 0;                      // bgfx の描画ステート（BGFX_STATE_*）
        Tsukino::GraphicsCommon::VertexFormat format  = Tsukino::GraphicsCommon::VertexFormat::Unknown;    // 頂点の形式

        PipelineState() = default;
        PipelineState(const PipelineState&)            = delete;
        PipelineState& operator=(const PipelineState&) = delete;

        //! プログラムを破棄します（bgfx を後始末した後なら何もしません）。
        ~PipelineState();
    };

    //------------------------------------------------------------------------
    //! メッシュの GPU の資源です。
    //------------------------------------------------------------------------
    struct MeshBuffer::Gpu {
        bgfx::VertexBufferHandle vertexBuffer = BGFX_INVALID_HANDLE;    // 頂点バッファ
        bgfx::IndexBufferHandle  indexBuffer  = BGFX_INVALID_HANDLE;    // インデックスバッファ（32bit）
        bgfx::VertexBufferHandle boneBuffer   = BGFX_INVALID_HANDLE;    // ボーンの番号とウェイト（スキニングするメッシュだけ。2本目の頂点バッファ）

        Gpu() = default;
        Gpu(const Gpu&)            = delete;
        Gpu& operator=(const Gpu&) = delete;

        //! バッファを破棄します（bgfx を後始末した後なら何もしません）。
        ~Gpu();
    };

    //------------------------------------------------------------------------
    //! RenderResources の中身です。
    //------------------------------------------------------------------------
    struct RenderResources::Gpu {
        //! テクスチャの表の1行
        struct TextureEntry {
            bgfx::TextureHandle texture = BGFX_INVALID_HANDLE;    // bgfx のテクスチャ
            u16                 width   = 0;                      // 幅（スプライトの UV を求めるのに使う）
            u16                 height  = 0;                      // 高さ
            Tsukino::GraphicsCommon::TextureFormat format = Tsukino::GraphicsCommon::TextureFormat::Unknown;    // 形式（書き換えのときに使う）
        };

        //! バッファの種類
        enum class BufferKind : u8 {
            None,            // 空き
            Instance,        // インスタンスごとのデータ（DrawCommand::instanceData）
            UserConstant,    // ゲーム定義の定数（DrawCommand::userConstantBuffer）
        };

        //! バッファの表の1行
        //! @note  中身は CPU 側に持ち、描くときに送る。bgfx のユニフォームもインスタンスのデータも、
        //!        描画1回ごとに渡すものなので、GPU 側に置きっぱなしのバッファは作らない
        struct BufferEntry {
            BufferKind      kind        = BufferKind::None;    // 種類
            u32             stride      = 0;                   // 1要素のバイト数（インスタンスは1インスタンスぶん、定数は 16）
            u32             capacity    = 0;                   // 確保した要素数（インスタンスの数、または vec4 の数）
            u32             activeCount = 0;                   // 直近の更新で書き込んだインスタンスの数（定数では使わない）
            std::vector<u8> data;                              // 中身（stride × capacity バイト）
        };

        //! 1インスタンスのデータの上限（bgfx のインスタンスのデータは vec4 が16個まで。BGFX_CONFIG_MAX_INSTANCE_DATA_COUNT）
        static constexpr u32 kMaxInstanceStride = 16 * 16;

        //! ゲーム定義の定数バッファ1本の上限（vec4 の数。1024 バイト）
        static constexpr u16 kMaxUserConstantVec4Count = 64;

        //! ゲーム定義の定数バッファの枠の数（CBSlot::User0 / User1）
        static constexpr u32 kUserConstantSlotCount = 2;

        std::optional<PipelineFactory> pipelineFactory;    // パイプラインの生成器
        std::array<MeshBuffer, static_cast<size_t>(Tsukino::GraphicsCommon::PrimitiveType::Count)> primitiveMeshes;    // プリミティブ

        std::vector<TextureEntry>              textures;           // エンジンのハンドルの番号 → bgfx のテクスチャ
        std::vector<u16>                       freeTextureSlots;   // 空いた番号（破棄したテクスチャの分）
        std::unordered_map<u64, TextureHandle> textureCache;       // アセットのハンドル → テクスチャのハンドル
        std::vector<BufferEntry>               buffers;            // エンジンのハンドルの番号 → バッファ
        std::vector<u16>                       freeBufferSlots;    // 空いた番号（破棄したバッファの分）
        TextureHandle                          white;              // 白テクスチャ
        TextureHandle                          flatNormal;         // フラット法線テクスチャ

        //! ゲーム定義の定数のユニフォーム（u_user0 / u_user1）
        std::array<bgfx::UniformHandle, kUserConstantSlotCount> userConstantUniforms{bgfx::UniformHandle BGFX_INVALID_HANDLE,
                                                                                     bgfx::UniformHandle BGFX_INVALID_HANDLE};

        Gpu() = default;
        Gpu(const Gpu&)            = delete;
        Gpu& operator=(const Gpu&) = delete;

        //! 持っているテクスチャを全部破棄します。
        ~Gpu();

        //! bgfx のテクスチャを表に入れて、エンジンのハンドルを配ります。
        //! @param  [in] entry 表に入れるテクスチャ
        //! @return テクスチャのハンドル（表が一杯なら無効なハンドル。そのとき entry のテクスチャは破棄する）
        TextureHandle AddTexture(const TextureEntry& entry);

        //! エンジンのハンドルから、表の1行を引きます。
        //! @param  [in] handle テクスチャのハンドル
        //! @return 表の1行（無効なハンドルや破棄済みなら nullptr）
        [[nodiscard]]
        const TextureEntry* FindTexture(TextureHandle handle) const;

        //! バッファを表に入れて、エンジンのハンドルを配ります。
        //! @param  [in] entry 表に入れるバッファ
        //! @return バッファのハンドル（表が一杯なら無効なハンドル）
        BufferHandle AddBuffer(BufferEntry entry);

        //! エンジンのハンドルから、指定した種類のバッファを引きます。
        //! @param  [in] handle バッファのハンドル
        //! @param  [in] kind   バッファの種類
        //! @return 表の1行（無効なハンドル・破棄済み・種類が違うなら nullptr）
        [[nodiscard]]
        const BufferEntry* FindBuffer(BufferHandle handle, BufferKind kind) const;

        //! メッシュの頂点の形式から、bgfx の頂点レイアウトを返します。
        //! @param  [in] format 頂点の形式
        //! @return 頂点レイアウト（対応していない形式なら空のレイアウト。そのときは描かない）
        [[nodiscard]]
        static const bgfx::VertexLayout& GetVertexLayout(Tsukino::GraphicsCommon::VertexFormat format);

        //! スキニングの2本目の頂点バッファ（ボーンの番号 4つとウェイト 4つ。どちらも float）の頂点レイアウトを返します。
        //! @return 頂点レイアウト
        //! @note   bgfx の頂点の型に 32bit 整数が無いので、番号も float で持つ（128 までなので正確に表せる）
        [[nodiscard]]
        static const bgfx::VertexLayout& GetBoneLayout();

        //! サンプラーの種類から、bgfx のサンプラーのフラグを返します。
        //! @param  [in] type サンプラーの種類
        //! @return BGFX_SAMPLER_* の組み合わせ
        [[nodiscard]]
        static u32 GetSamplerFlags(Tsukino::GraphicsCommon::SamplerType type);
    };

}    // namespace Tsukino::Renderer
