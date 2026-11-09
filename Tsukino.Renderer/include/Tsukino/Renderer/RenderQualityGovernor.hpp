//----------------------------------------------------------------------------
//! @file   RenderQualityGovernor.hpp
//! @brief  フレーム時間の実測で描画品質の引き下げを判断するクラスの宣言
//! @detail フレーム間隔の移動平均が目標を割り続けたら「1段下げる」と答えます。
//!         実時間のフレーム間隔だけを見るので、垂直同期の有無や GPU の種類に依りません。
//----------------------------------------------------------------------------
#pragma once

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {

    //------------------------------------------------------------------------
    //! 引き下げの判断に使う値です。
    //------------------------------------------------------------------------
    struct RenderQualityGovernorSettings {
        float targetFrameSeconds = 1.0f / 30.0f;    //!< 許せるフレーム間隔の平均（秒）
        float sustainSeconds     = 3.0f;            //!< 目標を割り続けたら下げるまでの時間（秒）
        float cooldownSeconds    = 6.0f;            //!< 下げたあと次の判断を始めるまでの時間（秒）
        float smoothingSeconds   = 0.5f;            //!< 移動平均の時定数（秒）
        float ignoreAboveSeconds = 1.0f;            //!< これより長いフレームは数えない（読み込みやウィンドウ操作の待ち）
    };

    //------------------------------------------------------------------------
    //! フレーム時間の実測で描画品質の引き下げを判断するクラスです。
    //! @note  上げる判断はしない（一時的に軽くなった場面で上げ下げを繰り返さないため）
    //------------------------------------------------------------------------
    class RenderQualityGovernor {
    public:

        //! 判断に使う値を設定します。
        //! @param  [in] settings 判断に使う値
        void SetSettings(const RenderQualityGovernorSettings& settings) noexcept {
            m_settings = settings;
        }

        //! 測り直します（平均とクールダウンを捨てます）。
        void Reset() noexcept;

        //! 1フレーム分の間隔を与えて、品質を1段下げるべきかを返します。
        //! @param  [in] deltaSeconds 前のフレームからの実時間（秒）
        //! @return true: 1段下げるべき（返したあとはクールダウンに入る）
        [[nodiscard]]
        bool Tick(float deltaSeconds) noexcept;

    private:
        RenderQualityGovernorSettings m_settings{};             // 判断に使う値
        float                         m_average       = 0.0f;   // フレーム間隔の移動平均（秒。0 は未測定）
        float                         m_overBudget    = 0.0f;   // 目標を割り続けている時間（秒）
        float                         m_cooldownLeft  = 0.0f;   // 判断を止めている残り時間（秒）
    };

}    // namespace Tsukino::Renderer
