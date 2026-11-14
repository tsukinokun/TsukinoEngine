//----------------------------------------------------------------------------
//! @file   ShadowPass.hpp
//! @brief  シャドウマップパスの宣言
//! @detail Tsukino.Renderer の内部専用ヘッダです。シャドウマップ（テクスチャ・DSV・SRV・
//!         比較サンプラー）と影用パイプラインを持ち、GBuffer パスの形状を光源から見た深度で
//!         描きます。カスケード（カメラの視錐台を奥行きで切った1切れごとのシャドウマップ）の
//!         ライト空間 ViewProjection 行列の計算は ShadowCascades.hpp にあります。
//----------------------------------------------------------------------------
#pragma once
#include <wrl/client.h>
#include <Tsukino/Renderer/Renderer.hpp>
#include <Tsukino/Renderer/ShadowCascades.hpp>

#include <array>
#include <memory>
#include <vector>

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {
    using Microsoft::WRL::ComPtr;    // Renderer の内部用ヘッダなので、名前空間の中で使えるようにしておく

    class DrawCommandExecutor;    // 前方宣言

    //------------------------------------------------------------------------
    //! シャドウマップパスのクラスです。
    //------------------------------------------------------------------------
    class ShadowPass {
    public:
        //! カスケードの枚数。b0のcascadeViewProjの要素数と同じでなければならない
        static constexpr u32 kCascadeCount = kShadowCascadeCount;

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

        //! このフレームのカスケードを求めて、ワールドのシーン定数（b0）へ書き込みます。
        //! @param  [in] worldScene ワールドのシーン定数。view / invViewProj（メインカメラ）と lightDir を使う
        //! @note   カメラとライトが確定した後、Execute() とライティングより前に毎フレーム呼ぶこと。
        //!         ライトの向きが設定されていない（長さ0）ときは何もしない
        void UpdateCascades(const CBufferScene& worldScene);

        //! GBuffer パスの描画コマンドのうち、影を落とすものをシャドウマップへ描きます。
        //! @param  [in]     commands このフレームの描画コマンド
        //! @param  [in,out] stats    描画統計の加算先
        //! @return true: 描いた（レンダーターゲットとビューポートを書き換えたので呼び出し側で戻すこと）
        //!         false: 影用パイプラインが無いので何もしていない
        //! @note   先に UpdateCascades() を呼んでおくこと
        bool Execute(const std::vector<DrawCommand>& commands, Renderer::FrameStats& stats);

        //! シャドウマップ（t8）と比較サンプラー（s8）をピクセルシェーダーへバインドします。
        //! @param  [in] context デバイスコンテキスト
        void BindForSampling(ID3D11DeviceContext* context) const;

        //! シャドウパスでカスケードの範囲外にあるオブジェクトを間引くかを切り替えます。
        //! @note 既定は有効。同じビルドで有効・無効の両方を計測できるようにするための口で、
        //!       見た目は変わらない前提（変わるなら間引きすぎている）
        void SetCullingEnabled(bool enabled) { m_cullingEnabled = enabled; }

    private:

        //! シーンの調整値へ描画品質の上限を掛けて適用します。
        //! @note   mapSize が変わったときだけシャドウマップを作り直す
        void ApplySettings();

        //! シャドウマップ（テクスチャ・DSV・SRV・比較サンプラー）を作成します。
        //! @param  [in] device  描画デバイス
        //! @param  [in] mapSize シャドウマップの一辺（ピクセル）
        //! @return true: 作成成功, false: 作成失敗
        [[nodiscard]]
        bool CreateShadowMap(ID3D11Device* device, u32 mapSize);

    private:
        GraphicsContext*     m_graphicsContext = nullptr;    // デバイスコンテキストの取得元（借りている）
        FrameConstants*      m_frameConstants  = nullptr;    // b0 の転送先（借りている）
        DrawCommandExecutor* m_commandExecutor = nullptr;    // 描画コマンドの実行（借りている）

        ComPtr<ID3D11Texture2D>          m_mapTex;     // シャドウマップテクスチャ（kCascadeCount枚の配列）
        ComPtr<ID3D11ShaderResourceView> m_mapSRV;     // 配列全体の SRV（PS でのサンプリング用。t8）
        ComPtr<ID3D11SamplerState>       m_sampler;    // PCF 用比較サンプラー

        // スライスごとの DSV。書き込みは1枚ずつなので、配列要素を個別に指すビューが要る
        std::array<ComPtr<ID3D11DepthStencilView>, kCascadeCount> m_mapDSV;

        // このフレームのカスケード（UpdateCascades() が埋め、Execute() が使う）
        std::array<ShadowCascade, kCascadeCount> m_cascades{};

        ShadowSettings m_requestedSettings{};               // シーンから渡された調整値（品質の上限を掛ける前）
        ShadowSettings m_settings{};                        // 影の調整値（SanitizeSettings() で丸め、品質の上限を掛けた後）
        u32            m_mapSizeLimit       = 0;            // 描画品質によるシャドウマップの一辺の上限（0 で上限なし）
        u32            m_activeCascadeCount = kCascadeCount;    // 実際に描くカスケードの枚数

        bool m_cullingEnabled = true;    // カスケードの範囲外を間引くか（計測用に切り替えられる）

        std::shared_ptr<PipelineState> m_staticPipeline;      // スタティックメッシュ用シャドウパイプライン
        std::shared_ptr<PipelineState> m_skeletalPipeline;    // スケルタルメッシュ用シャドウパイプライン
    };
}    // namespace Tsukino::Renderer
