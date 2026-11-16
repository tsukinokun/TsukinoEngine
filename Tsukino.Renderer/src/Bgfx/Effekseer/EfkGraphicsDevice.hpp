//----------------------------------------------------------------------------
//! @file   EfkGraphicsDevice.hpp
//! @brief  Effekseer の描画デバイス（Effekseer::Backend::GraphicsDevice）の bgfx 版
//! @detail Effekseer の共通部分（EffekseerRendererCommon）が作る頂点・インデックスバッファ、頂点形式、
//!         テクスチャを bgfx の物で持ちます。中身はいったん CPU 側に持ち、描くときに bgfx へ渡します。
//!         - 毎フレーム書き換わる頂点（スプライトなど）は、描くたびに一時的な頂点バッファ（transient）へ写す
//!         - 書き換わらない頂点とインデックス（モデル・スプライトの並び）は、最初に描くときに静的なバッファを作る
//!         Effekseer の DX11 版（EffekseerRendererDX11 の GraphicsDevice）と同じ役割です。
//!         シェーダー・描く先・GPU パーティクルの道具（コンピュートシェーダーなど）は作りません。
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/Core/typedef.hpp>

#include <Effekseer.h>

#include <bgfx/bgfx.h>

#include <vector>

// 名前空間 : Tsukino::Renderer::EffekseerBgfx
namespace Tsukino::Renderer::EffekseerBgfx {

    //------------------------------------------------------------------------
    //! 頂点形式です。Effekseer の要素の並びを bgfx の頂点レイアウトへ直したものを持ちます。
    //! @note  Effekseer のセマンティクスを、シェーダーの入力の名前（Shaders/bgfx/Effekseer/varying.def.sc）へ当てる。
    //!        POSITION → a_position、NORMAL i → a_color{i}、TEXCOORD i → a_texcoord{i}
    //------------------------------------------------------------------------
    class VertexLayout final : public Effekseer::Backend::VertexLayout {
    public:
        //! 要素の並びから作ります。
        //! @param  [in] elements 要素の並び
        //! @param  [in] count    要素の数
        VertexLayout(const Effekseer::Backend::VertexLayoutElement* elements, int32_t count);

        //! bgfx の頂点レイアウトを返します。
        [[nodiscard]]
        const bgfx::VertexLayout& GetLayout() const noexcept {
            return m_layout;
        }

        //! すべての要素を bgfx の頂点レイアウトへ直せたかを返します。
        [[nodiscard]]
        bool IsValid() const noexcept {
            return m_valid;
        }

    private:
        bgfx::VertexLayout m_layout;           // bgfx の頂点レイアウト
        bool               m_valid = false;    // 直せたか
    };

    //------------------------------------------------------------------------
    //! 頂点バッファです。
    //------------------------------------------------------------------------
    class VertexBuffer final : public Effekseer::Backend::VertexBuffer {
    public:
        //! 作ります。
        //! @param  [in] size        バイト数
        //! @param  [in] initialData 最初の中身（無ければ nullptr）
        //! @param  [in] isDynamic   毎フレーム書き換えるか（true なら描くたびに一時的な頂点バッファへ写す）
        VertexBuffer(int32_t size, const void* initialData, bool isDynamic);

        //! bgfx のバッファを破棄します。
        ~VertexBuffer() override;

        //! 中身を書き換えます。
        //! @param  [in] src    書く中身
        //! @param  [in] size   書くバイト数
        //! @param  [in] offset 書く先の先頭（バイト）
        void UpdateData(const void* src, int32_t size, int32_t offset) override;

        //! 次の描画の頂点として bgfx へ渡します。
        //! @param  [in] firstVertex 先頭の頂点（layout の大きさで数える）
        //! @param  [in] vertexCount 頂点の数
        //! @param  [in] layout      頂点レイアウト
        //! @return 渡せたら true（範囲の外か、一時的な頂点バッファが足りないときは false）
        [[nodiscard]]
        bool Bind(u32 firstVertex, u32 vertexCount, const bgfx::VertexLayout& layout);

    private:
        //! 静的なバッファを破棄します。
        void DestroyHandle();

        std::vector<u8>          m_data;                                      // 中身（CPU 側）
        bool                     m_dynamic          = false;                  // 毎フレーム書き換えるか
        bgfx::VertexBufferHandle m_handle           = BGFX_INVALID_HANDLE;    // 静的なバッファ（書き換えないときだけ。最初に描くときに作る）
        u32                      m_handleLayoutHash = 0;                      // 静的なバッファを作った頂点レイアウト
    };

    //------------------------------------------------------------------------
    //! インデックスバッファです。最初に描くときに静的なバッファを作ります（書き換えたら作り直す）。
    //------------------------------------------------------------------------
    class IndexBuffer final : public Effekseer::Backend::IndexBuffer {
    public:
        //! 作ります。
        //! @param  [in] elementCount インデックスの数
        //! @param  [in] initialData  最初の中身（無ければ nullptr）
        //! @param  [in] stride       1つの大きさ（2 か 4 バイト）
        IndexBuffer(int32_t elementCount, const void* initialData, Effekseer::Backend::IndexBufferStrideType stride);

