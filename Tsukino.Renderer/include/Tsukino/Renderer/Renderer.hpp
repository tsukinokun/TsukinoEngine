//------------------------------------------------------------
//! @file	Renderer.hpp
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
        //! @brief コンストラクタ
        //------------------------------------------------------------
        Renderer() = default;

        //------------------------------------------------------------
        //! @brief デストラクタ
        //------------------------------------------------------------
        ~Renderer() = default;

        //------------------------------------------------------------
        // レンダラーの初期化
        //! @param hwnd   [in] 描画先のウィンドウハンドル
        //! @param width  [in] 描画領域の幅
        //! @param height [in] 描画領域の高さ
        //! @return true: [in] 初期化成功, false: 初期化失敗
        //------------------------------------------------------------
        [[nodiscard]]
        bool Initialize(HWND hwnd, uint32_t width, uint32_t height);

        //------------------------------------------------------------
        // 定数バッファの作成
        //! @return true: 定数バッファの作成成功, false: 定数バッファの作成失敗
        //------------------------------------------------------------
        [[nodiscard]]
        bool CreateConstantBuffer();

        //------------------------------------------------------------
        // 描画処理
        //------------------------------------------------------------
        void Render();

        //------------------------------------------------------------
        // 描画領域のクリアカラーを設定
        //! @param r [in] 赤成分 (0.0f - 1.0f)
        //! @param g [in] 緑成分 (0.0f - 1.0
        //! @param b [in] 青成分 (0.0f - 1.0f)
        //! @param a [in] アルファ成分 (0.0f - 1.0f)
        //------------------------------------------------------------
        void SetClearColor(float r, float g, float b, float a);

    private:
        // DirectX 11の主要なインターフェース
        ComPtr<ID3D11Device>           m_device;            // 描画デバイス
        ComPtr<ID3D11DeviceContext>    m_context;           // 描画コンテキスト
        ComPtr<IDXGISwapChain>         m_swapChain;         // スワップチェーン
        ComPtr<ID3D11RenderTargetView> m_rtv;               // レンダーターゲットビュー
        ComPtr<ID3D11Buffer>           m_vertexBuffer;      // 頂点バッファ
        ComPtr<ID3D11VertexShader>     m_vertexShader;      // 頂点シェーダ
        ComPtr<ID3D11PixelShader>      m_pixelShader;       // ピクセルシェーダ
        ComPtr<ID3D11InputLayout>      m_inputLayout;       // 入力レイアウト
        ComPtr<ID3D11Buffer>           m_constantBuffer;    // 定数バッファ
        std::array<float, 4>           m_clearColor = {0.5f, 0.5f, 0.5f, 1.0f};
    };
}    // namespace Tsukino::Renderer
