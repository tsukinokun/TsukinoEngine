//--------------------------------------------------------------
//! @file   GraphicsContext.hpp
//! @brief  DirectX11のグラフィックスコンテキストクラスの宣言
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once

#include <d3d11.h>
#include <dxgi.h>
#include <wrl/client.h>
#include <array>
// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {
    struct PipelineState;    // 前方宣言
    class Material;          // 前方宣言

    //--------------------------------------------------------------
    //! @class  GraphicsContext
    //! @brief  DirectX11のグラフィックスコンテキストクラス
    //--------------------------------------------------------------
    class GraphicsContext {
    public:
        //--------------------------------------------------------------
        // 初期化関数
        //! @param hwnd   [in] ウィンドウハンドル
        //! @param width  [in] 描画領域の幅（ピクセル)
        //--------------------------------------------------------------
        [[nodiscard]]
        bool Initialize(HWND hwnd, UINT width, UINT height);

        //--------------------------------------------------------------
        // フレームの開始処理
        //! @param r [in] 赤成分（0.0f～1.0f）
        //! @param g [in] 緑成分（0.0f～1.0f）
        //! @param b [in] 青成分（0.0f～1.0f）
        //! @param a [in] アルファ成分（0.0f～1.0f）
        //--------------------------------------------------------------
        void BeginFrame(float r, float g, float b, float a);

        //--------------------------------------------------------------
        //! @brief  フレームの終了処理
        //! @param hwnd [in] ウィンドウハンドル
        //! @param syncInterval [in] 垂直同期の有無（0: 無効、1: 有効）
        //--------------------------------------------------------------
        void EndFrame();

        //--------------------------------------------------------------
        //! @brief  DirectX Device取得
        //! @return DirectX Deviceオブジェクトへのポインタ
        //--------------------------------------------------------------
        [[nodiscard]]
        ID3D11Device* GetDevice() const noexcept {
            return m_device.Get();
        }

        //--------------------------------------------------------------
        //! @brief  DirectX DeviceContext取得
        //! @return DirectX DeviceContextオブジェクトへのポインタ
        //--------------------------------------------------------------
        [[nodiscard]]
        ID3D11DeviceContext* GetContext() const noexcept {
            return m_context.Get();
        }

        //--------------------------------------------------------------
        // パイプラインステートをセット
        //! @param state [in] セットするパイプラインステート
        //--------------------------------------------------------------
        void SetPipelineState(const PipelineState& state);

        //--------------------------------------------------------------
        // マテリアルをセット
        //! @param mat [in] セットするマテリアル
        //--------------------------------------------------------------
        void SetMaterial(const Material& mat);

         //--------------------------------------------------------------
        //! @brief HDRレンダーターゲットのSRVを取得（トーンマッピングで使用）
        //! @return ID3D11ShaderResourceViewへのポインタ
        //--------------------------------------------------------------
        [[nodiscard]]
        ID3D11ShaderResourceView* GetHDRSRV() const noexcept {
            return m_hdrSRV.Get();
        }

        //--------------------------------------------------------------
        //! @brief バックバッファ（スワップチェイン）のRTVにバインドする
        //! @note  トーンマッピングパスの直前に呼び出す
        //--------------------------------------------------------------
        void BindBackBuffer();

        //--------------------------------------------------------------
        //! @brief G-Buffer（4枚）とDSVをバインドしてクリアする
        //! @note  GBufferパスの先頭で呼ぶ
        //--------------------------------------------------------------
        void BeginGBufferPass();

        //--------------------------------------------------------------
        //! @brief HDRバッファへ戻す（クリアはしない）
        //! @note  Lightingパス完了後、World/Transparent/Waterパスの前に呼ぶ
        //--------------------------------------------------------------
        void BindHDRRenderTarget();

        //--------------------------------------------------------------
        //! @brief HDRバッファのみをRTVにバインドする（DSVなし）
        //! @note  ディファードLightingパス用。深度をSRVとして読む間は
        //!        同じリソースをDSVとして同時バインドできないため。
        //--------------------------------------------------------------
        void BindHDRTargetOnly();

        //--------------------------------------------------------------
        //! @brief G-BufferのSRVを取得する
        //! @param index [in] 0=Albedo, 1=Normal, 2=Material, 3=Emissive, 4=WorldPos
        //! @return ID3D11ShaderResourceViewへのポインタ
        //--------------------------------------------------------------
        [[nodiscard]]
        ID3D11ShaderResourceView* GetGBufferSRV(UINT index) const noexcept {
            return (index < GBufferCount) ? m_gbufferSRV[index].Get() : nullptr;
        }

        //--------------------------------------------------------------
        //! @brief 深度バッファのSRVを取得する（ディファードLightingパスで使用）
        //! @return ID3D11ShaderResourceViewへのポインタ
        //--------------------------------------------------------------
        [[nodiscard]]
        ID3D11ShaderResourceView* GetDepthSRV() const noexcept {
            return m_depthSRV.Get();
        }

        //--------------------------------------------------------------
        //! @brief G-Bufferの枚数
        //--------------------------------------------------------------
        static constexpr UINT GBufferCount = 5;

        //--------------------------------------------------------------
        //! @brief  描画領域のリサイズ
        //! @param  width  [in] 新しい幅（ピクセル）
        //! @param  height [in] 新しい高さ（ピクセル）
        //! @return true: 成功, false: 失敗
        //! @details
        //! スワップチェインのバックバッファと、画面サイズに依存する
        //! すべてのリソース（RTV / DSV / HDRバッファ）を作り直す。
        //! ResizeBuffers はバックバッファへの参照が1つでも残っていると
        //! 失敗するため、先に全ビューを解放してから呼ぶ必要がある。
        //--------------------------------------------------------------
        [[nodiscard]]
        bool Resize(UINT width, UINT height);

        //--------------------------------------------------------------
        //! @brief  現在の描画領域の幅を取得する
        //--------------------------------------------------------------
        [[nodiscard]]
        UINT GetWidth() const noexcept {
            return m_width;
        }

        //--------------------------------------------------------------
        //! @brief  現在の描画領域の高さを取得する
        //--------------------------------------------------------------
        [[nodiscard]]
        UINT GetHeight() const noexcept {
            return m_height;
        }

    private:
        //--------------------------------------------------------------
        //! @brief  画面サイズに依存するリソースを生成する
        //! @param  width  [in] 幅（ピクセル）
        //! @param  height [in] 高さ（ピクセル）
        //! @return true: 成功, false: 失敗
        //! @note   Initialize と Resize の両方から呼ばれる。
        //!         生成手順を1箇所に集約することで、リサイズ時の
        //!         作り忘れ・設定違いを防ぐ。
        //--------------------------------------------------------------
        [[nodiscard]]
        bool CreateSizeDependentResources(UINT width, UINT height);

        //--------------------------------------------------------------
        //! @brief  画面サイズに依存するリソースを解放する
        //! @note   ResizeBuffers の前にバックバッファへの参照を
        //!         すべて手放すために使う
        //--------------------------------------------------------------
        void ReleaseSizeDependentResources();

        Microsoft::WRL::ComPtr<ID3D11Device> m_device;

        // DirectX DeviceContext
        // GPUへの描画コマンドを送る
        Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_context;

        // SwapChain
        // BackBufferと画面表示を管理
        Microsoft::WRL::ComPtr<IDXGISwapChain> m_swapChain;

        // RenderTarget
        // 描画先のバックバッファ
        Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_rtv;

        // DepthStencilView
        Microsoft::WRL::ComPtr<ID3D11DepthStencilView>   m_dsv;
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_depthSRV;    //!< 深度のSRVビュー（ディファードLightingパス用）

        // HDRレンダーターゲット（通常描画の出力先）
        Microsoft::WRL::ComPtr<ID3D11Texture2D>          m_hdrTex;    //!< HDRカラーバッファ
        Microsoft::WRL::ComPtr<ID3D11RenderTargetView>   m_hdrRTV;    //!< HDR用RTV
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_hdrSRV;    //!< トーンマッピングで読むSRV

        // G-Buffer（ディファードGBufferパスの出力 / Lightingパスの入力）
        std::array<Microsoft::WRL::ComPtr<ID3D11Texture2D>, GBufferCount>          m_gbufferTex;
        std::array<Microsoft::WRL::ComPtr<ID3D11RenderTargetView>, GBufferCount>   m_gbufferRTV;
        std::array<Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>, GBufferCount> m_gbufferSRV;

        UINT m_width  = 0;    //!< 描画領域の幅
        UINT m_height = 0;    //!< 描画領域の高さ

    };

}    // namespace Tsukino::Renderer
