//----------------------------------------------------------------------------
//! @file   SkyPass.hpp
//! @brief  スカイ（大気散乱）パスの宣言
//! @detail 画面全体を覆う三角形へ大気散乱のスカイを描きます。GBuffer パスの前に、
//!         深度を書かずに最背面として描きます。IBL ベイクのキャプチャも同じシェーダーを使います。
//----------------------------------------------------------------------------
#pragma once
#include <memory>

#include <Tsukino/Renderer/ConstantBuffer.hpp>


namespace Tsukino::Asset {
    class ShaderAsset;    // 前方宣言
}

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {
    class GraphicsContext;    // 前方宣言
    class RenderResources;    // 前方宣言
    class FrameConstants;     // 前方宣言

    //------------------------------------------------------------------------
    //! スカイパスのクラスです。
    //! @note  Renderer が1つだけ所有し、Renderer::GetSky() で借りる。
    //!        パイプラインは SkyAtmosphereSystem が SetPipeline() で確立し、
    //!        確立されるまではスカイも IBL ベイクも走らない
    //------------------------------------------------------------------------
    class SkyPass {
    public:
        struct Gpu;    //!< 描画 API の資源（Renderer の内部で定義）

        //! 資源を持たない状態で作ります（Initialize() で作成します）。
        SkyPass();

        //! 資源を破棄します。
        ~SkyPass();

        SkyPass(const SkyPass&)            = delete;
        SkyPass& operator=(const SkyPass&) = delete;

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

        //! スカイの定数バッファ（b4）を作成します。
        //! @param  [in] graphicsContext デバイスとデバイスコンテキストの取得元
        //! @param  [in] resources       共通ステートの取得元
        //! @param  [in] frameConstants  b0 の取得元
        //! @return true: 作成成功, false: 作成失敗
        [[nodiscard]]
        bool Initialize(GraphicsContext& graphicsContext, RenderResources& resources, FrameConstants& frameConstants);

        //! 大気散乱のパラメータを設定します。
        //! @param  [in] sky 大気散乱の定数バッファデータ
        void SetParameters(const CBufferSky& sky) noexcept {
            m_data = sky;
        }

        //! スカイのシェーダーを作成して、パスを有効にします。
        //! @param  [in] vs 頂点シェーダーアセット
        //! @param  [in] ps ピクセルシェーダーアセット
        void SetPipeline(const Tsukino::Asset::ShaderAsset* vs, const Tsukino::Asset::ShaderAsset* ps);

        //! パイプラインが確立済みかを取得します。
        //! @return true: スカイを描ける, false: まだ SetPipeline() されていない
        [[nodiscard]]
        bool IsReady() const noexcept {
            return m_hasPipeline;
        }

        //! スカイを描きます（パラメータを b4 へ転送してから描きます）。
        void Execute();

        //! 描画品質として空を描くかを設定します。
        //! @param  [in] allowed true: 描く, false: 描かない（背景はクリアカラーになる）
        //! @note   描かないときも定数バッファは更新するので、環境光（IBL）のベイクには影響しない
        void SetDrawAllowed(bool allowed) noexcept {
            m_drawAllowed = allowed;
        }




    private:
        GraphicsContext* m_graphicsContext = nullptr;    // デバイスとデバイスコンテキストの取得元（借りている）
        RenderResources* m_resources       = nullptr;    // 共通ステートの取得元（借りている）
        FrameConstants*  m_frameConstants  = nullptr;    // b0 の取得元（借りている）

        std::unique_ptr<Gpu> m_gpu;    // 描画 API の資源（中身は Renderer の内部で定義）
        CBufferSky                                 m_data{};    // スカイパラメータ
        bool                                       m_drawAllowed = true;     // 描画品質として空を描くか
        bool                                       m_hasPipeline = false;    // SetPipeline() でシェーダーを作れたか
    };
}    // namespace Tsukino::Renderer
