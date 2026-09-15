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
#include <Tsukino/Renderer/RenderResources.hpp>
#include <Tsukino/Renderer/FrameConstants.hpp>
#include <Tsukino/Renderer/DebugDraw.hpp>
#include <Tsukino/Renderer/SkyPass.hpp>
#include <Tsukino/Renderer/IBLBaker.hpp>
#include <Tsukino/Renderer/LightingPass.hpp>
#include <Tsukino/Renderer/AmbientParticlePass.hpp>
#include <Tsukino/Renderer/FogPass.hpp>
#include <Tsukino/Renderer/MotionBlurPass.hpp>
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
#include <memory>
#include <vector>

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {
    // ComPtr の using 宣言。公開ヘッダなのでグローバルではなく名前空間の内側に置く
    using Microsoft::WRL::ComPtr;

    class IPostWorldPass;         // 前方宣言（Worldパスの後に差し込む描画。実体は上位層が持つ）
    class DrawCommandExecutor;    // 前方宣言（描画コマンドの実行。Tsukino.Renderer の内部専用）
    class FullscreenPass;         // 前方宣言（フルスクリーン三角形の描画。同上）
    class TonemapPass;            // 前方宣言（トーンマップパス。同上）
    class ShadowPass;             // 前方宣言（シャドウマップパス。同上）

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
    //! @details DirectX11を使用してウィンドウに描画を行うクラス。
    //!          各パスと共有資源を所有し、Render() でパスの順番を回す。
    //!          パスごとの設定は Get*() で担当のクラスを借りて行う
    //!          （例: GetFog().SetParameters()、GetLighting().SetLights()）
    //------------------------------------------------------------
    class Renderer {
    public:
        //------------------------------------------------------------
        //! @brief コンストラクタ
        //------------------------------------------------------------
        Renderer();

        //------------------------------------------------------------
        //! @brief デストラクタ
        //! @note  内部専用の部品を前方宣言だけで持っているため、定義は Renderer.cpp に置く
        //------------------------------------------------------------
        ~Renderer();

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
        //! @brief  このフレームの描画コマンドキューを取得する
        //! @return 描画コマンドとマテリアルを受け付ける DrawCommandQueue
        //! @note   DrawCommand::material / materialData が指す実体は、必ずこのキューの
        //!         AllocMaterial() / AllocMaterialData() から取ること。System 側で持つと、
        //!         コマンドの寿命と食い違ってダングリングになる。
        //!         キューの中身は Render() の末尾で破棄される
        //------------------------------------------------------------
        [[nodiscard]]
        DrawCommandQueue& GetDrawQueue() noexcept {
            return m_drawQueue;
        }

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
        //! @brief  デバッグ用の線と三角形の描画を取得する
        //! @return 線・三角形を積んでまとめて描く DebugDraw
        //------------------------------------------------------------
        [[nodiscard]]
        DebugDraw& GetDebugDraw() noexcept {
            return m_debugDraw;
        }

        //------------------------------------------------------------
        //! @brief  描画で共有する資源を取得する
        //! @return 共通ステート・サンプラー・既定テクスチャ・テクスチャキャッシュ・
        //!         プリミティブメッシュ・PipelineFactory をまとめて持つ RenderResources
        //------------------------------------------------------------
        [[nodiscard]]
        RenderResources& GetResources() noexcept {
            return m_resources;
        }

        //------------------------------------------------------------
        //! @brief  フレーム単位のシーン定数（b0）を取得する
        //! @return カメラ行列・ライト・経過時間を持つ FrameConstants
        //------------------------------------------------------------
        [[nodiscard]]
        FrameConstants& GetFrameConstants() noexcept {
            return m_frameConstants;
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
        //! @brief  ディファードライティングパスを取得する
        //! @return ディレクショナルライトと点光源・スポットライトを受け付ける LightingPass
        //------------------------------------------------------------
        [[nodiscard]]
        LightingPass& GetLighting() noexcept {
            return m_lightingPass;
        }

        //------------------------------------------------------------
        //! @brief  スカイ（大気散乱）パスを取得する
        //! @return パラメータとシェーダーを受け付ける SkyPass
        //------------------------------------------------------------
        [[nodiscard]]
        SkyPass& GetSky() noexcept {
            return m_skyPass;
        }

        //------------------------------------------------------------
        //! @brief  スカイ由来の環境光（IBL）のベイクを取得する
        //! @return 再ベイクの要求を受け付ける IBLBaker
        //------------------------------------------------------------
        [[nodiscard]]
        IBLBaker& GetIBL() noexcept {
            return m_iblBaker;
        }

        //------------------------------------------------------------
        //! @brief  環境パーティクル（火の粉・灰）パスを取得する
        //! @return パラメータと有効フラグを受け付ける AmbientParticlePass
        //------------------------------------------------------------
        [[nodiscard]]
        AmbientParticlePass& GetAmbientParticles() noexcept {
            return m_ambientParticlePass;
        }

        //------------------------------------------------------------
        //! @brief  フォグパスを取得する
        //! @return パラメータと有効フラグを受け付ける FogPass
        //------------------------------------------------------------
        [[nodiscard]]
        FogPass& GetFog() noexcept {
            return m_fogPass;
        }

        //------------------------------------------------------------
        //! @brief  モーションブラーパスを取得する
        //! @return パラメータと有効フラグを受け付ける MotionBlurPass
        //------------------------------------------------------------
        [[nodiscard]]
        MotionBlurPass& GetMotionBlur() noexcept {
            return m_motionBlurPass;
        }

    private:
        // DirectX 11の主要なインターフェース
        //! @note 【この宣言順は破棄順序の設計であり、並べ替えてはならない】
        //!       以降の部品はすべて m_graphicsContext が持つデバイスを借りて資源を作るため、
        //!       デバイスを最後まで生かすよう先頭に置く
        GraphicsContext m_graphicsContext;    // グラフィックスコンテキスト（Device, DeviceContext, SwapChainを管理）
        RenderResources m_resources;          // 描画で共有する資源（ステート・サンプラー・既定テクスチャ・テクスチャキャッシュ等）
        FrameConstants  m_frameConstants;     // フレーム単位のシーン定数（b0）

        std::unique_ptr<DrawCommandExecutor> m_commandExecutor;    // 描画コマンドの実行（上の3つを借りるので、その後に宣言する）
        DebugDraw                            m_debugDraw;          // デバッグ用の線と三角形（同上）
        std::unique_ptr<FullscreenPass>      m_fullscreenPass;     // フルスクリーン三角形の描画（ライティング・フォグ・モーションブラー・トーンマップ・IBLで共用）
        std::unique_ptr<TonemapPass>         m_tonemapPass;        // トーンマップパス（m_fullscreenPass を借りるので、その後に宣言する）
        std::unique_ptr<ShadowPass>          m_shadowPass;         // シャドウマップパス（m_commandExecutor を借りる）
        SkyPass                              m_skyPass;            // スカイ（大気散乱）パス
        IBLBaker                             m_iblBaker;           // スカイ由来の環境光（m_fullscreenPass と m_skyPass を借りるので、その後に宣言する）
        LightingPass                         m_lightingPass;       // ディファードライティング（m_shadowPass と m_iblBaker を借りるので、その後に宣言する）
        AmbientParticlePass                  m_ambientParticlePass;    // 環境パーティクル
        FogPass                              m_fogPass;                // フォグ（m_fullscreenPass を借りる）
        MotionBlurPass                       m_motionBlurPass;         // モーションブラー（同上）

        std::array<float, 4> m_clearColor = {0.5f, 0.5f, 0.5f, 1.0f};    // 描画領域のクリアカラー (デフォルトはグレー)

        FrameStats m_frameStats;    // 1フレーム分の描画統計（Render()の先頭でリセットする）

        DrawCommandQueue m_drawQueue;    // 描画コマンドキュー

        //! Overlayパスの実行順を決めるための添字バッファ。
        //! DrawCommand本体ではなく添字を並べ替えるのは、DrawCommandがhlsl++の
        //! 16バイト境界型（matrix）を含み、並べ替えの一時バッファが拡張アライメント
        //! として弾かれるため（SpriteRenderSystem/FontRendererSystemと同じ理由）。
        //! メンバに持たせてclear()で使い回し、毎フレームの確保を避ける
        std::vector<u32> m_overlayOrder;

    };
}    // namespace Tsukino::Renderer
