//----------------------------------------------------------------------------
//! @file   IBLBaker.hpp
//! @brief  スカイ由来の環境光（IBL）のベイクの宣言
//! @detail スカイを6面のキューブマップへキャプチャし、拡散用の irradiance 畳み込みと
//!         鏡面用のスペキュラプレフィルタを行います。スカイに依存しない BRDF LUT は
//!         初期化時に一度だけ作ります。結果はライティングとフォワードのモデル描画が読みます。
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/Core/typedef.hpp>

#include <Tsukino/Renderer/ConstantBuffer.hpp>


#include <memory>

namespace Tsukino::Asset {
    class ShaderAsset;    // 前方宣言
}

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {
    class GraphicsContext;    // 前方宣言
    class RenderResources;    // 前方宣言
    class FrameConstants;     // 前方宣言
    class FullscreenPass;     // 前方宣言（Tsukino.Renderer の内部専用）
    class SkyPass;            // 前方宣言

    //------------------------------------------------------------------------
    //! スカイ由来の環境光（IBL）をベイクするクラスです。
    //! @note  Renderer が1つだけ所有し、Renderer::GetIBL() で借りる。
    //!        スカイのパイプラインが確立した最初のフレームで自動的に一度だけベイクする
    //------------------------------------------------------------------------
    class IBLBaker {
    public:
        struct Gpu;    //!< 描画 API の資源（Renderer の内部で定義）

        //! 資源を持たない状態で作ります（Initialize() で作成します）。
        IBLBaker();

        //! 資源を破棄します。
        ~IBLBaker();

        IBLBaker(const IBLBaker&)            = delete;
        IBLBaker& operator=(const IBLBaker&) = delete;

        //! 描画 API の資源を取得します（Renderer の中で使う。中身の定義は公開しない）。
        //! @return 資源への参照
        [[nodiscard]]
        Gpu& GetGpu() noexcept {
            return *m_gpu;
        }

        //! 描画 API の資源を取得します（Renderer の中で使う。中身の定義は公開しない）。
        //! @return 資源への参照
        [[nodiscard]]
        const Gpu& GetGpu() const noexcept {
            return *m_gpu;
        }

        //! 再ベイクを要求します。
        //! @note  太陽が動く演出（day-night等）が将来追加されたときは、そのシステムが
        //!        太陽角度の変化を検知してこれを呼べば次フレームで再ベイクされる。
        //!        ベイクは6〜48回のフルスクリーン三角形描画（すべて128px以下）を伴うため、
        //!        毎フレーム呼ぶような使い方はしないこと（呼び出し側でスロットリングする）
        void RequestRecapture() noexcept {
            m_baked = false;
        }

        //! 定数バッファ（b10 と、ベイク中だけ使う b11）を作成します。
        //! @param  [in] graphicsContext 描画デバイスの持ち主
        //! @return true: 作成成功, false: 作成失敗
        //! @note   IBL を使わないときもライティングが b10 を読むため、Initialize() の成否に関わらず必要
        [[nodiscard]]
        bool CreateConstantBuffers(const GraphicsContext& graphicsContext);

        //! キューブマップ・BRDF LUT・ベイク用シェーダーを作成し、BRDF LUT を焼きます。
        //! @param  [in] graphicsContext        デバイスとデバイスコンテキストの取得元
        //! @param  [in] resources              共通ステートとサンプラーの取得元
        //! @param  [in] frameConstants         b0 の取得元（キャプチャの視点を差し替えて書き込む）
        //! @param  [in] fullscreenPass         フルスクリーン三角形の描画
        //! @param  [in] skyPass                キャプチャで使うスカイのシェーダーと定数
        //! @param  [in] irradiancePS           拡散IBL用 irradiance 畳み込み PS
        //! @param  [in] specularPrefilterPS    鏡面IBL用プレフィルタ PS
        //! @param  [in] brdfLutPS              split-sum 用 BRDF 積分 LUT 生成 PS
        //! @return true: 成功, false: 失敗（IBL が定数0扱いになるだけで描画は継続できる）
        [[nodiscard]]
        bool Initialize(GraphicsContext&                   graphicsContext,
                        RenderResources&                   resources,
                        FrameConstants&                    frameConstants,
                        const FullscreenPass&              fullscreenPass,
                        const SkyPass&                     skyPass,
                        const Tsukino::Asset::ShaderAsset* irradiancePS,
                        const Tsukino::Asset::ShaderAsset* specularPrefilterPS,
                        const Tsukino::Asset::ShaderAsset* brdfLutPS);

        //! まだ焼いていなければベイクします（キャプチャ → irradiance → スペキュラプレフィルタ）。
        //! @return true: 焼いた, false: 何もしていない
        //! @note   スカイパスの直後に呼ぶこと（スカイの b4 がこのフレームの値で転送済みである必要がある）。
        //!         ビューポートは画面サイズへ戻すが、面ごとに書き換えた b0 は戻さないので、
        //!         呼び出し側が直後に FrameConstants::UploadWorld() で本来のカメラ値へ戻すこと
        bool BakeIfNeeded();

        //! IBL の結果（b10, t17〜t19, s10）をピクセルシェーダーへバインドします。
        //! @note  ディファードの Lighting パスと、フォワードの World/TransparentDepth/Transparent パスの
        //!        両方から呼ぶ（EvaluateIBL を呼ぶシェーダーがその2系統だけのため）
        void Bind();

        //! Bind() で張った SRV を解除します。
        void Unbind();

    private:

        //! 原点から見たキューブの1面ぶんのシーン定数を組み立てます。
        //! @param  [in] face キューブの面（0〜5：+X,-X,+Y,-Y,+Z,-Z）
        //! @return view/projection/viewProj/invViewProj/cameraPos を差し替えたシーン定数
        //!         （lightDir/lightColor/timeParams 等はワールドのシーン定数の値を引き継ぐ。
        //!          Sky.ps.hlsl が lightColor を太陽の色として読むため）
        [[nodiscard]]
        CBufferScene BuildCubeFaceSceneData(u32 face) const;

        //! スカイを6面のキューブマップへ焼きます（Sky.ps.hlsl を面ごとに視点だけ差し替えて使います）。
        void ExecuteCapturePass();

        //! 拡散IBL用の irradiance を畳み込みます（面ごとに1回）。
        void ExecuteIrradiancePass();

        //! 鏡面IBL用のスペキュラをプレフィルタします（面×mipごとに1回）。
        void ExecuteSpecularPrefilterPass();

        //! BRDF LUT を生成します（スカイに依存しないため起動時に1回だけ呼ぶ）。
        void ExecuteBRDFLUTPass();

    private:
        static constexpr u32 kCaptureSize      = 128;    // キャプチャキューブの1面の一辺（px）
        static constexpr u32 kIrradianceSize   = 32;     // irradiance キューブの1面の一辺（px）
        static constexpr u32 kSpecularBaseSize = 128;    // プレフィルタ済みスペキュラキューブの mip0 の一辺（px）
        static constexpr u32 kSpecularMipCount = 6;      // プレフィルタ済みスペキュラキューブのミップ数
        static constexpr u32 kBRDFLUTSize      = 128;    // BRDF LUT の一辺（px）

        GraphicsContext*      m_graphicsContext = nullptr;    // デバイスとデバイスコンテキストの取得元（借りている）
        RenderResources*      m_resources       = nullptr;    // 共通ステートとサンプラーの取得元（借りている）
        FrameConstants*       m_frameConstants  = nullptr;    // b0 の取得元（借りている）
        const FullscreenPass* m_fullscreenPass  = nullptr;    // フルスクリーン三角形の描画（借りている）
        const SkyPass*        m_skyPass         = nullptr;    // スカイのシェーダーと定数（借りている）

        std::unique_ptr<Gpu> m_gpu;    // 描画 API の資源（中身は Renderer の内部で定義）


        bool                                      m_hasBakeShaders = false; // 上記3PSと3キューブ/LUTの生成がすべて成功したか

        CBufferIBL                           m_data{};        // CPU側の IBL パラメータ（ベイク完了時に1度だけ更新する）

        bool m_baked = false;    // スカイ由来の IBL（キャプチャ/irradiance/プレフィルタ）を焼き終えたか
    };
}    // namespace Tsukino::Renderer
