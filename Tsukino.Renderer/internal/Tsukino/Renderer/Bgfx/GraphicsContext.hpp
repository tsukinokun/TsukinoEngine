//----------------------------------------------------------------------------
//! @file   GraphicsContext.hpp
//! @brief  bgfx 版の GraphicsContext（Renderer の内部用）
//! @detail bgfx の初期化・画面サイズの変更・垂直同期・フレームの送り出しを担当します。
//!         Renderer が持ち、各パスへ貸します。
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
        //! @param  [in] deviceOptions デバイスの選び方
        //! @return true: 初期化成功, false: 初期化失敗
        [[nodiscard]]
        bool Initialize(void* windowHandle, u32 width, u32 height, const GraphicsDeviceOptions& deviceOptions);

        //! 描画領域の大きさを変えます。
        //! @param  [in] width  幅
        //! @param  [in] height 高さ
        //! @return true: 成功
        //! @note   3D を描く先も作り直すので、呼んだ後はビューの描く先を設定し直す
        bool Resize(u32 width, u32 height);

        //! 垂直同期の有無を設定します。
        //! @param  [in] enabled true で垂直同期あり
        void SetVSyncEnabled(bool enabled);

        //! 描画先のウィンドウを付け直し、バックバッファを作り直します。
        //! @param  [in] windowHandle 新しい OS のウィンドウ
        void SetWindowHandle(void* windowHandle);

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

        //! 内部解像度の倍率を設定し、3D を描く先を作り直します。
        //! @param  [in] scale 倍率（RenderQualitySettings::kMinRenderScale〜1 に収める）
        //! @return true: 成功（倍率が変わらなければ何もせず true）
        //! @note   描く先のハンドルが変わるので、呼んだ後はビューの描く先を設定し直す
        bool SetRenderScale(float scale);

        //! 3D を描く幅（描画領域の幅 × 内部解像度の倍率）を返します。
        //! @return 幅（ピクセル）
        [[nodiscard]]
        u32 GetSceneWidth() const noexcept {
            return m_sceneWidth;
        }

        //! 3D を描く高さ（描画領域の高さ × 内部解像度の倍率）を返します。
        //! @return 高さ（ピクセル）
        [[nodiscard]]
        u32 GetSceneHeight() const noexcept {
            return m_sceneHeight;
        }

        //! bgfx が初期化されていて、まだ後始末されていないかを返します。
        //! @return 資源を作ったり破棄したりしてよいなら true
        //! @note   MeshBuffer などは Renderer より長生きすることがある。bgfx を後始末した後に
        //!         bgfx::destroy を呼ぶと落ちるので、破棄する前にこれで確かめる
        [[nodiscard]]
        static bool IsBgfxAlive() noexcept;

        //------------------------------------------------------------------------
        // 3D を描く先（HDR バッファ・G-Buffer・深度）。
        // どれも GetSceneWidth() × GetSceneHeight() の大きさ。描画領域の大きさか内部解像度が変わると作り直す
        // （ハンドルが変わるので、使う側は毎フレーム取り直すか、作り直した後に設定し直す）
        //------------------------------------------------------------------------

        //! G-Buffer の枚数（6枚）
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

        //! ポストプロセス用の中間バッファのテクスチャを返します（モーションブラーが HDR を読んで書く先）。
        //! @return テクスチャ
        [[nodiscard]]
        bgfx::TextureHandle GetPostProcessTexture() const noexcept {
            return m_postProcessTexture;
        }

        //! ポストプロセス用の中間バッファだけのフレームバッファを返します。
        //! @return フレームバッファ
        [[nodiscard]]
        bgfx::FrameBufferHandle GetPostProcessFrameBuffer() const noexcept {
            return m_postProcessFrameBuffer;
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

        //------------------------------------------------------------------------
        // エフェクト（Effekseer）を描く先。Renderer が Effekseer を使い始めたときだけ作る（EnableEffectTargets）。
        // トーンマップした絵（LDR）を 3D の深度と組にして、その上にエフェクトを重ね、最後にバックバッファへ写す。
        // 大きさは 3D を描く先と同じで、一緒に作り直す
        //------------------------------------------------------------------------

        //! エフェクトを描く先を作るようにします（2回目以降は何もしない）。
        //! @return true: 作れた（描画領域がまだ無いときは、作るのを後に回して true）
        [[nodiscard]]
        bool EnableEffectTargets();

        //! エフェクトを描く先があるかを返します。
        [[nodiscard]]
        bool HasEffectTargets() const noexcept {
            return bgfx::isValid(m_effectFrameBuffer);
        }

        //! トーンマップした絵と 3D の深度のフレームバッファを返します（トーンマップとエフェクトの描く先）。
        [[nodiscard]]
        bgfx::FrameBufferHandle GetEffectFrameBuffer() const noexcept {
            return m_effectFrameBuffer;
        }

        //! トーンマップした絵のテクスチャを返します（RGBA8。最後にバックバッファへ写す）。
        [[nodiscard]]
        bgfx::TextureHandle GetEffectColorTexture() const noexcept {
            return m_effectColorTexture;
        }

        //! 歪みが読む背景のテクスチャを返します（エフェクトを描く前の絵の写し）。
        [[nodiscard]]
        bgfx::TextureHandle GetEffectBackgroundTexture() const noexcept {
            return m_effectBackgroundTexture;
        }

        //! ソフトパーティクルが読む深度のテクスチャを返します（3D の深度の写し。描く先の深度は読めないので写す）。
        [[nodiscard]]
        bgfx::TextureHandle GetEffectDepthTexture() const noexcept {
            return m_effectDepthTexture;
        }

    private:
        //! 使っている GPU の情報を調べて保持し、ログへ出します。
        //! @note   Windows（Direct3D 11）は DXGI から読む。それ以外は bgfx が持つ ID と描画 API の名前だけ
        void QueryAdapterInfo();

        //! 今の大きさと垂直同期の設定で、bgfx のバックバッファを作り直します。
        void ApplyReset();

        //! 3D を描く先のテクスチャとフレームバッファを、今の描画領域と内部解像度の大きさで作ります。
        //! @return true: 作成成功
        bool CreateRenderTargets();

        //! 3D を描く大きさが変わっていれば、描く先を作り直します。
        //! @return true: 作り直さなくてよかったか、作り直せた
        bool RecreateRenderTargetsIfNeeded();

        //! 3D を描く先を破棄します。
        void DestroyRenderTargets();

        bgfx::TextureHandle m_hdrTexture                     = BGFX_INVALID_HANDLE;    // HDR バッファ（RGBA16F）
        bgfx::TextureHandle m_depthTexture                   = BGFX_INVALID_HANDLE;    // 深度（D32F。ライティングで読む）
        bgfx::TextureHandle m_gbufferTextures[kGBufferCount] = {
            BGFX_INVALID_HANDLE, BGFX_INVALID_HANDLE, BGFX_INVALID_HANDLE, BGFX_INVALID_HANDLE, BGFX_INVALID_HANDLE, BGFX_INVALID_HANDLE};
        bgfx::FrameBufferHandle m_gbufferFrameBuffer      = BGFX_INVALID_HANDLE;    // G-Buffer ＋ 深度
        bgfx::FrameBufferHandle m_hdrFrameBuffer          = BGFX_INVALID_HANDLE;    // HDR ＋ 深度
        bgfx::FrameBufferHandle m_hdrColorFrameBuffer     = BGFX_INVALID_HANDLE;    // HDR だけ
        bgfx::TextureHandle     m_postProcessTexture      = BGFX_INVALID_HANDLE;    // ポストプロセス用の中間バッファ（RGBA16F）
        bgfx::FrameBufferHandle m_postProcessFrameBuffer  = BGFX_INVALID_HANDLE;    // 中間バッファだけ
        bgfx::TextureHandle     m_effectColorTexture      = BGFX_INVALID_HANDLE;    // トーンマップした絵（RGBA8）
        bgfx::FrameBufferHandle m_effectFrameBuffer       = BGFX_INVALID_HANDLE;    // トーンマップした絵 ＋ 3D の深度
        bgfx::TextureHandle     m_effectBackgroundTexture = BGFX_INVALID_HANDLE;    // 歪みの背景（トーンマップした絵の写し）
        bgfx::TextureHandle     m_effectDepthTexture      = BGFX_INVALID_HANDLE;    // ソフトパーティクルの深度（3D の深度の写し）
        bool                    m_effectTargetsEnabled    = false;                  // エフェクトを描く先を作るか

        bool                m_initialized  = false;      // bgfx を初期化したか
        bool                m_vsync        = true;       // 垂直同期をするか
        void*               m_windowHandle = nullptr;    // 描画先のウィンドウ（作り直すときにも渡す）
        u32                 m_width        = 0;          // 描画領域の幅
        u32                 m_height       = 0;          // 描画領域の高さ
        float               m_renderScale  = 1.0f;       // 内部解像度の倍率
        u32                 m_sceneWidth   = 0;          // 3D を描く幅（描く先を作ったときの大きさ）
        u32                 m_sceneHeight  = 0;          // 3D を描く高さ
        GraphicsAdapterInfo m_adapterInfo;               // 使っている GPU の情報
    };

}    // namespace Tsukino::Renderer
