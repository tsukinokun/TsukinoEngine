//----------------------------------------------------------------------------
//! @file   FogPass.hpp
//! @brief  フォグパスの宣言
//! @detail 深度バッファだけを読み、距離フォグ・高さフォグを HDR バッファへプリマルチプライの
//!         over 合成で書き込みます。HDR を SRV として読まないので中間バッファを消費しません。
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
    class FullscreenPass;     // 前方宣言（Tsukino.Renderer の内部専用）

    //------------------------------------------------------------------------
    //! フォグパスのクラスです。
    //! @note  Renderer が1つだけ所有し、Renderer::GetFog() で借りる。
    //!        有効フラグはフレーム単位で、Render() の末尾で毎回 false へ戻る。
    //!        有効にしたいフレームでは毎フレーム SetEnabled(true) を呼ぶこと（FogSystem の責務）
    //------------------------------------------------------------------------
    class FogPass {
    public:

        //! 定数バッファ（b9）とピクセルシェーダーを作成します。
        //! @param  [in] graphicsContext デバイスと深度バッファの取得元
        //! @param  [in] resources       共通ステートとサンプラーの取得元
        //! @param  [in] frameConstants  b0 の取得元
        //! @param  [in] fullscreenPass  フルスクリーン三角形の描画
        //! @param  [in] ps              フォグ用ピクセルシェーダー（VS はフルスクリーン三角形用を共用）
        //! @return true: 作成成功, false: 作成失敗（パスは何もしなくなる）
        [[nodiscard]]
        bool Initialize(GraphicsContext& graphicsContext, RenderResources& resources, FrameConstants& frameConstants,
                        const FullscreenPass& fullscreenPass, const Tsukino::Asset::ShaderAsset* ps);

        //! フォグのパラメータを設定します。
        //! @param  [in] params フォグの定数バッファデータ
        void SetParameters(const CBufferFog& params) noexcept {
            m_data = params;
        }

        //! このフレームでフォグを掛けるかを設定します。
        //! @param  [in] enabled true: 掛ける, false: 掛けない
        void SetEnabled(bool enabled) noexcept {
            m_enabled = enabled;
        }

        //! フォグを HDR バッファへ合成します。
        //! @note   Transparent パスの直後・モーションブラーパスの直前に呼ぶこと
        void Execute();

        //! フレームの終わりの処理をします（有効フラグを false へ戻します）。
        //! @note   毎フレーム FogSystem が再度 true にする前提にしておくと、FogSystem を持たない
        //!         シーンへ切り替えたときにフラグが立ちっぱなしで残らない
        void EndFrame() noexcept {
            m_enabled = false;
        }

    private:
        GraphicsContext*      m_graphicsContext = nullptr;    // デバイスと深度バッファの取得元（借りている）
        RenderResources*      m_resources       = nullptr;    // 共通ステートとサンプラーの取得元（借りている）
        FrameConstants*       m_frameConstants  = nullptr;    // b0 の取得元（借りている）
        const FullscreenPass* m_fullscreenPass  = nullptr;    // フルスクリーン三角形の描画（借りている）

        Microsoft::WRL::ComPtr<ID3D11PixelShader> m_ps;                 // フォグ用ピクセルシェーダー
        Microsoft::WRL::ComPtr<ID3D11Buffer>      m_buffer;             // フォグパラメータ用バッファ (b9)
        CBufferFog                                m_data{};             // CPU側のフォグパラメータ
        bool                                      m_enabled = false;    // このフレームで有効か
    };
}    // namespace Tsukino::Renderer
