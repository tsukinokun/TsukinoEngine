//----------------------------------------------------------------------------
//! @file   GraphicsContext.hpp
//! @brief  bgfx 版の GraphicsContext（Renderer の内部用）
//! @detail bgfx の初期化・画面サイズの変更・垂直同期・フレームの送り出しを担当します。
//!         DX11 版の GraphicsContext と同じ名前で、Renderer から借りられる形にしてあります。
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/Core/typedef.hpp>
#include <Tsukino/Renderer/RenderQuality.hpp>

#include <bgfx/bgfx.h>

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {

    //------------------------------------------------------------------------
    //! bgfx の初期化と後始末を持つクラスです。
    //------------------------------------------------------------------------
    class GraphicsContext {
    public:
        //! bgfx を後始末します（初期化していなければ何もしません）。
        ~GraphicsContext();

        //! bgfx を初期化します。
        //! @param  [in] windowHandle  描画先のウィンドウ（Windows では HWND）
        //! @param  [in] width         描画領域の幅
        //! @param  [in] height        描画領域の高さ
        //! @param  [in] deviceOptions デバイスの選び方（bgfx 版ではまだ使わない）
        //! @return true: 初期化成功, false: 初期化失敗
        [[nodiscard]]
        bool Initialize(void* windowHandle, u32 width, u32 height, const GraphicsDeviceOptions& deviceOptions);

        //! 描画領域の大きさを変えます。
        //! @param  [in] width  幅
        //! @param  [in] height 高さ
        //! @return true: 成功
        bool Resize(u32 width, u32 height);

        //! 垂直同期の有無を設定します。
        //! @param  [in] enabled true で垂直同期あり
        void SetVSyncEnabled(bool enabled);

        //! 垂直同期が有効かを返します。
        //! @return 有効なら true
        [[nodiscard]]
        bool IsVSyncEnabled() const noexcept {
            return m_vsync;
        }

        //! 使っている GPU の情報を返します。
        //! @return GPU の情報
        [[nodiscard]]
        const GraphicsAdapterInfo& GetAdapterInfo() const noexcept {
            return m_adapterInfo;
        }

        //! 描画領域の幅を返します。
        //! @return 幅（ピクセル）
        [[nodiscard]]
        u32 GetWidth() const noexcept {
            return m_width;
        }

        //! 描画領域の高さを返します。
        //! @return 高さ（ピクセル）
        [[nodiscard]]
        u32 GetHeight() const noexcept {
            return m_height;
        }

        //! bgfx が初期化されていて、まだ後始末されていないかを返します。
        //! @return 資源を作ったり破棄したりしてよいなら true
        //! @note   MeshBuffer などは Renderer より長生きすることがある。bgfx を後始末した後に
        //!         bgfx::destroy を呼ぶと落ちるので、破棄する前にこれで確かめる
        [[nodiscard]]
        static bool IsBgfxAlive() noexcept;

        //------------------------------------------------------------------------
        // 3D を描く先（DX11 版の GraphicsContext が持つ HDR バッファ・G-Buffer・深度と同じ組み）。
        // どれもバックバッファと同じ大きさで作り、bgfx が画面の大きさの変更に合わせて作り直す
        //------------------------------------------------------------------------

        //! G-Buffer の枚数（DX11 版と同じ6枚）
        static constexpr u32 kGBufferCount = 6;

        //! G-Buffer（6枚）と深度のフレームバッファを返します。
        //! @return フレームバッファ
        [[nodiscard]]
        bgfx::FrameBufferHandle GetGBufferFrameBuffer() const noexcept {
            return m_gbufferFrameBuffer;
        }

        //! HDR バッファと深度のフレームバッファを返します（フォワードの 3D を描く先）。
        //! @return フレームバッファ
        [[nodiscard]]
        bgfx::FrameBufferHandle GetHdrFrameBuffer() const noexcept {
            return m_hdrFrameBuffer;
        }

        //! HDR バッファだけのフレームバッファを返します（深度を読みながら描くライティング用）。
        //! @return フレームバッファ
        [[nodiscard]]
        bgfx::FrameBufferHandle GetHdrColorFrameBuffer() const noexcept {
            return m_hdrColorFrameBuffer;
        }

        //! HDR バッファのテクスチャを返します。
        //! @return テクスチャ
        [[nodiscard]]
        bgfx::TextureHandle GetHdrTexture() const noexcept {
            return m_hdrTexture;
        }

        //! G-Buffer の1枚のテクスチャを返します。
        //! @param  [in] index 0〜5（0: albedo, 1: 法線, 2: 材質, 3: 発光, 4: ワールド座標, 5: 速度）
        //! @return テクスチャ
        [[nodiscard]]
        bgfx::TextureHandle GetGBufferTexture(u32 index) const noexcept {
            return index < kGBufferCount ? m_gbufferTextures[index] : bgfx::TextureHandle BGFX_INVALID_HANDLE;
        }

        //! 3D の深度のテクスチャを返します（リバース Z。何も描かれていない所は 0）。
        //! @return テクスチャ
        [[nodiscard]]
        bgfx::TextureHandle GetDepthTexture() const noexcept {
            return m_depthTexture;
        }

    private:
        //! 今の大きさと垂直同期の設定で、bgfx のバックバッファを作り直します。
        void ApplyReset();

        //! 3D を描く先のテクスチャとフレームバッファを作ります。
        //! @return true: 作成成功
        bool CreateRenderTargets();

        //! 3D を描く先を破棄します。
        void DestroyRenderTargets();

        bgfx::TextureHandle                    m_hdrTexture   = BGFX_INVALID_HANDLE;    // HDR バッファ（RGBA16F）
        bgfx::TextureHandle                    m_depthTexture = BGFX_INVALID_HANDLE;    // 深度（D32F。ライティングで読む）
        bgfx::TextureHandle                    m_gbufferTextures[kGBufferCount] = {BGFX_INVALID_HANDLE, BGFX_INVALID_HANDLE, BGFX_INVALID_HANDLE,
                                                                                 BGFX_INVALID_HANDLE, BGFX_INVALID_HANDLE, BGFX_INVALID_HANDLE};
        bgfx::FrameBufferHandle                m_gbufferFrameBuffer  = BGFX_INVALID_HANDLE;    // G-Buffer ＋ 深度
        bgfx::FrameBufferHandle                m_hdrFrameBuffer      = BGFX_INVALID_HANDLE;    // HDR ＋ 深度
        bgfx::FrameBufferHandle                m_hdrColorFrameBuffer = BGFX_INVALID_HANDLE;    // HDR だけ

        bool                m_initialized = false;    // bgfx を初期化したか
        bool                m_vsync       = true;     // 垂直同期をするか
        void*               m_windowHandle = nullptr; // 描画先のウィンドウ（作り直すときにも渡す）
        u32                 m_width       = 0;        // 描画領域の幅
        u32                 m_height      = 0;        // 描画領域の高さ
        GraphicsAdapterInfo m_adapterInfo;            // 使っている GPU の情報
    };

}    // namespace Tsukino::Renderer