        //! bgfx のバッファを破棄します。
        ~IndexBuffer() override;

        //! 中身を書き換えます。
        //! @param  [in] src    書く中身
        //! @param  [in] size   書くバイト数
        //! @param  [in] offset 書く先の先頭（バイト）
        void UpdateData(const void* src, int32_t size, int32_t offset) override;

        //! 次の描画のインデックスとして bgfx へ渡します。
        //! @param  [in] indexCount 使うインデックスの数（先頭から）
        //! @return 渡せたら true
        [[nodiscard]]
        bool Bind(u32 indexCount);

    private:
        //! bgfx のバッファを破棄します。
        void DestroyHandle();

        std::vector<u8>         m_data;                            // 中身（CPU 側）
        bgfx::IndexBufferHandle m_handle = BGFX_INVALID_HANDLE;    // bgfx のバッファ
    };

    //------------------------------------------------------------------------
    //! テクスチャです。自分で作ったもの（エフェクトの画像）と、Renderer が持つテクスチャを
    //! 借りて見せるもの（歪みの背景・ソフトパーティクルの深度）があります。
    //------------------------------------------------------------------------
    class Texture final : public Effekseer::Backend::Texture {
    public:
        //! bgfx のテクスチャを持たせて作ります。
        //! @param  [in] handle テクスチャ
        //! @param  [in] param  大きさと形式
        //! @param  [in] owned  true なら破棄のときに handle も破棄する
        Texture(bgfx::TextureHandle handle, const Effekseer::Backend::TextureParameter& param, bool owned);

        //! 持っているテクスチャを破棄します（自分で作ったものだけ）。
        ~Texture() override;

        //! bgfx のテクスチャを返します。
        [[nodiscard]]
        bgfx::TextureHandle GetHandle() const noexcept {
            return m_handle;
        }

        //! 借りているテクスチャを差し替えます（描く先が作り直されてハンドルが変わったとき）。
        //! @param  [in] handle 新しいテクスチャ
        //! @param  [in] width  幅
        //! @param  [in] height 高さ
        void SetBorrowedHandle(bgfx::TextureHandle handle, u32 width, u32 height);

    private:
        bgfx::TextureHandle m_handle = BGFX_INVALID_HANDLE;    // テクスチャ
        bool                m_owned  = false;                  // 自分で作ったか
    };

    //------------------------------------------------------------------------
    //! Effekseer の描画デバイスの bgfx 版です。
    //------------------------------------------------------------------------
    class GraphicsDevice final : public Effekseer::Backend::GraphicsDevice {
    public:
        //! 頂点バッファを作ります。
        Effekseer::Backend::VertexBufferRef CreateVertexBuffer(int32_t size, const void* initialData, bool isDynamic) override;

        //! インデックスバッファを作ります。
        Effekseer::Backend::IndexBufferRef CreateIndexBuffer(int32_t                                   elementCount,
                                                             const void*                               initialData,
                                                             Effekseer::Backend::IndexBufferStrideType stride) override;

        //! 頂点バッファの中身を書き換えます。
        bool UpdateVertexBuffer(Effekseer::Backend::VertexBufferRef& buffer, int32_t size, int32_t offset, const void* data) override;

        //! インデックスバッファの中身を書き換えます。
        bool UpdateIndexBuffer(Effekseer::Backend::IndexBufferRef& buffer, int32_t size, int32_t offset, const void* data) override;

        //! 頂点形式を作ります。
        Effekseer::Backend::VertexLayoutRef CreateVertexLayout(const Effekseer::Backend::VertexLayoutElement* elements, int32_t elementCount) override;

        //! テクスチャを作ります。
        //! @note  2D の画像だけ（配列・3D・描く先は作らない）。段の数が 0 のときは、8bit の形式なら
        //!        CPU で 1×1 までの段を作る（bgfx は普通のテクスチャの段を GPU で作れないため）
        Effekseer::Backend::TextureRef CreateTexture(const Effekseer::Backend::TextureParameter& param,
                                                     const Effekseer::CustomVector<uint8_t>&     initialData) override;

        //! 描画デバイスの名前を返します。
        std::string GetDeviceName() const override { return "bgfx"; }

        //! Renderer が持つテクスチャを借りて見せるテクスチャを作ります（破棄しない）。
        //! @param  [in] handle テクスチャ
        //! @param  [in] width  幅
        //! @param  [in] height 高さ
        //! @return テクスチャ
        [[nodiscard]]
        Effekseer::RefPtr<Texture> CreateBorrowedTexture(bgfx::TextureHandle handle, u32 width, u32 height);
    };

}    // namespace Tsukino::Renderer::EffekseerBgfx
