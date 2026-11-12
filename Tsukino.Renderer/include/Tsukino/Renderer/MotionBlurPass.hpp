//----------------------------------------------------------------------------
//! @file   MotionBlurPass.hpp
//! @brief  モーションブラーパスの宣言
//! @detail HDR バッファと速度バッファを読み、ポストプロセス用の中間バッファへブラーを掛けた絵を
//!         書き出します。UI とエフェクトはトーンマップ後にバックバッファへ直接描かれるため、
//!         ブラーの影響を受けません。
//----------------------------------------------------------------------------
#pragma once
#include <memory>
#include <Tsukino/Core/typedef.hpp>

#include <Tsukino/Renderer/ConstantBuffer.hpp>


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
    //! モーションブラーパスのクラスです。
    //! @note  Renderer が1つだけ所有し、Renderer::GetMotionBlur() で借りる。
    //!        有効フラグはフレーム単位で、Render() の末尾で毎回 false へ戻る。
    //!        有効にしたいフレームでは毎フレーム SetEnabled(true) を呼ぶこと（MotionBlurSystem の責務）
    //------------------------------------------------------------------------
    class MotionBlurPass {
    public:
        struct Gpu;    //!< 描画 API の資源（Renderer の内部で定義）

        //! 資源を持たない状態で作ります（Initialize() で作成します）。
        MotionBlurPass();

        //! 資源を破棄します。
        ~MotionBlurPass();

        MotionBlurPass(const MotionBlurPass&)            = delete;
        MotionBlurPass& operator=(const MotionBlurPass&) = delete;

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

        //! 定数バッファ（b8）とピクセルシェーダーを作成します。
        //! @param  [in] graphicsContext デバイス・HDR バッファ・速度バッファの取得元
        //! @param  [in] resources       共通ステートとサンプラーの取得元
        //! @param  [in] frameConstants  b0 の取得元
        //! @param  [in] fullscreenPass  フルスクリーン三角形の描画
        //! @param  [in] ps              モーションブラー用ピクセルシェーダー（VS はフルスクリーン三角形用を共用）
        //! @return true: 作成成功, false: 作成失敗（パスは何もしなくなる）
        [[nodiscard]]
        bool Initialize(GraphicsContext& graphicsContext, RenderResources& resources, FrameConstants& frameConstants,
                        const FullscreenPass& fullscreenPass, const Tsukino::Asset::ShaderAsset* ps);

        //! モーションブラーのパラメータを設定します。
        //! @param  [in] params モーションブラーの定数バッファデータ
        void SetParameters(const CBufferMotionBlur& params) noexcept {
            m_data = params;
        }

        //! このフレームでモーションブラーを掛けるかを設定します。
        //! @param  [in] enabled true: 掛ける, false: 掛けない
        //! @note   無効なフレームでは、速度バッファ用の前フレームボーン行列（8KB/ドロー）の転送も省かれる
        void SetEnabled(bool enabled) noexcept {
            m_enabled = enabled;
        }

        //! このフレームでモーションブラーが有効かを取得します。
        //! @return true: 有効（描画コマンドが速度を書き出す）, false: 無効
        [[nodiscard]]
        bool IsEnabled() const noexcept {
            return m_enabled && m_allowed;
        }

        //! 描画品質としてモーションブラーを許すかを設定します。
        //! @param  [in] allowed true: 許す（SetEnabled のとおりに掛かる）, false: 許さない（SetEnabled(true) でも掛からない）
        void SetAllowed(bool allowed) noexcept {
            m_allowed = allowed;
        }

        //! モーションブラーを掛けてポストプロセス用の中間バッファへ書きます。
        //! @return true: ブラーを掛けて中間バッファへ書いた
        //!         false: 無効なので何もしていない（HDR バッファがそのまま最新）
        //! @note   Transparent パスの直後・トーンマップパスの直前に呼ぶこと
        bool Execute();

        //! フレームの終わりの処理をします（有効フラグを false へ戻します）。
        //! @note   毎フレーム MotionBlurSystem が再度 true にする前提にしておくと、MotionBlurSystem を
        //!         持たないシーンへ切り替えたときにフラグが立ちっぱなしで残らない（無駄なフルスクリーンパスの防止）
        void EndFrame() noexcept {
            m_enabled = false;
        }

    private:
        GraphicsContext*      m_graphicsContext = nullptr;    // デバイス・HDR バッファ・速度バッファの取得元（借りている）
        RenderResources*      m_resources       = nullptr;    // 共通ステートとサンプラーの取得元（借りている）
        FrameConstants*       m_frameConstants  = nullptr;    // b0 の取得元（借りている）
        const FullscreenPass* m_fullscreenPass  = nullptr;    // フルスクリーン三角形の描画（借りている）

        std::unique_ptr<Gpu> m_gpu;    // 描画 API の資源（中身は Renderer の内部で定義）
        CBufferMotionBlur                         m_data{};             // CPU側のモーションブラーパラメータ
        bool                                      m_enabled = false;    // このフレームで有効か
        bool                                      m_allowed = true;     // 描画品質として許しているか
    };
}    // namespace Tsukino::Renderer
