//------------------------------------------------------------
//! @file	Renderer.hpp
//! @brief	レンダラークラスの宣言
//! @author 山﨑愛
//------------------------------------------------------------
#pragma once
#include <Tsukino/Core/WindowsLean.hpp>

#include <Tsukino/Renderer/DX11/GraphicsContext.hpp>
#include <Tsukino/Renderer/DX11/MeshBuffer.hpp>
#include <Tsukino/Renderer/DX11/PipelineFactory.hpp>
#include <Tsukino/Renderer/SpriteRenderer.hpp>
#include <Tsukino/Renderer/DrawCommandQueue.hpp>
#include <Tsukino/Renderer/DX11/Texture/DX11Texture2D.hpp>
#include <Tsukino/Renderer/DX11/Texture/DX11TextureCube.hpp>
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
#include <vector>


namespace Tsukino::Asset {
    class TextureAsset;    // 前方宣言
}

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {
    // ComPtr の using 宣言。公開ヘッダなのでグローバルではなく名前空間の内側に置く
    using Microsoft::WRL::ComPtr;

    struct CBufferScene;    // 前方宣言
    class IPostWorldPass;   // 前方宣言（Worldパスの後に差し込む描画。実体は上位層が持つ）

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
        const Tsukino::Asset::ShaderAsset* ambientParticleVS = nullptr;    //!< 環境パーティクル用VS（SV_VertexIDだけで板を生成する）
        const Tsukino::Asset::ShaderAsset* ambientParticlePS = nullptr;    //!< 環境パーティクル用PS

        //! IBLベイク用PS（VSはtonemapVSを共用）。irradiance畳み込み・スペキュラプレフィルタ・
        //! BRDF LUT生成はいずれもフルスクリーン三角形へのPS一発で完結する
        const Tsukino::Asset::ShaderAsset* iblIrradiancePS      = nullptr;    //!< 拡散IBL用irradiance畳み込みPS
        const Tsukino::Asset::ShaderAsset* iblSpecularPrefilterPS = nullptr;    //!< 鏡面IBL用プレフィルタPS
        const Tsukino::Asset::ShaderAsset* iblBRDFLUTPS         = nullptr;    //!< split-sum用BRDF積分LUT生成PS
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
        //! @param postWorldPass [in] Worldパスの後に差し込む追加描画（不要ならnullptr）
        //------------------------------------------------------------
        void Render(IPostWorldPass* postWorldPass = nullptr);

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
        // このフレームで使うマテリアル実体を1つ確保する
        //! @return 確保したマテリアルへの参照
        //! @note   DrawCommand::material が指す実体は必ずここから取ること。
        //!         System 側で持つと、コマンドの寿命と食い違ってダングリングになる
        //------------------------------------------------------------
        [[nodiscard]]
        Material& AllocMaterial();

        //------------------------------------------------------------
        // このフレームで使うマテリアル定数データを1つ確保する
        //! @return 確保した定数データへの参照
        //! @note   DrawCommand::materialData が指す実体は必ずここから取ること
        //------------------------------------------------------------
        [[nodiscard]]
        CBufferMaterial& AllocMaterialData();

        //------------------------------------------------------------
        //! @struct FrameStats
        //! @brief  1フレーム分の描画統計（負荷調査用）
        //! @note   Render()の先頭でリセットし、各パスの実行中に積む。
        //!         フレーム時間だけを見ても「ドローコールが多いのか、
        //!         1本あたりが重いのか」が分からないため、内訳を数える
        //------------------------------------------------------------
        struct FrameStats {
            u32 commandCount = 0;    //!< DrawCommandQueueに積まれたコマンド総数

            u32 shadowDrawCalls      = 0;    //!< Shadowパスのドロー数（GBufferと同じ形状をもう一度描いている）
            u32 gbufferDrawCalls     = 0;    //!< GBufferパスのドロー数
            u32 worldDrawCalls       = 0;    //!< Worldパス（フォワード不透明・デバッグ線）のドロー数
            u32 transparentDrawCalls = 0;    //!< TransparentDepth + Transparent のドロー数
            u32 overlayDrawCalls     = 0;    //!< Overlayパス（UI・フォント）のドロー数

            u32 skinnedDrawCalls = 0;    //!< うちスキニングありのドロー数
            u64 triangleCount    = 0;    //!< 描画した三角形数（インデックス数 / 3 の総和）

            //! ボーン行列として定数バッファへ転送したバイト数。
            //! 1スキンドローあたり sizeof(CBufferSkinning) = 8KB を実ボーン数に関係なく
            //! 転送しているため、ここが跳ね上がるならその改善が効くという判断材料になる
            u64 boneBytesUploaded = 0;

            //------------------------------------------------------------
            //! @brief 全パスのドローコール数の合計を返す関数
            //------------------------------------------------------------
            [[nodiscard]]
            u32 TotalDrawCalls() const {
                return shadowDrawCalls + gbufferDrawCalls + worldDrawCalls + transparentDrawCalls + overlayDrawCalls;
            }
        };

        //------------------------------------------------------------
        //! @brief  直前のフレームの描画統計を取得する関数
        //! @return 描画統計
        //------------------------------------------------------------
        [[nodiscard]]
        const FrameStats& GetFrameStats() const {
            return m_frameStats;
        }

        //------------------------------------------------------------
        //! @brief 垂直同期の有無を設定する関数
        //! @param enabled [in] true でVSync有効、false で無効
        //! @note  性能計測時に false にする。詳細は GraphicsContext::SetVSyncEnabled を参照
        //------------------------------------------------------------
        void SetVSyncEnabled(bool enabled) {
            m_graphicsContext.SetVSyncEnabled(enabled);
        }

        //------------------------------------------------------------
        //! @brief  垂直同期が有効かを取得する関数
        //------------------------------------------------------------
        [[nodiscard]]
        bool IsVSyncEnabled() const {
            return m_graphicsContext.IsVSyncEnabled();
        }

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
        //! @note   シャドウマップの投影範囲（平行投影、±500ユニット）は
        //!         ワールド原点ではなく、このフレームのカメラ位置
        //!         （SetWorldCameraMatrix()が書き込んだ値）を中心にする。
        //!         そのため、このフレームのカメラ更新（SetWorldCameraMatrix
        //!         を呼ぶ側。通常はCameraSystem）より後に呼び出すこと
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

        //------------------------------------------------------------
        //! 環境パーティクルのパラメータをセットします。
        //! @param  [in] params        環境パーティクル定数バッファデータ
        //! @param  [in] particleCount 粒子数（kMaxAmbientParticles でクランプされる）
        //------------------------------------------------------------
        void SetAmbientParticleParameters(const CBufferAmbientParticle& params, u32 particleCount);

        //------------------------------------------------------------
        //! 環境パーティクルの有効・無効を切り替えます。
        //! @param [in] enabled true: 有効, false: 無効
        //! @note  フォグと同じくフレーム単位のフラグで、Render()の末尾で
        //!        毎回falseへ戻る。有効にしたいフレームでは毎フレーム
        //!        呼ぶこと（AmbientParticleSystemの責務）。
        //------------------------------------------------------------
        void SetAmbientParticleEnabled(bool enabled) noexcept {
            m_ambientParticleEnabled = enabled;
        }

        //------------------------------------------------------------
        //! @brief IBL（スカイ由来の環境光）の再ベイクを要求する
        //! @note  IBLはスカイのキャプチャ→irradiance畳み込み→スペキュラプレフィルタという
        //!        一発ベイクで、Render()内でスカイパイプライン確立後に自動的に一度だけ走る。
        //!        太陽が動く演出（day-night等）が将来追加されたときは、そのシステムが
        //!        太陽角度の変化を検知してこれを呼べば次フレームで再ベイクされる。
        //!        ベイクは6〜48回のフルスクリーン三角形描画（すべて128px以下）を伴うため、
        //!        毎フレーム呼ぶような使い方はしないこと（呼び出し側でスロットリングする）。
        //------------------------------------------------------------
        void RequestIBLRecapture() noexcept {
            m_iblBaked = false;
        }

        //------------------------------------------------------------
        //! フレームの経過時間を進めます。
        //! @param deltaTime [in] 前フレームからの経過秒
        //! @note  ここで進めた時間は CBufferScene(b0) の timeParams として
        //!        全シェーダーへ配られる。演出ごとに自前の時間を持たずに済むよう、
        //!        エンジンが1箇所で数える（UnityのTime / UEのView.GameTimeと同じ考え方）。
        //!        毎フレーム1回だけ呼ぶこと
        //------------------------------------------------------------
        void AdvanceFrameTime(float deltaTime);

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
        //! @brief シャドウパスの実行（シャドウマップへの深度書き込み）
        //! @param cmd [in] 実行する描画コマンド
        //------------------------------------------------------------
        void ExecuteShadowCommand(const DrawCommand& cmd);

        //------------------------------------------------------------
        //! @brief パス別のドローコール数と三角形数を数える関数
        //! @param pass       [in] 描画パス
        //! @param indexCount [in] 描画したインデックス数
        //------------------------------------------------------------
        void CountDrawCall(RenderPass pass, u32 indexCount);

        //------------------------------------------------------------
        //! インスタンスごとのデータを頂点シェーダーへバインドします。
        //! @param srv [in] バインドするSRV。nullptrならスロットを明示的に空にする
        //! @note  スロットを空にする処理が要るのは、インスタンス描画をしない
        //!        コマンドが直前のコマンドのSRVを引き継いでしまうのを防ぐため
        //------------------------------------------------------------
        void BindInstanceData(ID3D11ShaderResourceView* srv);

        //------------------------------------------------------------
        //! ゲーム定義の定数バッファをバインドします。
        //! @param buffer [in] バインドするバッファ。nullptrならスロットを空にする
        //! @param slot   [in] バインド先。User0 / User1 以外は無視する
        //! @note  エンジンが使う b0〜b9 を上書きされると描画が壊れるため、
        //!        ゲーム予約枠以外は弾く
        //------------------------------------------------------------
        void BindUserConstantBuffer(ID3D11Buffer* buffer, CBSlot slot);

        //------------------------------------------------------------
        //! @brief  ボーン行列を定数バッファへ転送する関数
        //! @param  buffer       [in] 転送先の定数バッファ（DYNAMICであること）
        //! @param  boneMatrices [in] ボーン行列の配列
        //! @param  boneCount    [in] ボーン数
        //! @return 実際に転送したバイト数（統計用）
        //------------------------------------------------------------
        u32 UploadBoneMatrices(ID3D11Buffer* buffer, const void* boneMatrices, u32 boneCount);

        //! @brief シェーダー側のボーン配列の宣言数（CBufferSkinning::bones と揃えること）
        static constexpr u32 kMaxBoneCount = 128;

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
        //! @brief IBL用リソース（キューブマップ・BRDF LUT・ベイク用PS）の作成
        //! @param shaders [in] ビルトインシェーダー一式（irradiance/prefilter/BRDF LUT用PS）
        //! @return true: 成功, false: 失敗（失敗時はIBLが定数0扱いになるだけで描画は継続する）
        //! @note  BRDF LUTはスカイに依存しないため、ここで即座に一度だけベイクする。
        //!        キャプチャ/irradiance/プレフィルタのスカイ由来ベイクはRender()側で
        //!        スカイパイプライン確立後に別途トリガーされる（ExecuteIBL*Pass参照）。
        //------------------------------------------------------------
        [[nodiscard]]
        bool CreateIBLResources(const RendererShaderSet& shaders);

        //------------------------------------------------------------
        //! @brief 原点から見たキューブの1面ぶんのCBufferSceneを組み立てる
        //! @param face [in] キューブの面（0〜5：+X,-X,+Y,-Y,+Z,-Z）
        //! @return IBLベイク用に view/projection/viewProj/invViewProj/cameraPos を
        //!         差し替えたCBufferScene（lightDir/lightColor/timeParams等は
        //!         m_worldSceneDataの値をそのまま引き継ぐ。Sky.ps.hlslが
        //!         lightColorを太陽の色として読むため）
        //------------------------------------------------------------
        [[nodiscard]]
        CBufferScene BuildCubeFaceSceneData(u32 face) const;

        //------------------------------------------------------------
        //! @brief IBLキャプチャパスの実行（スカイを6面のキューブマップへ焼く）
        //! @note  Sky.ps.hlslを面ごとに視点だけ差し替えてそのまま再利用する
        //------------------------------------------------------------
        void ExecuteIBLCapturePass();

        //------------------------------------------------------------
        //! @brief IBL irradiance畳み込みパスの実行（拡散IBL用、面ごとに1回）
        //------------------------------------------------------------
        void ExecuteIBLIrradiancePass();

        //------------------------------------------------------------
        //! @brief IBLスペキュラプレフィルタパスの実行（鏡面IBL用、面×mipごとに1回）
        //------------------------------------------------------------
        void ExecuteIBLSpecularPrefilterPass();

        //------------------------------------------------------------
        //! @brief IBL BRDF LUT生成パスの実行（スカイに依存しないため起動時に1回だけ呼ぶ）
        //------------------------------------------------------------
        void ExecuteIBLBRDFLUTPass();

        //------------------------------------------------------------
        //! @brief IBL消費側（Lighting.ps.hlsl / Model.ps.hlsl）向けにIBLリソースをバインドする
        //! @note  b10, t17〜t19, s10 をまとめてPSへセットする。ディファードLightingパスと
        //!        フォワードのWorld/TransparentDepth/Transparentパスの両方から呼ぶ
        //!        （EvaluateIBLを呼ぶシェーダーがその2系統だけのため）
        //------------------------------------------------------------
        void BindIBLResources();

        //------------------------------------------------------------
        //! @brief BindIBLResourcesで張ったSRVを解除する
        //------------------------------------------------------------
        void UnbindIBLResources();

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
        //!        Transparentパスの直後・Tonemapパスの直前に呼ぶこと。
        //------------------------------------------------------------
        bool ExecuteMotionBlurPass();

        //------------------------------------------------------------
        //! @brief フォグパスの実行
        //! @note  深度バッファだけを読み、HDRバッファへ直接over合成する。
        //!        HDRをSRVとして読まないので中間バッファを消費しない。
        //!        Transparentパスの直後・モーションブラーパスの直前に呼ぶこと。
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
        //! 環境パーティクルパスを実行します。
        //! @note  頂点バッファもインデックスバッファも持たず、1回のDrawで
        //!        粒子数×6頂点を出す。粒子の属性はすべて頂点シェーダーが
        //!        SV_VertexIDのハッシュから作るため、送るのはb10のパラメータだけ。
        //!        深度テストありのHDRバッファ描画なので世界の物体に隠れ、
        //!        フォグとトーンマップの両方が乗る。フォグパスの直前に呼ぶこと。
        //------------------------------------------------------------
        void ExecuteAmbientParticlePass();

        //------------------------------------------------------------
        //! 環境パーティクルのパイプラインをセットします。
        //! @param  [in] vs 頂点シェーダーアセット
        //! @param  [in] ps ピクセルシェーダーアセット
        //! @return true: 成功, false: 失敗
        //------------------------------------------------------------
        [[nodiscard]]
        bool SetAmbientParticlePipeline(const Tsukino::Asset::ShaderAsset* vs, const Tsukino::Asset::ShaderAsset* ps);

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
        FrameStats m_frameStats;    // 1フレーム分の描画統計（Render()の先頭でリセットする）

        u32 m_lastDrawBoneBytes = 0;    // 直前のExecuteDrawCommandで転送したボーン行列のバイト数（統計用）
        u32 m_shadowBoneBytes   = 0;    // 直前のExecuteShadowCommandで転送したボーン行列のバイト数（統計用）

        DrawCommandQueue                                        m_drawQueue;          // 描画コマンドキュー

        //! Overlayパスの実行順を決めるための添字バッファ。
        //! DrawCommand本体ではなく添字を並べ替えるのは、DrawCommandがhlsl++の
        //! 16バイト境界型（matrix）を含み、並べ替えの一時バッファが拡張アライメント
        //! として弾かれるため（SpriteRenderSystem/FontRendererSystemと同じ理由）。
        //! メンバに持たせてclear()で使い回し、毎フレームの確保を避ける
        std::vector<u32> m_overlayOrder;

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

        // 環境パーティクル用リソース
        ComPtr<ID3D11VertexShader> m_ambientParticleVS;                     //!< 環境パーティクル用VS
        ComPtr<ID3D11PixelShader>  m_ambientParticlePS;                     //!< 環境パーティクル用PS
        float                      m_elapsedTime    = 0.0f;                 //!< 起動からの経過秒（b0のtimeParams.xへ配られる）
        float                      m_frameDeltaTime = 0.0f;                 //!< 前フレームからの経過秒（同 timeParams.y）

        ComPtr<ID3D11Buffer>       m_ambientParticleBuffer;                 //!< 環境パーティクルパラメータ用バッファ (b9)
        CBufferAmbientParticle     m_ambientParticleData{};                 //!< CPU側の環境パーティクルパラメータ
        u32                        m_ambientParticleCount   = 0;            //!< 今フレームの粒子数
        bool                       m_hasAmbientParticle     = false;        //!< シェーダーの構築が済んでいるか
        bool                       m_ambientParticleEnabled = false;        //!< 今フレームで有効か（AmbientParticleSystemが毎フレーム設定）

        // IBL（スカイ由来の環境光）用リソース
        static constexpr u32 kIBLCaptureSize      = 128;    //!< キャプチャキューブの1面の一辺（px）
        static constexpr u32 kIBLIrradianceSize    = 32;     //!< irradianceキューブの1面の一辺（px）
        static constexpr u32 kIBLSpecularBaseSize  = 128;    //!< プレフィルタ済みスペキュラキューブのmip0の一辺（px）
        static constexpr u32 kIBLSpecularMipCount  = 6;      //!< プレフィルタ済みスペキュラキューブのミップ数
        static constexpr u32 kIBLBRDFLUTSize       = 128;    //!< BRDF LUTの一辺（px）

        std::unique_ptr<DX11TextureCube> m_iblCaptureCube;              //!< スカイを焼いたキャプチャキューブ（一発ベイクの中間結果）
        std::unique_ptr<DX11TextureCube> m_iblIrradianceCube;           //!< 拡散IBL用irradianceキューブ（t17で読む）
        std::unique_ptr<DX11TextureCube> m_iblPrefilteredSpecularCube;  //!< 鏡面IBL用プレフィルタ済みキューブ（t18で読む）

        ComPtr<ID3D11Texture2D>          m_iblBRDFLUTTex;    //!< split-sum用BRDF積分LUT本体
        ComPtr<ID3D11RenderTargetView>   m_iblBRDFLUTRTV;    //!< 生成時（起動時1回だけ）に使うRTV
        ComPtr<ID3D11ShaderResourceView> m_iblBRDFLUTSRV;    //!< シェーダー読み取り用SRV（t19）

        ComPtr<ID3D11PixelShader> m_iblIrradiancePS;           //!< irradiance畳み込み用PS（VSはm_tonemapVSを共用）
        ComPtr<ID3D11PixelShader> m_iblSpecularPrefilterPS;    //!< スペキュラプレフィルタ用PS（同上）
        ComPtr<ID3D11PixelShader> m_iblBRDFLUTPS;              //!< BRDF LUT生成用PS（同上）
        bool m_hasIBLBakeShaders = false;    //!< 上記3PSと3キューブ/LUTの生成がすべて成功したか

        ComPtr<ID3D11Buffer> m_iblBuffer;         //!< CBufferIBL用バッファ (b10)
        CBufferIBL           m_iblData{};         //!< CPU側のIBLパラメータ（ベイク完了時に1度だけ更新する）
        ComPtr<ID3D11Buffer> m_iblBakeBuffer;      //!< CBufferIBLBake用バッファ (b11, ベイク中だけ使う一時バッファ)

        bool m_iblBaked = false;    //!< スカイ由来のIBL（キャプチャ/irradiance/プレフィルタ）を焼き終えたか
    };
}    // namespace Tsukino::Renderer
