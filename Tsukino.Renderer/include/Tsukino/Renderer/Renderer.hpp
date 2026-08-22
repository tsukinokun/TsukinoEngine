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

// 前方宣言
namespace Tsukino::BuiltIn::ECS {
    class EffectSystem;
}

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {
    struct CBufferScene;    // 前方宣言

    //------------------------------------------------------------
    //! @struct RendererShaderSet
    //! @brief  Renderer::Initialize に渡すビルトインシェーダー一式
    //! @note   位置引数の羅列が肥大化するのを防ぐための集約構造体。
    //!         各メンバの実体は Tsukino::BuiltIn::BuiltInShaders が読み込んだアセット。
    //------------------------------------------------------------
    struct RendererShaderSet {
        const Tsukino::Asset::ShaderAsset* debugVS          = nullptr;    //!< デバッグ線用VS
        const Tsukino::Asset::ShaderAsset* debugPS          = nullptr;    //!< デバッグ線用PS
        const Tsukino::Asset::ShaderAsset* tonemapVS        = nullptr;    //!< フルスクリーン三角形VS（Tonemap/Lighting共用）
        const Tsukino::Asset::ShaderAsset* tonemapPS        = nullptr;    //!< トーンマッピング用PS
        const Tsukino::Asset::ShaderAsset* shadowStaticVS   = nullptr;    //!< シャドウマップ用VS（スタティック）
        const Tsukino::Asset::ShaderAsset* shadowSkeletalVS = nullptr;    //!< シャドウマップ用VS（スケルタル）
        const Tsukino::Asset::ShaderAsset* shadowPS         = nullptr;    //!< シャドウマップ用PS
        const Tsukino::Asset::ShaderAsset* lightingPS       = nullptr;    //!< ディファードLightingパス用PS（VSはtonemapVSを共用）
        const Tsukino::Asset::ShaderAsset* motionBlurPS     = nullptr;    //!< モーションブラーパス用PS（VSはtonemapVSを共用）
        const Tsukino::Asset::ShaderAsset* fogPS            = nullptr;    //!< フォグパス用PS（VSはtonemapVSを共用）
    };

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
        //! @param hwnd    [in] 描画先のウィンドウハンドル
        //! @param width   [in] 描画領域の幅
        //! @param height  [in] 描画領域の高さ
        //! @param shaders [in] ビルトインシェーダー一式
        //! @return true: [in] 初期化成功, false: 初期化失敗
        //------------------------------------------------------------
        [[nodiscard]]
        bool Initialize(HWND hwnd, uint32_t width, uint32_t height, const RendererShaderSet& shaders);

        //------------------------------------------------------------
        // 描画処理
        //------------------------------------------------------------
        void Render(class Tsukino::BuiltIn::ECS::EffectSystem* effectSystem = nullptr);

        //------------------------------------------------------------
        //! @brief 描画領域のリサイズ
        //! @param width  [in] 新しい幅（ピクセル）
        //! @param height [in] 新しい高さ（ピクセル）
        //! @note  ウィンドウの WM_SIZE から呼ばれる。スワップチェインと
        //!        画面サイズ依存のリソースを作り直す。
        //!        シャドウマップは固定解像度のため作り直さない。
        //------------------------------------------------------------
        void Resize(uint32_t width, uint32_t height);

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
        // デバイスコンテキストの取得を公開
        //! @return ID3D11DeviceContextのポインタ
        //------------------------------------------------------------
        [[nodiscard]]
        ID3D11DeviceContext* GetContext() const {
            return m_graphicsContext.GetContext();
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
        //! @note  マテリアルテクスチャ未設定時のデフォルト。
        //!        アルベド/MR/エミッシブ/AOはいずれもcbuffer定数との「乗算」で
        //!        合成するため、白(=1.0)を掛ければ定数値がそのまま残る。
        //------------------------------------------------------------
        ID3D11ShaderResourceView* GetWhiteTextureSRV();

        //------------------------------------------------------------
        //! @brief フラット法線テクスチャのSRVを取得
        //! @return ID3D11ShaderResourceViewへのポインタ
        //! @note  ノーマルマップ未設定時のデフォルト。接空間の(0,0,1)を
        //!        エンコードした値(R=0x80,G=0x80,B=0xFF)で、これを適用しても
        //!        頂点法線がそのまま保たれる。白を使うと法線が斜めにずれる。
        //------------------------------------------------------------
        ID3D11ShaderResourceView* GetFlatNormalTextureSRV();

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
        //! @brief 水面の時間経過を更新（波のアニメーションなどに使用）
        //! @param deltaTime [in] 前フレームからの経過時間
        //------------------------------------------------------------
        void UpdateWaterTime(float deltaTime);

        //------------------------------------------------------------
        //! @brief 水面パラメータのセット
        //! @param water [in] 水面定数バッファデータ
        //------------------------------------------------------------
        void SetWaterParameters(const CBufferWater& water);

        //------------------------------------------------------------
        //! @brief 水面パイプラインのセット
        //! @param vs [in] 頂点シェーダーアセット
        //! @param ps [in] ピクセルシェーダーアセット
        //------------------------------------------------------------
        void SetWaterPipeline(const Tsukino::Asset::ShaderAsset* vs, const Tsukino::Asset::ShaderAsset* ps);

        //------------------------------------------------------------
        //! @brief 点光源・スポットライト配列のセット（ディファードLightingパス用）
        //! @param lights [in] GPULightの配列
        //! @param count  [in] 配列の要素数（MAX_LIGHTSを超える分は切り捨てられる）
        //------------------------------------------------------------
        void SetLights(const GPULight* lights, u32 count);

        //------------------------------------------------------------
        //! @brief モーションブラーパイプラインのセット
        //! @param ps [in] ピクセルシェーダーアセット（VSはtonemapVSを共用する）
        //! @return true: 成功, false: 失敗
        //------------------------------------------------------------
        bool SetMotionBlurPipeline(const Tsukino::Asset::ShaderAsset* ps);

        //------------------------------------------------------------
        //! @brief モーションブラーパラメータのセット
        //! @param params [in] モーションブラー定数バッファデータ
        //------------------------------------------------------------
        void SetMotionBlurParameters(const CBufferMotionBlur& params);

        //------------------------------------------------------------
        //! @brief モーションブラーの有効・無効を切り替える
        //! @param enabled [in] true: 有効, false: 無効
        //! @note  このフラグはフレーム単位で、Render()の末尾で毎回falseへ戻る。
        //!        有効にしたいフレームでは毎フレーム呼ぶこと（MotionBlurSystemの責務）。
        //!        こうしておくと、MotionBlurSystemを持たないシーンへ切り替えたときに
        //!        フラグが立ちっぱなしで残らない。
        //!        無効時は速度バッファ用の前フレームボーン行列（8KB/ドロー）の
        //!        転送もスキップされる。
        //------------------------------------------------------------
        void SetMotionBlurEnabled(bool enabled) noexcept {
            m_motionBlurEnabled = enabled;
        }

        //------------------------------------------------------------
        //! @brief フォグパラメータのセット
        //! @param params [in] フォグ定数バッファデータ
        //------------------------------------------------------------
        void SetFogParameters(const CBufferFog& params);

        //------------------------------------------------------------
        //! @brief フォグの有効・無効を切り替える
        //! @param enabled [in] true: 有効, false: 無効
        //! @note  モーションブラーと同じくフレーム単位のフラグで、Render()の
        //!        末尾で毎回falseへ戻る。有効にしたいフレームでは毎フレーム
        //!        呼ぶこと（FogSystemの責務）。
        //------------------------------------------------------------
        void SetFogEnabled(bool enabled) noexcept {
            m_fogEnabled = enabled;
        }

    private:
        //------------------------------------------------------------
        // 定数バッファの作成
        //! @return true: 定数バッファの作成成功, false: 定数バッファの作成失敗
        //------------------------------------------------------------
        [[nodiscard]] bool CreateConstantBuffer();

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
        //! @brief シャドウ用パイプラインの生成関数
        //! @param shadowStaticVS   [in] スタティックメッシュ用シャドウ頂点シェーダーアセット
        //! @param shadowSkeletalVS [in] スケルタルメッシュ用
        //! @param shadowPS         [in] シャドウピクセルシェーダーアセット
        //! @return true: 作成成功, false: 作成失敗
        //------------------------------------------------------------
        [[nodiscard]]
        bool CreateShadowPipelines(const Tsukino::Asset::ShaderAsset* shadowStaticVS,
                                   const Tsukino::Asset::ShaderAsset* shadowSkeletalVS,
                                   const Tsukino::Asset::ShaderAsset* shadowPS);

        //------------------------------------------------------------
        //! @brief シャドウマップ用リソースの作成
        //! @return true: 作成成功, false: 作成失敗
        //------------------------------------------------------------
        [[nodiscard]]
        bool CreateShadowMap();

        //------------------------------------------------------------
        //! @brief 水面の描画コマンドの実行
        //! @param cmd [in] 実行する描画コマンド
        //------------------------------------------------------------
        void ExecuteWaterCommand(const DrawCommand& cmd);

        //------------------------------------------------------------
        //! @brief シャドウパスの実行（シャドウマップへの深度書き込み）
        //! @param cmd [in] 実行する描画コマンド
        //------------------------------------------------------------
        void ExecuteShadowCommand(const DrawCommand& cmd);

        //------------------------------------------------------------
        //! @brief シャドウ用シェーダーと入力レイアウトの作成
        //! @return true: 作成成功, false: 作成失敗
        //------------------------------------------------------------
        //------------------------------------------------------------
        //! @brief 1x1のデフォルトテクスチャを作成する
        //! @param rgba [in] ピクセル値。R8G8B8A8_UNORMはメモリ上のバイト順が
        //!                  R,G,B,Aなので、リトルエンディアンでは0xAABBGGRRと書く
        //!                  （例: フラット法線 R=0x80,G=0x80,B=0xFF,A=0xFF → 0xFFFF8080）
        //! @param outTex [out] 作成したテクスチャ
        //! @param outSRV [out] 作成したSRV
        //! @param debugName [in] 失敗時のログに出す名前
        //! @return true: 作成成功, false: 作成失敗
        //------------------------------------------------------------
        [[nodiscard]]
        bool Create1x1Texture(u32                                               rgba,
                              Microsoft::WRL::ComPtr<ID3D11Texture2D>&          outTex,
                              Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& outSRV,
                              const char*                                       debugName);

        //------------------------------------------------------------
        //! @brief マテリアル用デフォルトテクスチャ（白・フラット法線）の作成
        //! @return true: 作成成功, false: 作成失敗
        //------------------------------------------------------------
        [[nodiscard]]
        bool CreateDefaultTextures();

        //------------------------------------------------------------
        //! @brief スカイパスの実行
        //------------------------------------------------------------
        void ExecuteSkyPass();

        //------------------------------------------------------------
        //! @brief ディファードLightingパスの実行
        //------------------------------------------------------------
        void ExecuteLightingPass();

        //------------------------------------------------------------
        //! @brief ディファードLightingパイプラインのセット
        //! @param ps [in] ピクセルシェーダーアセット（VSはTonemapと共用）
        //! @return true: 成功, false: 失敗
        //------------------------------------------------------------
        [[nodiscard]]
        bool SetLightingPipeline(const Tsukino::Asset::ShaderAsset* ps);

        //------------------------------------------------------------
        //! @brief モーションブラーパスの実行
        //! @return true: ブラーを実行してポストプロセスバッファへ書いた
        //!         false: 無効なので何もしていない（HDRバッファがそのまま最新）
        //! @note  HDRバッファを読み、ポストプロセス用中間バッファへ書く。
        //!        Waterパスの直後・Tonemapパスの直前に呼ぶこと。
        //------------------------------------------------------------
        bool ExecuteMotionBlurPass();

        //------------------------------------------------------------
        //! @brief フォグパスの実行
        //! @note  深度バッファだけを読み、HDRバッファへ直接over合成する。
        //!        HDRをSRVとして読まないので中間バッファを消費しない。
        //!        Waterパスの直後・モーションブラーパスの直前に呼ぶこと。
        //------------------------------------------------------------
        void ExecuteFogPass();

        //------------------------------------------------------------
        //! @brief フォグパイプラインのセット
        //! @param ps [in] ピクセルシェーダーアセット（VSはtonemapVSを共用する）
        //! @return true: 成功, false: 失敗
        //------------------------------------------------------------
        [[nodiscard]]
        bool SetFogPipeline(const Tsukino::Asset::ShaderAsset* ps);

        //------------------------------------------------------------
        //! @brief トーンマッピングパスの実行
        //! @param source [in] 入力となるシーンカラーのSRV
        //!                    （モーションブラーが走ったかどうかで切り替わる）
        //------------------------------------------------------------
        void ExecuteTonemapPass(ID3D11ShaderResourceView* source);

        //------------------------------------------------------------
        //! @brief トーンマッピングパイプラインのセット
        //! @param vs [in] 頂点シェーダーアセット
        //! @param ps [in] ピクセルシェーダーアセット
        //------------------------------------------------------------
        void SetTonemapPipeline(const Tsukino::Asset::ShaderAsset* vs, const Tsukino::Asset::ShaderAsset* ps);

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

        // モーションブラー用リソース
        ComPtr<ID3D11Buffer>      m_prevSkinningBuffer;    //!< 前フレームのボーン行列用バッファ (b7)
        ComPtr<ID3D11Buffer>      m_motionBlurBuffer;      //!< モーションブラーパラメータ用バッファ (b8)
        ComPtr<ID3D11PixelShader> m_motionBlurPS;          //!< モーションブラー用PS（VSはm_tonemapVSを共用）
        CBufferMotionBlur         m_motionBlurData{};      //!< CPU側のモーションブラーパラメータ
        bool                      m_hasMotionBlur     = false;    //!< PSの構築が済んでいるか
        bool                      m_motionBlurEnabled = false;    //!< 今フレームで有効か（MotionBlurSystemが毎フレーム設定）

        //! @brief 前フレームのViewProjection行列
        //! @note  CameraSystemはdirty時しか行列を再計算しないため、
        //!        Render()の末尾でフレーム単位に退避するのが確実。
        Tsukino::Core::Math::matrix m_prevWorldViewProj = Tsukino::Core::Math::matrix::identity();

        // シャドウマップ用リソース
        static constexpr uint32_t        SHADOW_MAP_SIZE = 2048;
        ComPtr<ID3D11Texture2D>          m_shadowMapTex;     //!< シャドウマップテクスチャ
        ComPtr<ID3D11DepthStencilView>   m_shadowMapDSV;     //!< シャドウマップDSV（深度書き込み用）
        ComPtr<ID3D11ShaderResourceView> m_shadowMapSRV;     //!< シャドウマップSRV（PSでのサンプリング用）
        ComPtr<ID3D11SamplerState>       m_shadowSampler;    //!< PCF用比較サンプラー

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

        // マテリアルテクスチャ未設定時のデフォルト
        Microsoft::WRL::ComPtr<ID3D11Texture2D>          m_whiteTex;
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_whiteSRV;
        Microsoft::WRL::ComPtr<ID3D11Texture2D>          m_flatNormalTex;
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_flatNormalSRV;

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
        ComPtr<ID3D11Buffer> m_waterBuffer;    //!< 水面定数バッファ (b5)
        CBufferWater         m_waterData{};
        float                m_waterTime = 0.0f;
        bool                 m_hasWater  = false;

        std::shared_ptr<PipelineState> m_waterPipeline;         //!< 水面用パイプラインキャッシュ
        ComPtr<ID3D11SamplerState>     m_waterShadowSampler;    //!< 水面用 PCF サンプラー (s8)

        // ディファードLightingパス用リソース
        ComPtr<ID3D11PixelShader> m_lightingPS;              //!< Lightingパス用PS（VSはm_tonemapVSを共用）
        bool                      m_hasLighting = false;
        ComPtr<ID3D11Buffer>      m_lightsBuffer;             //!< 点光源・スポットライト配列用定数バッファ (b6)
        CBufferLights             m_lightsData{};              //!< CPU側のライト配列（毎フレームGPUへ転送）
        bool                      m_lightOverflowWarned = false;    //!< MAX_LIGHTS超過の警告を1回だけ出すためのフラグ

        // フォグ用リソース
        ComPtr<ID3D11PixelShader> m_fogPS;                //!< フォグ用PS（VSはm_tonemapVSを共用）
        ComPtr<ID3D11Buffer>      m_fogBuffer;            //!< フォグパラメータ用バッファ (b9)
        CBufferFog                m_fogData{};            //!< CPU側のフォグパラメータ
        bool                      m_hasFog     = false;    //!< PSの構築が済んでいるか
        bool                      m_fogEnabled = false;    //!< 今フレームで有効か（FogSystemが毎フレーム設定）
    };
}    // namespace Tsukino::Renderer
