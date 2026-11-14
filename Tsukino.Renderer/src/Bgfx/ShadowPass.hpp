//----------------------------------------------------------------------------
//! @file   ShadowPass.hpp
//! @brief  bgfx 版のシャドウマップパスの宣言
//! @detail Tsukino.Renderer の内部専用ヘッダです。シャドウマップ（カスケード1枚を1層に持つ
//!         配列の深度テクスチャ）と影用パイプラインを持ち、GBuffer パスの形状を光源から見た深度で
//!         描きます。カスケードの投影の計算は ShadowCascades.hpp にあります。
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/Renderer/Renderer.hpp>
#include <Tsukino/Renderer/ShadowCascades.hpp>

#include <bgfx/bgfx.h>

#include <array>
#include <memory>
#include <vector>

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {
    class DrawCommandExecutor;    // 前方宣言

    //------------------------------------------------------------------------
    //! シャドウマップパスのクラスです（bgfx 版）。
    //------------------------------------------------------------------------
    class ShadowPass {
    public:
        //! カスケードの枚数。b0のcascadeViewProjの要素数と同じでなければならない
        static constexpr u32 kCascadeCount = ShadowCascades::kCascadeCount;

        //! 資源を持たない状態で作ります。
        ShadowPass();

        //! シャドウマップとサンプラーを破棄します。
        ~ShadowPass();

        //! シャドウマップと影用パイプラインを作成します。
        //! @param  [in] resources       PipelineFactory の取得元
        //! @param  [in] frameConstants  シーン定数の転送先
        //! @param  [in] commandExecutor 描画コマンドの実行
        //! @param  [in] staticVS        スタティックメッシュ用のシャドウ頂点シェーダー
        //! @param  [in] skeletalVS      スケルタルメッシュ用のシャドウ頂点シェーダー
        //! @param  [in] ps              シャドウ用ピクセルシェーダー
        //! @return true: 作成成功, false: 作成失敗
        [[nodiscard]]
        bool Initialize(RenderResources&                   resources,
                        FrameConstants&                    frameConstants,
                        DrawCommandExecutor&               commandExecutor,
                        const Tsukino::Asset::ShaderAsset* staticVS,
                        const Tsukino::Asset::ShaderAsset* skeletalVS,
                        const Tsukino::Asset::ShaderAsset* ps);

        //! 影の調整値を設定します。
        //! @param  [in] settings 影の調整値。範囲外の値は使える範囲へ丸める
        //! @note   mapSize が変わったときだけシャドウマップを作り直す。作り直しに失敗したら前の解像度のまま残す
        void SetSettings(const ShadowSettings& settings);

        //! 描画品質による影の上限を設定します。
        //! @param  [in] mapSizeLimit シャドウマップの一辺の上限（0 で上限なし）
        //! @param  [in] cascadeCount 実際に描くカスケードの枚数（1〜kCascadeCount に丸める）
        //! @note   シーン側（LightSystem）が毎フレーム渡す調整値とは別に持ち、そちらの値に上限を掛ける
        void SetQuality(u32 mapSizeLimit, u32 cascadeCount);

        //! 今の影の調整値（丸めた後）を取得します。
        //! @return 影の調整値
        [[nodiscard]]
        const ShadowSettings& GetSettings() const noexcept {
            return m_settings;
        }

        //! このフレームのカスケードを求めて、ワールドのシーン定数へ書き込みます。
        //! @param  [in] worldScene ワールドのシーン定数。view / invViewProj（メインカメラ）と lightDir を使う
        //! @note   カメラとライトが確定した後、Execute() とライティングより前に毎フレーム呼ぶこと。
        //!         ライトの向きが設定されていない（長さ0）ときは何もしない
        void UpdateCascades(const CBufferScene& worldScene);

        //! GBuffer パスの描画コマンドのうち、影を落とすものをシャドウマップへ描きます。
        //! @param  [in]     commands このフレームの描画コマンド
        //! @param  [in,out] stats    描画統計の加算先
        //! @note   先に UpdateCascades() を呼んでおくこと。描いているカスケードの番号をシーン定数へ
        //!         書くので、呼び出し側はこの後でワールドのシーン定数を送り直すこと
        void Execute(const std::vector<DrawCommand>& commands, Renderer::FrameStats& stats);

        //! 次に Submit する描画へ、シャドウマップを張ります。
        void BindForSampling() const;

        //! シャドウパスでカスケードの範囲外にあるオブジェクトを間引くかを切り替えます。
        //! @param  [in] enabled true で間引く
        //! @note   既定は有効。同じビルドで有効・無効の両方を計測できるようにするための口で、
        //!         見た目は変わらない前提（変わるなら間引きすぎている）
        void SetCullingEnabled(bool enabled) {
            m_cullingEnabled = enabled;
        }

    private:
        //! シーンの調整値へ描画品質の上限を掛けて適用します。
        //! @note   mapSize が変わったときだけシャドウマップを作り直す
        void ApplySettings();

        //! シャドウマップ（配列の深度テクスチャと、層ごとのフレームバッファ）を作成します。
        //! @param  [in] mapSize シャドウマップの一辺（ピクセル）
        //! @return true: 作成成功, false: 作成失敗（前のシャドウマップはそのまま残る）
        [[nodiscard]]
        bool CreateShadowMap(u32 mapSize);

    private:
        FrameConstants*      m_frameConstants  = nullptr;    // シーン定数の転送先（借りている）
        DrawCommandExecutor* m_commandExecutor = nullptr;    // 描画コマンドの実行（借りている）

        bgfx::TextureHandle m_mapTexture = BGFX_INVALID_HANDLE;    // シャドウマップ（kCascadeCount 層の配列。比較サンプラー付き）
        bgfx::UniformHandle m_sampler    = BGFX_INVALID_HANDLE;    // s_shadowMap

        // 層ごとのフレームバッファ。書き込みは1枚ずつなので、層を個別に指すものが要る
        std::array<bgfx::FrameBufferHandle, kCascadeCount> m_frameBuffers{};

        // このフレームのカスケード（UpdateCascades() が埋め、Execute() が使う）
        std::array<ShadowCascade, kCascadeCount> m_cascades{};

        ShadowSettings m_requestedSettings{};                   // シーンから渡された調整値（品質の上限を掛ける前）
        ShadowSettings m_settings{};                            // 影の調整値（丸めて、品質の上限を掛けた後）
        u32            m_mapSizeLimit       = 0;                // 描画品質によるシャドウマップの一辺の上限（0 で上限なし）
        u32            m_activeCascadeCount = kCascadeCount;    // 実際に描くカスケードの枚数

        bool m_cullingEnabled = true;    // カスケードの範囲外を間引くか（計測用に切り替えられる）

        std::shared_ptr<PipelineState> m_staticPipeline;      // スタティックメッシュ用シャドウパイプライン
        std::shared_ptr<PipelineState> m_skeletalPipeline;    // スケルタルメッシュ用シャドウパイプライン
    };
}    // namespace Tsukino::Renderer
