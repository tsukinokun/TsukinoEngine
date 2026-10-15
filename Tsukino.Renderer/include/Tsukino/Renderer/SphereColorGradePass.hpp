//----------------------------------------------------------------------------
//! @file   SphereColorGradePass.hpp
//! @brief  球形の色調補正パスの宣言
//! @detail 深度からワールド座標を復元し、指定した球の内側だけ HDR バッファの色を補正します
//!         （Unreal の Post Process Volume＋SphereMask、Unity の Local Volume に相当）。
//!         HDR は SRV として読まず、デュアルソースブレンド（dest * src1 + src0）で
//!         乗算と加算を1回の描画で済ませるので、中間バッファを消費しません。
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
    //! 球形の色調補正パスのクラスです。
    //! @note  Renderer が1つだけ所有し、Renderer::GetSphereColorGrade() で借りる。
    //!        有効フラグはフレーム単位で、Render() の末尾で毎回 false へ戻る（FogPass と同じ）。
    //!        有効にしたいフレームでは毎フレーム SetEnabled(true) を呼ぶこと（SphereColorGradeSystem の責務）
    //------------------------------------------------------------------------
    class SphereColorGradePass {
    public:

        //! 定数バッファ（b8）・ブレンドステート・ピクセルシェーダーを作成します。
        //! @param  [in] graphicsContext デバイスと深度バッファの取得元
        //! @param  [in] resources       共通ステートとサンプラーの取得元
        //! @param  [in] frameConstants  b0 の取得元
        //! @param  [in] fullscreenPass  フルスクリーン三角形の描画
        //! @param  [in] ps              色調補正用ピクセルシェーダー（VS はフルスクリーン三角形用を共用）
        //! @return true: 作成成功, false: 作成失敗（パスは何もしなくなる）
        [[nodiscard]]
        bool Initialize(GraphicsContext& graphicsContext, RenderResources& resources, FrameConstants& frameConstants,
                        const FullscreenPass& fullscreenPass, const Tsukino::Asset::ShaderAsset* ps);

        //! 色調補正のパラメータを設定します。
        //! @param  [in] params 定数バッファデータ
        void SetParameters(const CBufferSphereColorGrade& params) noexcept {
            m_data = params;
        }

        //! このフレームで色調補正を掛けるかを設定します。
        //! @param  [in] enabled true: 掛ける, false: 掛けない
        void SetEnabled(bool enabled) noexcept {
            m_enabled = enabled;
        }

        //! 色調補正を HDR バッファへ掛けます。
        //! @note   フォグパスの直後・TransparentDepth パスの直前に呼ぶこと（理由は FogPass::Execute と同じ）。
        //!         終わると DSV が外れているので、続けて3Dを描くなら HDR＋DSV を張り直すこと
        void Execute();

        //! フレームの終わりの処理をします（有効フラグを false へ戻します）。
        void EndFrame() noexcept {
            m_enabled = false;
        }

    private:
        GraphicsContext*      m_graphicsContext = nullptr;    // デバイスと深度バッファの取得元（借りている）
        RenderResources*      m_resources       = nullptr;    // 共通ステートとサンプラーの取得元（借りている）
        FrameConstants*       m_frameConstants  = nullptr;    // b0 の取得元（借りている）
        const FullscreenPass* m_fullscreenPass  = nullptr;    // フルスクリーン三角形の描画（借りている）

        Microsoft::WRL::ComPtr<ID3D11PixelShader> m_ps;                 // 色調補正用ピクセルシェーダー
        Microsoft::WRL::ComPtr<ID3D11Buffer>      m_buffer;             // パラメータ用バッファ (b8)
        Microsoft::WRL::ComPtr<ID3D11BlendState>  m_blendState;         // dest * src1 + src0 のデュアルソースブレンド
        CBufferSphereColorGrade                   m_data{};             // CPU側のパラメータ
        bool                                      m_enabled = false;    // このフレームで有効か
    };
}    // namespace Tsukino::Renderer
