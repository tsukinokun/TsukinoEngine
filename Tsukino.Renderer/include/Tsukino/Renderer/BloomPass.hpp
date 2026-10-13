//----------------------------------------------------------------------------
//! @file   BloomPass.hpp
//! @brief  ブルームパスの宣言
//! @detail トーンマップ直前の HDR の絵から閾値を超えた明るさだけを取り出し、
//!         縮小しながらぼかして（1/2, 1/4, ... の段）拡大しながら足し戻し、
//!         最後にトーンマップの入力へ加算します。UI とエフェクトはトーンマップ後に
//!         バックバッファへ直接描かれるため、ブルームの影響を受けません。
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/Core/WindowsLean.hpp>
#include <Tsukino/Core/typedef.hpp>

#include <Tsukino/Renderer/ConstantBuffer.hpp>

#include <wrl/client.h>
#include <d3d11.h>

#include <array>

namespace Tsukino::Asset {
    class ShaderAsset;    // 前方宣言
}

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {
    class GraphicsContext;    // 前方宣言
    class RenderResources;    // 前方宣言
    class FullscreenPass;     // 前方宣言（Tsukino.Renderer の内部専用）

    //------------------------------------------------------------------------
    //! @struct BloomParameters
    //! @brief  ブルームの効き方（BloomSystem が BloomComponent から毎フレーム詰める）
    //------------------------------------------------------------------------
    struct BloomParameters {
        float threshold = 1.0f;    //!< これを超えた明るさだけが滲む（HDR。1.0 = 白）
        float softKnee  = 0.5f;    //!< 閾値付近をなだらかにする幅（0 で段差がくっきり出る）
        float intensity = 0.6f;    //!< 滲みを元の絵へ足す強さ
        float radius    = 1.0f;    //!< 拡大時のぼかし半径の倍率（大きいほど遠くまで広がる）
        float tintR     = 1.0f;    //!< 滲みの色味（R）
        float tintG     = 1.0f;    //!< 滲みの色味（G）
        float tintB     = 1.0f;    //!< 滲みの色味（B）
    };

    //------------------------------------------------------------------------
    //! ブルームパスのクラスです。
    //! @note  Renderer が1つだけ所有し、Renderer::GetBloom() で借りる。
    //!        有効フラグはフレーム単位で、Render() の末尾で毎回 false へ戻る。
    //!        有効にしたいフレームでは毎フレーム SetEnabled(true) を呼ぶこと（BloomSystem の責務）。
    //!
    //!        縮小段のテクスチャは画面サイズに依存するが、GraphicsContext には持たせず
    //!        このパスが自分で持つ。Execute の先頭で画面サイズの変化を見て作り直すので、
    //!        Renderer::Resize からの通知は要らない
    //------------------------------------------------------------------------
    class BloomPass {
    public:
        //! 縮小段の最大数（1/2 から 1/64 まで）。画面が小さいときは 2px を切る手前で打ち切る
        static constexpr u32 kMaxMipCount = 6;

        //! 定数バッファとピクセルシェーダーを作成します。
        //! @param  [in] graphicsContext デバイス・HDR バッファ・中間バッファの取得元
        //! @param  [in] resources       共通ステートとサンプラーの取得元
        //! @param  [in] fullscreenPass  フルスクリーン三角形の描画
        //! @param  [in] prefilterPS     明部抽出 + 最初の縮小 用ピクセルシェーダー
        //! @param  [in] downsamplePS    縮小用ピクセルシェーダー
        //! @param  [in] upsamplePS      拡大（と最後の合成）用ピクセルシェーダー
        //! @return true: 作成成功, false: 作成失敗（パスは何もしなくなる）
        [[nodiscard]]
        bool Initialize(GraphicsContext& graphicsContext, RenderResources& resources, const FullscreenPass& fullscreenPass,
                        const Tsukino::Asset::ShaderAsset* prefilterPS, const Tsukino::Asset::ShaderAsset* downsamplePS,
                        const Tsukino::Asset::ShaderAsset* upsamplePS);

        //! ブルームのパラメータを設定します。
        //! @param  [in] params ブルームの効き方
        void SetParameters(const BloomParameters& params) noexcept {
            m_params = params;
        }

        //! このフレームでブルームを掛けるかを設定します。
        //! @param  [in] enabled true: 掛ける, false: 掛けない
        void SetEnabled(bool enabled) noexcept {
            m_enabled = enabled;
        }

        //! このフレームでブルームが有効かを取得します。
        //! @return true: 有効, false: 無効
        [[nodiscard]]
        bool IsEnabled() const noexcept {
            return m_enabled;
        }

        //! ブルームを掛けて、トーンマップの入力になるバッファへ加算します。
        //! @param  [in] sourceIsPostProcess true: モーションブラーが中間バッファへ書いた後,
        //!                                  false: HDR バッファがそのまま最新
        //! @note   モーションブラーパスの直後・トーンマップパスの直前に呼ぶこと。
        //!         読み書きする先は同じバッファ（加算で足し戻す）なので、トーンマップへ渡す SRV は変わらない
        void Execute(bool sourceIsPostProcess);

        //! フレームの終わりの処理をします（有効フラグを false へ戻します）。
        void EndFrame() noexcept {
            m_enabled = false;
        }

    private:
        //! 縮小段のテクスチャを画面サイズに合わせて作り直します。
        //! @return true: 使える縮小段が1段以上ある, false: 作成失敗
        [[nodiscard]]
        bool EnsureMipTargets(UINT width, UINT height);

        //! 定数バッファを書き換えてフルスクリーン三角形を1枚描きます。
        //! @param  [in] source       読むテクスチャ（t0）
        //! @param  [in] sourceWidth  読むテクスチャの幅（テクセルの大きさを求めるため）
        //! @param  [in] sourceHeight 読むテクスチャの高さ
        //! @param  [in] targetWidth  書き込み先の幅（ビューポート）
        //! @param  [in] targetHeight 書き込み先の高さ（ビューポート）
        //! @param  [in] data         定数バッファの中身（texelSize はこの関数が埋める）
        void DrawFullscreen(ID3D11ShaderResourceView* source, UINT sourceWidth, UINT sourceHeight, UINT targetWidth, UINT targetHeight,
                            CBufferBloom data);

        //! 縮小段1枚分のテクスチャ
        struct MipTarget {
            Microsoft::WRL::ComPtr<ID3D11Texture2D>          texture;
            Microsoft::WRL::ComPtr<ID3D11RenderTargetView>   rtv;
            Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv;
            UINT                                             width  = 0;
            UINT                                             height = 0;
        };

        GraphicsContext*      m_graphicsContext = nullptr;    // デバイス・HDR バッファ・中間バッファの取得元（借りている）
        RenderResources*      m_resources       = nullptr;    // 共通ステートとサンプラーの取得元（借りている）
        const FullscreenPass* m_fullscreenPass  = nullptr;    // フルスクリーン三角形の描画（借りている）

        Microsoft::WRL::ComPtr<ID3D11PixelShader> m_prefilterPS;     // 明部抽出 + 最初の縮小
        Microsoft::WRL::ComPtr<ID3D11PixelShader> m_downsamplePS;    // 縮小
        Microsoft::WRL::ComPtr<ID3D11PixelShader> m_upsamplePS;      // 拡大と合成
        Microsoft::WRL::ComPtr<ID3D11Buffer>      m_buffer;          // パラメータ用バッファ（CBSlot::Bloom）

        std::array<MipTarget, kMaxMipCount> m_mips{};             // 縮小段（[0] が 1/2）
        u32                                 m_mipCount     = 0;    // 実際に使う縮小段の数
        UINT                                m_sourceWidth  = 0;    // 縮小段を作ったときの画面幅
        UINT                                m_sourceHeight = 0;    // 縮小段を作ったときの画面高さ

        BloomParameters m_params{};          // ブルームの効き方
        bool            m_enabled = false;    // このフレームで有効か
    };
}    // namespace Tsukino::Renderer
