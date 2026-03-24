//------------------------------------------------------------
//! @file	Renderer.hpp
//! @brief	レンダラークラスの宣言
//! @author 山﨑愛
//------------------------------------------------------------
#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <Tsukino/Renderer/DX11/GraphicsContext.hpp>
#include <Tsukino/Renderer/DX11/MeshBuffer.hpp>
#include <Tsukino/Renderer/DX11/PipelineFactory.hpp>  
#include <Tsukino/Renderer/SpriteRenderer.hpp>
#include <Tsukino/Renderer/DrawCommandQueue.hpp>

#include <Tsukino/GraphicsCommon/Mesh/PrimitiveType.hpp>
#include <Tsukino/GraphicsCommon/Mesh/MeshData.hpp>

#include <wrl/client.h>    // ComPtrの依存関係を明示
#include <d3d11.h>         // 依存関係を明示
#include <dxgi.h>          // 依存関係を明示
#include <array>
#include <optional>

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

        //------------------------------------------------------------
        // 描画コマンドの追加
        //! @param cmd [in] 追加する描画コマンド
        //------------------------------------------------------------
        void PushDrawCommand(const DrawCommand& cmd);

        //------------------------------------------------------------
        // PipelineFactoryを使うためのGetterを公開
        //! @return PipelineFactoryのポインタ
        //------------------------------------------------------------
        [[nodiscard]]
        PipelineFactory* GetPipelineFactory() {
            return &m_pipelineFactory.value();
        }

    private:
        //------------------------------------------------------------
        // 定数バッファの作成
        //! @return true: 定数バッファの作成成功, false: 定数バッファの作成失敗
        //------------------------------------------------------------
        [[nodiscard]]
        bool CreateConstantBuffer();

        //------------------------------------------------------------
        // プリミティブメッシュの作成
        //! @return true: プリミティブメッシュの作成成功, false:作成失敗
        //------------------------------------------------------------
        [[nodiscard]]
        bool CreatePrimitiveMeshes();

        //------------------------------------------------------------
        // 描画コマンドを実行
        //! @param cmd [in] 実行する描画コマンド
        //------------------------------------------------------------
        void ExecuteDrawCommand(const DrawCommand& cmd);

        //------------------------------------------------------------
        // プリミティブメッシュの取得
        //! @param type [in] 取得するプリミティブの種類
        //! @return メッシュバッファへのポインタ
        //------------------------------------------------------------
        [[nodiscard]]
        MeshBuffer* GetPrimitiveMesh(Tsukino::GraphicsCommon::PrimitiveType type) {
            return &m_primitiveMeshes[static_cast<size_t>(type)];
        }

    private:
        // DirectX 11の主要なインターフェース
        GraphicsContext            m_graphicsContext;                          // グラフィックスコンテキスト（Device, DeviceContext, SwapChainを管理）
        ComPtr<ID3D11Buffer>       m_vertexBuffer;                             // 頂点バッファ
        ComPtr<ID3D11VertexShader> m_vertexShader;                             // 頂点シェーダ
        ComPtr<ID3D11PixelShader>  m_pixelShader;                              // ピクセルシェーダ
        ComPtr<ID3D11InputLayout>  m_inputLayout;                              // 入力レイアウト
        ComPtr<ID3D11Buffer>       m_constantBuffer;                           // 定数バッファ
        std::array<float, 4>       m_clearColor = {0.5f, 0.5f, 0.5f, 1.0f};    // 描画領域のクリアカラー (デフォルトはグレー)

        std::array<MeshBuffer, (size_t)Tsukino::GraphicsCommon::PrimitiveType::Count> m_primitiveMeshes;    // プリミティブメッシュバッファの配列
        std::optional<PipelineFactory>                                                m_pipelineFactory;    // メンバとして持たせる
        SpriteRenderer                                                                m_spriteRenderer;     // スプライト描画クラスのインスタンス
        DrawCommandQueue                                                              m_drawQueue;          // 描画コマンドキュー
    };
}    // namespace Tsukino::Renderer
