//----------------------------------------------------------------------------
//! @file   ShadowPass.hpp
//! @brief  シャドウマップパスの宣言
//! @detail Tsukino.Renderer の内部専用ヘッダです。シャドウマップ（テクスチャ・DSV・SRV・
//!         比較サンプラー）と影用パイプラインを持ち、GBuffer パスの形状を光源から見た深度で
//!         描きます。ディレクショナルライトの ViewProjection 行列の計算もここで行います。
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/Renderer/Renderer.hpp>

#include <memory>
#include <vector>

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {
    class DrawCommandExecutor;    // 前方宣言

    //------------------------------------------------------------------------
    //! シャドウマップパスのクラスです。
    //------------------------------------------------------------------------
    class ShadowPass {
    public:
        //! シャドウマップの一辺（ピクセル）。画面サイズに依存しないのでリサイズでは作り直さない
        static constexpr u32 kMapSize = 2048;

        //! シャドウマップと影用パイプラインを作成します。
        //! @param  [in] graphicsContext  デバイスとデバイスコンテキストの取得元
        //! @param  [in] resources        PipelineFactory の取得元
        //! @param  [in] frameConstants   b0 の転送先
        //! @param  [in] commandExecutor  描画コマンドの実行
        //! @param  [in] staticVS         スタティックメッシュ用のシャドウ頂点シェーダー
        //! @param  [in] skeletalVS       スケルタルメッシュ用のシャドウ頂点シェーダー
        //! @param  [in] ps               シャドウ用ピクセルシェーダー
        //! @return true: 作成成功, false: 作成失敗
        [[nodiscard]]
        bool Initialize(GraphicsContext&                   graphicsContext,
                        RenderResources&                   resources,
                        FrameConstants&                    frameConstants,
                        DrawCommandExecutor&               commandExecutor,
                        const Tsukino::Asset::ShaderAsset* staticVS,
                        const Tsukino::Asset::ShaderAsset* skeletalVS,
                        const Tsukino::Asset::ShaderAsset* ps);

        //! GBuffer パスの描画コマンドのうち、影を落とすものをシャドウマップへ描きます。
        //! @param  [in]     commands このフレームの描画コマンド
        //! @param  [in,out] stats    描画統計の加算先
        //! @return true: 描いた（レンダーターゲットとビューポートを書き換えたので呼び出し側で戻すこと）
        //!         false: 影用パイプラインが無いので何もしていない
        bool Execute(const std::vector<DrawCommand>& commands, Renderer::FrameStats& stats);

        //! シャドウマップ（t8）と比較サンプラー（s8）をピクセルシェーダーへバインドします。
        //! @param  [in] context デバイスコンテキスト
        void BindForSampling(ID3D11DeviceContext* context) const;

        //! ディレクショナルライトの ViewProjection 行列を求めます。
        //! @param  [in] normalizedDir 光の向き（正規化済み）
        //! @param  [in] focusPoint    シャドウマップの投影範囲（平行投影、±500ユニット）の中心
        //! @return ライト空間の ViewProjection 行列
        [[nodiscard]]
        static Tsukino::Core::Math::matrix ComputeLightViewProj(const hlslpp::float3& normalizedDir, const hlslpp::float3& focusPoint);

    private:

        //! シャドウマップ（テクスチャ・DSV・SRV・比較サンプラー）を作成します。
        //! @param  [in] device 描画デバイス
        //! @return true: 作成成功, false: 作成失敗
        [[nodiscard]]
        bool CreateShadowMap(ID3D11Device* device);

    private:
        GraphicsContext*     m_graphicsContext = nullptr;    // デバイスコンテキストの取得元（借りている）
        FrameConstants*      m_frameConstants  = nullptr;    // b0 の転送先（借りている）
        DrawCommandExecutor* m_commandExecutor = nullptr;    // 描画コマンドの実行（借りている）

        ComPtr<ID3D11Texture2D>          m_mapTex;     // シャドウマップテクスチャ
        ComPtr<ID3D11DepthStencilView>   m_mapDSV;     // シャドウマップ DSV（深度書き込み用）
        ComPtr<ID3D11ShaderResourceView> m_mapSRV;     // シャドウマップ SRV（PS でのサンプリング用）
        ComPtr<ID3D11SamplerState>       m_sampler;    // PCF 用比較サンプラー

        std::shared_ptr<PipelineState> m_staticPipeline;      // スタティックメッシュ用シャドウパイプライン
        std::shared_ptr<PipelineState> m_skeletalPipeline;    // スケルタルメッシュ用シャドウパイプライン
    };
}    // namespace Tsukino::Renderer
