//----------------------------------------------------------------------------
//! @file   AmbientParticlePass.hpp
//! @brief  環境パーティクル（火の粉・灰）パスの宣言
//! @detail 頂点バッファもインデックスバッファも持たず、1回の Draw で粒子数×6頂点を出します。
//!         粒子の属性はすべて頂点シェーダーが SV_VertexID のハッシュから作るため、送るのは
//!         b10 のパラメータだけです。深度テストありの HDR バッファ描画なので世界の物体に隠れ、
//!         フォグとトーンマップの両方が乗ります。
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/Core/WindowsLean.hpp>
#include <Tsukino/Core/typedef.hpp>

#include <Tsukino/Renderer/ConstantBuffer.hpp>

#include <wrl/client.h>
#include <d3d11.h>

namespace Tsukino::Asset {
    class ShaderAsset;    // 前方宣言
}

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {
    class GraphicsContext;    // 前方宣言
    class RenderResources;    // 前方宣言
    class FrameConstants;     // 前方宣言

    //------------------------------------------------------------------------
    //! 環境パーティクルパスのクラスです。
    //! @note  Renderer が1つだけ所有し、Renderer::GetAmbientParticles() で借りる。
    //!        有効フラグはフレーム単位で、Render() の末尾で毎回 false へ戻る。
    //!        有効にしたいフレームでは毎フレーム SetEnabled(true) を呼ぶこと（AmbientParticleSystem の責務）
    //------------------------------------------------------------------------
    class AmbientParticlePass {
    public:

        //! 定数バッファ（b10）とシェーダーを作成します。
        //! @param  [in] graphicsContext デバイスの取得元
        //! @param  [in] resources       共通ステートの取得元
        //! @param  [in] frameConstants  b0 の取得元
        //! @param  [in] vs              環境パーティクル用の頂点シェーダー
        //! @param  [in] ps              環境パーティクル用のピクセルシェーダー
        //! @return true: 作成成功, false: 作成失敗（パスは何もしなくなる）
        [[nodiscard]]
        bool Initialize(GraphicsContext& graphicsContext, RenderResources& resources, FrameConstants& frameConstants,
                        const Tsukino::Asset::ShaderAsset* vs, const Tsukino::Asset::ShaderAsset* ps);

        //! 環境パーティクルのパラメータを設定します。
        //! @param  [in] params        環境パーティクルの定数バッファデータ
        //! @param  [in] particleCount 粒子数（kMaxAmbientParticles でクランプされる）
        void SetParameters(const CBufferAmbientParticle& params, u32 particleCount) noexcept {
            m_data  = params;
            m_count = (particleCount < kMaxAmbientParticles) ? particleCount : kMaxAmbientParticles;
        }

        //! このフレームで環境パーティクルを描くかを設定します。
        //! @param  [in] enabled true: 描く, false: 描かない
        void SetEnabled(bool enabled) noexcept {
            m_enabled = enabled;
        }

        //! 環境パーティクルを HDR バッファへ加算合成で描きます。
        //! @note   フォグパスの直前に呼ぶこと
        void Execute();

        //! フレームの終わりの処理をします（有効フラグを false へ戻します）。
        void EndFrame() noexcept {
            m_enabled = false;
        }

    private:
        GraphicsContext* m_graphicsContext = nullptr;    // デバイスの取得元（借りている）
        RenderResources* m_resources       = nullptr;    // 共通ステートの取得元（借りている）
        FrameConstants*  m_frameConstants  = nullptr;    // b0 の取得元（借りている）

        Microsoft::WRL::ComPtr<ID3D11VertexShader> m_vs;                 // 環境パーティクル用頂点シェーダー
        Microsoft::WRL::ComPtr<ID3D11PixelShader>  m_ps;                 // 環境パーティクル用ピクセルシェーダー
        Microsoft::WRL::ComPtr<ID3D11Buffer>       m_buffer;             // 環境パーティクルパラメータ用バッファ (b10)
        CBufferAmbientParticle                     m_data{};             // CPU側の環境パーティクルパラメータ
        u32                                        m_count   = 0;        // 今フレームの粒子数
        bool                                       m_enabled = false;    // このフレームで有効か
    };
}    // namespace Tsukino::Renderer
