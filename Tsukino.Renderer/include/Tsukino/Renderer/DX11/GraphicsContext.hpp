//--------------------------------------------------------------
//! @file   GraphicsContext.hpp
//! @brief  DirectX11のグラフィックスコンテキストクラスの宣言
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once

#include <d3d11.h>
#include <dxgi.h>
#include <wrl/client.h>
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

    private:
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
        Microsoft::WRL::ComPtr<ID3D11DepthStencilView> m_dsv;
    };

}    // namespace Tsukino::Renderer
