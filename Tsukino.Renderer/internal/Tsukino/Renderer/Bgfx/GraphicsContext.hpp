//----------------------------------------------------------------------------
//! @file   GraphicsContext.hpp
//! @brief  bgfx 版の GraphicsContext（Renderer の内部用）
//! @detail bgfx の初期化・画面サイズの変更・垂直同期・フレームの送り出しを担当します。
//!         DX11 版の GraphicsContext と同じ名前で、Renderer から借りられる形にしてあります。
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/Core/typedef.hpp>
#include <Tsukino/Renderer/RenderQuality.hpp>

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

    private:
        //! 今の大きさと垂直同期の設定で、bgfx のバックバッファを作り直します。
        void ApplyReset();

        bool                m_initialized = false;    // bgfx を初期化したか
        bool                m_vsync       = true;     // 垂直同期をするか
        void*               m_windowHandle = nullptr; // 描画先のウィンドウ（作り直すときにも渡す）
        u32                 m_width       = 0;        // 描画領域の幅
        u32                 m_height      = 0;        // 描画領域の高さ
        GraphicsAdapterInfo m_adapterInfo;            // 使っている GPU の情報
    };

}    // namespace Tsukino::Renderer
