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
#include <Tsukino/Renderer/DX11/Texture/DX11Texture2D.hpp>

#include <Tsukino/GraphicsCommon/Mesh/PrimitiveType.hpp>
#include <Tsukino/GraphicsCommon/Mesh/MeshData.hpp>
#include <Tsukino/GraphicsCommon/State/SamplerType.hpp>

#include <wrl/client.h>    // ComPtrの依存関係を明示
#include <d3d11.h>         // 依存関係を明示
#include <dxgi.h>          // 依存関係を明示
#include <SpriteFont.h>
#include <CommonStates.h>

#include <array>
#include <optional>
#include <unordered_map>

using Microsoft::WRL::ComPtr;    // ComPtr を使用するための using 宣言

namespace Tsukino::Asset {
    class TextureAsset;    // 前方宣言
}

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {
    struct CBufferScene;    // 前方宣言
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

        //------------------------------------------------------------
        // プリミティブメッシュの取得
        //! @param type [in] 取得するプリミティブの種類
        //! @return メッシュバッファへのポインタ
        //------------------------------------------------------------
        [[nodiscard]]
        MeshBuffer* GetPrimitiveMesh(Tsukino::GraphicsCommon::PrimitiveType type) {
            return &m_primitiveMeshes[static_cast<size_t>(type)];
        }

        //------------------------------------------------------------
        // サンプラーの取得
        //! @param type [in] 取得するサンプラーの種類
        //! @return サンプラーステートへのポインタ
        //------------------------------------------------------------
        [[nodiscard]]
        ID3D11SamplerState* GetSampler(Tsukino::GraphicsCommon::SamplerType type) const {
            return m_samplers[static_cast<size_t>(type)].Get();
        }

        //------------------------------------------------------------
        // テクスチャ（SRV）の取得（なければ生成してキャッシュ）
        //! @param textureAsset [in] 取得元のテクスチャアセット
        //! @return ID3D11ShaderResourceView へのポインタ
        //------------------------------------------------------------
        [[nodiscard]]
        ID3D11ShaderResourceView* GetTextureSRV(const Tsukino::Asset::TextureAsset& textureAsset);

        //------------------------------------------------------------
        // シーン定数バッファの更新
        //! @param sceneData [in] シーン定数バッファの値データ
        //------------------------------------------------------------
        void UpdateSceneBuffer(const CBufferScene& sceneData);

        //------------------------------------------------------------
        // スプライトフォントの作成
        //! @param  data [in] フォントデータのバイナリ
        //! @param  size [in] フォントデータのサイズ
        //! @return SpriteFontのユニークポインタ
        //------------------------------------------------------------
        std::unique_ptr<DirectX::SpriteFont> CreateSpriteFont(const uint8_t* data, size_t size);

        //------------------------------------------------------------
        // スプライトバッチの作成
        //! @return SpriteBatchのユニークポインタ
        //------------------------------------------------------------
        std::unique_ptr<DirectX::SpriteBatch> CreateSpriteBatch();

        //------------------------------------------------------------
        //! @brief 共通ステートの取得
        //------------------------------------------------------------
        DirectX::CommonStates* GetCommonStatesTK() const { return m_commonStatesTK.get(); }

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
        // 共通ステート（サンプラーなど）の作成
        //! @return true: 作成成功, false: 作成失敗
        //------------------------------------------------------------
        [[nodiscard]]
        bool CreateCommonStates();

    private:
        // DirectX 11の主要なインターフェース
        GraphicsContext            m_graphicsContext;    // グラフィックスコンテキスト（Device, DeviceContext, SwapChainを管理）
        ComPtr<ID3D11Buffer>       m_vertexBuffer;       // 頂点バッファ
        ComPtr<ID3D11VertexShader> m_vertexShader;       // 頂点シェーダ
        ComPtr<ID3D11PixelShader>  m_pixelShader;        // ピクセルシェーダ
        ComPtr<ID3D11InputLayout>  m_inputLayout;        // 入力レイアウト

        // 定数バッファ
        ComPtr<ID3D11Buffer> m_objectBuffer;    // オブジェクトデータ用定数バッファ
        ComPtr<ID3D11Buffer> m_sceneBuffer;     // シーンデータ用定数バッファ

        std::array<float, 4> m_clearColor = {0.5f, 0.5f, 0.5f, 1.0f};    // 描画領域のクリアカラー (デフォルトはグレー)

        std::array<MeshBuffer, (size_t)Tsukino::GraphicsCommon::PrimitiveType::Count> m_primitiveMeshes;    // プリミティブメッシュバッファの配列
        std::array<ComPtr<ID3D11SamplerState>, static_cast<size_t>(Tsukino::GraphicsCommon::SamplerType::Count)> m_samplers;
        std::unordered_map<u64, std::unique_ptr<DX11Texture2D>> m_textureCache;       // Textureのキャッシュ (AssetHandle の Value(uint64_t) をキーにする)
        std::optional<PipelineFactory>                          m_pipelineFactory;    // メンバとして持たせる
        SpriteRenderer                                          m_spriteRenderer;     // スプライト描画クラスのインスタンス
        DrawCommandQueue                                        m_drawQueue;          // 描画コマンドキュー

        std::unique_ptr<DirectX::CommonStates> m_commonStatesTK;
    };
}    // namespace Tsukino::Renderer
