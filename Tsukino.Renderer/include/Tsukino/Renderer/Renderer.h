//------------------------------------------------------------
//! @file	Renderer.h
//! @brief	レンダラークラスの宣言
//! @author 山﨑愛
//------------------------------------------------------------
#pragma once
#include <wrl/client.h>    // ComPtrの依存関係を明示
#include <d3d11.h>         // 依存関係を明示
#include <dxgi.h>          // 依存関係を明示
#include <array>           // std::array を使用するためのヘッダーファイル、明示的に書く

using Microsoft::WRL::ComPtr;    // ComPtr を使用するための using 宣言

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {

    //------------------------------------------------------------
    //! @class	 Renderer
    //! @brief	 レンダラークラス
    //! @details DirectX11を使用してウィンドウに描画を行うクラス
    //------------------------------------------------------------
    class Renderer {
    public:
        //------------------------------------------------------------
        // コンストラクタ
        //------------------------------------------------------------
        Renderer() = default;

        //------------------------------------------------------------
        // デストラクタ
        //------------------------------------------------------------
        ~Renderer() = default;

        //------------------------------------------------------------
        // レンダラーの初期化
        //! @param hwnd 描画先のウィンドウハンドル
        //! @param width 描画領域の幅
        //! @param height 描画領域の高さ
        //! @return true: 初期化成功, false: 初期化失敗
        //------------------------------------------------------------
        bool Initialize(HWND hwnd, uint32_t width, uint32_t height);

        //------------------------------------------------------------
        // 描画処理
        //------------------------------------------------------------
        void Render();

        //------------------------------------------------------------
        // 描画領域のクリアカラーを設定
        //! @param r 赤成分 (0.0f - 1.0f)
        //! @param g 緑成分 (0.0f - 1.0
        //! @param b 青成分 (0.0f - 1.0f)
        //! @param a アルファ成分 (0.0f - 1.0f)
        //------------------------------------------------------------
        void SetClearColor(float r, float g, float b, float a);

    private:
        // DirectX 11の主要なインターフェース
        ComPtr<ID3D11Device>           m_device     = nullptr;    // 描画デバイス
        ComPtr<ID3D11DeviceContext>    m_context    = nullptr;    // 描画コンテキスト
        ComPtr<IDXGISwapChain>         m_swapChain  = nullptr;    // スワップチェーン
        ComPtr<ID3D11RenderTargetView> m_rtv        = nullptr;    // レンダーターゲットビュー
        std::array<float, 4>           m_clearColor = {0.5f, 0.5f, 0.5f, 1.0f};
    };
}    // namespace Tsukino::Renderer
