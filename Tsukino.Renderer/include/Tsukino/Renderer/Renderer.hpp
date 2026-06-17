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
#include <Tsukino/Renderer/ConstantBuffer.hpp>

#include <Tsukino/GraphicsCommon/Mesh/PrimitiveType.hpp>
#include <Tsukino/GraphicsCommon/Mesh/MeshData.hpp>
#include <Tsukino/GraphicsCommon/State/SamplerType.hpp>
#include <Tsukino/GraphicsCommon/Vertex/DebugVertex.hpp>

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
        bool Initialize(HWND                               hwnd,
                        uint32_t                           width,
                        uint32_t                           height,
                        const Tsukino::Asset::ShaderAsset* debugVS,
                        const Tsukino::Asset::ShaderAsset* debugPS,
                        const Tsukino::Asset::ShaderAsset* tonemapVS,
                        const Tsukino::Asset::ShaderAsset* tonemapPS);

        //------------------------------------------------------------
        // 描画処理
        //------------------------------------------------------------
        void Render();

        //------------------------------------------------------------
        // 描画領域のクリアカラーを設定
        //! @param r [in] 赤成分 (0.0f - 1.0f)
        //! @param g [in] 緑成分 (0.0f - 1.0f)
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
        // デバッグライン/三角形の追加
        //------------------------------------------------------------
        void DrawDebugLine(const Tsukino::GraphicsCommon::DebugVertex& v1, const Tsukino::GraphicsCommon::DebugVertex& v2);
        void DrawDebugTriangle(const Tsukino::GraphicsCommon::DebugVertex& v1,
                               const Tsukino::GraphicsCommon::DebugVertex& v2,
                               const Tsukino::GraphicsCommon::DebugVertex& v3);

        //------------------------------------------------------------
        // 追加されたデバッグ線を実際に描画する
        //------------------------------------------------------------
        void FlushDebugDraw();

        //------------------------------------------------------------
        // PipelineFactoryを使うためのGetterを公開
        //! @return PipelineFactoryのポインタ
        //------------------------------------------------------------
        [[nodiscard]]
        PipelineFactory* GetPipelineFactory() {
            return &m_pipelineFactory.value();
        }

        //------------------------------------------------------------
        // デバイスの取得を公開
        //! @return ID3D11Deviceのポインタ
        //------------------------------------------------------------
        [[nodiscard]]
        ID3D11Device* GetDevice() const {
            return m_graphicsContext.GetDevice();
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
        std::unique_ptr<DirectX::SpriteFont> CreateSpriteFont(const u8* data, size_t size);

        //------------------------------------------------------------
        // ワールドカメラ行列のセット
        //! @param data [in] シーン定数バッファの値データ
        //------------------------------------------------------------
        void SetWorldCameraMatrix(const CBufferScene& data);

        //------------------------------------------------------------
        // オーバーレイカメラ行列のセット
        //! @param data [in] シーン定数バッファの値データ
        //------------------------------------------------------------
        void SetOverlayCameraMatrix(const CBufferScene& data);

        //------------------------------------------------------------
        // スプライトバッチの作成
        //! @return SpriteBatchのユニークポインタ
        //------------------------------------------------------------
        std::unique_ptr<DirectX::SpriteBatch> CreateSpriteBatch();

        //------------------------------------------------------------
        //! @brief 共通ステートの取得
        //------------------------------------------------------------
        DirectX::CommonStates* GetCommonStatesTK() const { return m_commonStatesTK.get(); }

        //------------------------------------------------------------
        //! @brief ディレクショナルライトの設定
        //! @param direction [in] ライトの方向（正規化推奨）
        //! @param color     [in] ライトの色
        //! @param intensity [in] ライトの強度
        //------------------------------------------------------------
        void SetDirectionalLight(const hlslpp::float3& direction, const hlslpp::float3& color, float intensity);

        //------------------------------------------------------------
        //! @brief シャドウパイプラインのセット
        //! @param staticPipeline   [in] スタティックメッシュ用シャド
        //! @param skeletalPipeline [in] スケルタルメッシュ用シャドウパイプライン
        //------------------------------------------------------------
        void SetShadowPipeline(std::shared_ptr<PipelineState> staticPipeline, std::shared_ptr<PipelineState> skeletalPipeline);

        //------------------------------------------------------------
        //! @brief 白テクスチャのSRVを取得
        //! @return ID3D11ShaderResourceViewへのポインタ
        //------------------------------------------------------------
        ID3D11ShaderResourceView* GetWhiteTextureSRV();

        //------------------------------------------------------------
        //! @brief 大気散乱パラメータのセット
        //! @param sky [in] 大気散乱定数バッファデータ
        //------------------------------------------------------------
        void SetSkyParameters(const CBufferSky& sky);

        //------------------------------------------------------------
        //! @brief スカイパイプラインのセット
        //! @param vs [in] 頂点シェーダーアセット
        //! @param ps [in] ピクセルシェーダーアセット
        //------------------------------------------------------------
        void SetSkyPipeline(const Tsukino::Asset::ShaderAsset* vs, const Tsukino::Asset::ShaderAsset* ps);

        //------------------------------------------------------------
        //! @brief 水面パイプラインのセット
        //! @param vs [in] 頂点シェーダーアセット
        //! @param ps [in] ピクセルシェーダーアセット
        //------------------------------------------------------------
        void SetWaterPipeline(const Tsukino::Asset::ShaderAsset* vs, const Tsukino::Asset::ShaderAsset* ps);

        //------------------------------------------------------------
        //! @brief 水面の時間経過を更新（波のアニメーションなどに使用）
        //! @param deltaTime [in] 前フレームからの経過時間
        //------------------------------------------------------------
        void UpdateWaterTime(float deltaTime);

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

        //------------------------------------------------------------
        // デバッグ用バッファとシェーダーの作成
        //! @return true: 作成成功, false: 作成失敗
        //------------------------------------------------------------
        [[nodiscard]]
        bool CreateDebugBuffers(const Tsukino::Asset::ShaderAsset* vs, const Tsukino::Asset::ShaderAsset* ps);

        //------------------------------------------------------------
        //! @brief シャドウマップ用リソースの作成
        //! @return true: 作成成功, false: 作成失敗
        //------------------------------------------------------------
        [[nodiscard]]
        bool CreateShadowMap();

        //------------------------------------------------------------
        //! @brief シャドウパスの実行（シャドウマップへの深度書き込み）
        //! @param cmd [in] 実行する描画コマンド
        //------------------------------------------------------------
        void ExecuteShadowCommand(const DrawCommand& cmd);

        //------------------------------------------------------------
        //! @brief シャドウ用シェーダーと入力レイアウトの作成
        //! @return true: 作成成功, false: 作成失敗
        //------------------------------------------------------------
        [[nodiscard]]
        bool CreateWhiteTexture();

        //------------------------------------------------------------
        //! @brief スカイパスの実行
        //------------------------------------------------------------
        void ExecuteSkyPass();

        //------------------------------------------------------------
        //! @brief トーンマッピングパスの実行
        //------------------------------------------------------------
        void ExecuteTonemapPass();

        //------------------------------------------------------------
        //! @brief トーンマッピングパイプラインのセット
        //! @param vs [in] 頂点シェーダーアセット
        //! @param ps [in] ピクセルシェーダーアセット
        //------------------------------------------------------------
        void SetTonemapPipeline(const Tsukino::Asset::ShaderAsset* vs, const Tsukino::Asset::ShaderAsset* ps);

        //------------------------------------------------------------
        //! @brief 水面の作成
        //------------------------------------------------------------
        bool CreateWaterResources();
    private:
        // DirectX 11の主要なインターフェース
        GraphicsContext            m_graphicsContext;    // グラフィックスコンテキスト（Device, DeviceContext, SwapChainを管理）
        ComPtr<ID3D11Buffer>       m_vertexBuffer;       // 頂点バッファ
        ComPtr<ID3D11VertexShader> m_vertexShader;       // 頂点シェーダ
        ComPtr<ID3D11PixelShader>  m_pixelShader;        // ピクセルシェーダ
        ComPtr<ID3D11InputLayout>  m_inputLayout;        // 入力レイアウト

        // 定数バッファ
        ComPtr<ID3D11Buffer> m_objectBuffer;      // オブジェクトデータ用定数バッファ
        ComPtr<ID3D11Buffer> m_sceneBuffer;       // シーンデータ用定数バッファ
        ComPtr<ID3D11Buffer> m_materialBuffer;    // マテリアルデータ用定数バッファ
        ComPtr<ID3D11Buffer> m_skinningBuffer;    // ボーン行列用バッファ

        // シャドウマップ用リソース
        static constexpr uint32_t        SHADOW_MAP_SIZE = 2048;
        ComPtr<ID3D11Texture2D>          m_shadowMapTex;     //!< シャドウマップテクスチャ
        ComPtr<ID3D11DepthStencilView>   m_shadowMapDSV;     //!< シャドウマップDSV（深度書き込み用）
        ComPtr<ID3D11ShaderResourceView> m_shadowMapSRV;     //!< シャドウマップSRV（PSでのサンプリング用）
        ComPtr<ID3D11SamplerState>       m_shadowSampler;    //!< PCF用比較サンプラー

        // シャドウ用シェーダー
        ComPtr<ID3D11VertexShader> m_shadowVS;            //!< スタティック用シャドウVS
        ComPtr<ID3D11VertexShader> m_shadowSkeletalVS;    //!< スケルタル用シャドウVS
        ComPtr<ID3D11InputLayout>  m_shadowStaticIL;      //!< スタティック用入力レイアウト
        ComPtr<ID3D11InputLayout>  m_shadowSkeletalIL;    //!< スケルタル用入力レイアウト

        // シャドウ用パイプラインステート
        std::shared_ptr<PipelineState> m_shadowStaticPipeline;      //!< スタティック用シャドウパイプライン
        std::shared_ptr<PipelineState> m_shadowSkeletalPipeline;    //!< スケルタル用シャドウパイプライン

        std::array<float, 4> m_clearColor = {0.5f, 0.5f, 0.5f, 1.0f};    // 描画領域のクリアカラー (デフォルトはグレー)

        std::array<MeshBuffer, (size_t)Tsukino::GraphicsCommon::PrimitiveType::Count> m_primitiveMeshes;    // プリミティブメッシュバッファの配列
        std::array<ComPtr<ID3D11SamplerState>, static_cast<size_t>(Tsukino::GraphicsCommon::SamplerType::Count)> m_samplers;
        std::unordered_map<u64, std::unique_ptr<DX11Texture2D>> m_textureCache;       // Textureのキャッシュ (AssetHandle の Value(uint64_t) をキーにする)
        std::optional<PipelineFactory>                          m_pipelineFactory;    // メンバとして持たせる
        SpriteRenderer                                          m_spriteRenderer;     // スプライト描画クラスのインスタンス
        DrawCommandQueue                                        m_drawQueue;          // 描画コマンドキュー

        // カメラ行列のセットを保存する変数
        Tsukino::Renderer::CBufferScene m_worldSceneData;      // 3D（メインカメラ）用
        Tsukino::Renderer::CBufferScene m_overlaySceneData;    // 2D（UIカメラ）用

        // デバッグ描画用の頂点群
        std::vector<Tsukino::GraphicsCommon::DebugVertex> m_debugLineVertices;
        std::vector<Tsukino::GraphicsCommon::DebugVertex> m_debugTriangleVertices;

        // デバッグ描画用の動的バッファ等
        ComPtr<ID3D11Buffer>       m_debugLineVB;
        ComPtr<ID3D11Buffer>       m_debugTriangleVB;
        ComPtr<ID3D11VertexShader> m_debugVS;
        ComPtr<ID3D11PixelShader>  m_debugPS;
        ComPtr<ID3D11InputLayout>  m_debugIL;

        std::unique_ptr<DirectX::CommonStates> m_commonStatesTK;

        // 白テクスチャ用
        Microsoft::WRL::ComPtr<ID3D11Texture2D>          m_whiteTex;
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_whiteSRV;

        // スカイ用リソース
        ComPtr<ID3D11VertexShader> m_skyVS;             //!< スカイ用頂点シェーダー
        ComPtr<ID3D11PixelShader>  m_skyPS;             //!< スカイ用ピクセルシェーダー
        ComPtr<ID3D11Buffer>       m_skyBuffer;         //!< スカイ定数バッファ (b4)
        CBufferSky                 m_skyData{};         //!< スカイパラメータ
        bool                       m_hasSky = false;    //!< スカイが有効かどうか

        // トーンマッピング用リソース
        ComPtr<ID3D11VertexShader> m_tonemapVS;    //!< トーンマッピング用VS
        ComPtr<ID3D11PixelShader>  m_tonemapPS;    //!< トーンマッピング用PS
        bool                       m_hasTonemapper = false;

        // 水面用リソース
        ComPtr<ID3D11VertexShader> m_waterVS;
        ComPtr<ID3D11PixelShader>  m_waterPS;
        ComPtr<ID3D11Buffer>       m_waterBuffer;    //!< 水面定数バッファ (b5)
        CBufferWater               m_waterData{};
        float                      m_waterTime = 0.0f;
        bool                       m_hasWater  = false;
    };
}    // namespace Tsukino::Renderer
