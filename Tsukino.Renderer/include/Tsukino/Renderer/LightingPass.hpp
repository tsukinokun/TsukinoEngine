//----------------------------------------------------------------------------
//! @file   LightingPass.hpp
//! @brief  ディファードライティングパスの宣言
//! @detail G-Buffer と深度をもとに、ディレクショナルライト・点光源・スポットライト・
//!         スカイ由来の環境光（IBL）を1回のフルスクリーン描画で HDR バッファへ加算します。
//----------------------------------------------------------------------------
#pragma once
#include <memory>
#include <Tsukino/Core/typedef.hpp>

#include <Tsukino/Renderer/ConstantBuffer.hpp>


#include <hlsl++.h>

namespace Tsukino::Asset {
    class ShaderAsset;    // 前方宣言
}

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {
    class GraphicsContext;    // 前方宣言
    class RenderResources;    // 前方宣言
    class FrameConstants;     // 前方宣言
    class FullscreenPass;     // 前方宣言（Tsukino.Renderer の内部専用）
    class ShadowPass;         // 前方宣言（同上）
    class IBLBaker;           // 前方宣言

    //------------------------------------------------------------------------
    //! ディファードライティングパスのクラスです。
    //! @note  Renderer が1つだけ所有し、Renderer::GetLighting() で借りる。
    //!        ライトの設定は LightSystem が毎フレーム行う
    //------------------------------------------------------------------------
    class LightingPass {
    public:
        struct Gpu;    //!< 描画 API の資源（Renderer の内部で定義）

        //! 資源を持たない状態で作ります（Initialize() で作成します）。
        LightingPass();

        //! 資源を破棄します。
        ~LightingPass();

        LightingPass(const LightingPass&)            = delete;
        LightingPass& operator=(const LightingPass&) = delete;

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

        //! ライト配列の定数バッファ（b6）とピクセルシェーダーを作成します。
        //! @param  [in] graphicsContext デバイスと G-Buffer の取得元
        //! @param  [in] resources       共通ステートとサンプラーの取得元
        //! @param  [in] frameConstants  b0 の取得元・ディレクショナルライトの書き込み先
        //! @param  [in] fullscreenPass  フルスクリーン三角形の描画
        //! @param  [in] shadowPass      シャドウマップ
        //! @param  [in] iblBaker        スカイ由来の環境光
        //! @param  [in] ps              ライティング用のピクセルシェーダー（VS はフルスクリーン三角形用を共用）
        //! @return true: 作成成功, false: 作成失敗
        [[nodiscard]]
        bool Initialize(GraphicsContext&                   graphicsContext,
                        RenderResources&                   resources,
                        FrameConstants&                    frameConstants,
                        const FullscreenPass&              fullscreenPass,
                        const ShadowPass&                  shadowPass,
                        IBLBaker&                          iblBaker,
                        const Tsukino::Asset::ShaderAsset* ps);

        //! ディレクショナルライトを設定します。
        //! @param  [in] direction ライトの方向（正規化推奨）
        //! @param  [in] color     ライトの色
        //! @param  [in] intensity ライトの強度
        //! @note   影の範囲は渡さなくてよい。描画の直前に、メインカメラの視錐台を
        //!         奥行きで切り分けたカスケードをレンダラーが作る
        void SetDirectionalLight(const hlslpp::float3& direction, const hlslpp::float3& color, float intensity);

        //! 点光源・スポットライトの配列を設定します。
        //! @param  [in] lights GPULight の配列
        //! @param  [in] count  配列の要素数（MAX_LIGHTS を超える分は切り捨てられる）
        void SetLights(const GPULight* lights, u32 count);

        //! ライティングを実行して HDR バッファへ加算します。
        //! @note   深度0（スカイパスが描いた背景）は PS 側で discard して保護する
        void Execute();

    private:
        GraphicsContext*      m_graphicsContext = nullptr;    // デバイスと G-Buffer の取得元（借りている）
        RenderResources*      m_resources       = nullptr;    // 共通ステートとサンプラーの取得元（借りている）
        FrameConstants*       m_frameConstants  = nullptr;    // b0 の取得元（借りている）
        const FullscreenPass* m_fullscreenPass  = nullptr;    // フルスクリーン三角形の描画（借りている）
        const ShadowPass*     m_shadowPass      = nullptr;    // シャドウマップ（借りている）
        IBLBaker*             m_iblBaker        = nullptr;    // スカイ由来の環境光（借りている）

        std::unique_ptr<Gpu> m_gpu;    // 描画 API の資源（中身は Renderer の内部で定義）
        CBufferLights                             m_lightsData{};              // CPU側のライト配列（毎フレームGPUへ転送）
        bool                                      m_lightOverflowWarned = false;    // MAX_LIGHTS 超過の警告を1回だけ出すためのフラグ
    };
}    // namespace Tsukino::Renderer
