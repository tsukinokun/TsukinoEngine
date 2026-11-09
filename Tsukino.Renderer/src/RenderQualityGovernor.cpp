//----------------------------------------------------------------------------
//! @file   RenderQualityGovernor.cpp
//! @brief  フレーム時間の実測で描画品質の引き下げを判断するクラスの実装
//----------------------------------------------------------------------------
#include <Tsukino/Renderer/RenderQualityGovernor.hpp>

#include <algorithm>
#include <cmath>

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {

    //------------------------------------------------------------------------
    //! 測り直します（平均とクールダウンを捨てます）。
    //------------------------------------------------------------------------
    void RenderQualityGovernor::Reset() noexcept {
        m_average      = 0.0f;
        m_overBudget   = 0.0f;
        m_cooldownLeft = 0.0f;
    }

    //------------------------------------------------------------------------
    //! 1フレーム分の間隔を与えて、品質を1段下げるべきかを返します。
    //------------------------------------------------------------------------
    bool RenderQualityGovernor::Tick(float deltaSeconds) noexcept {
        // 読み込みやウィンドウのドラッグの待ちは描画の重さではないので数えない
        if(deltaSeconds <= 0.0f || deltaSeconds > m_settings.ignoreAboveSeconds)
            return false;

        if(m_cooldownLeft > 0.0f) {
            // 下げた直後は作り直しの引っかかりが残るので、平均も取り直す
            m_cooldownLeft = std::max(0.0f, m_cooldownLeft - deltaSeconds);
            m_average      = 0.0f;
            m_overBudget   = 0.0f;
            return false;
        }

        // フレーム間隔が違っても同じ速さで追うよう、経過時間から混ぜる割合を求める
        const float blend = 1.0f - std::exp(-deltaSeconds / std::max(m_settings.smoothingSeconds, 1.0e-3f));
        m_average         = (m_average > 0.0f) ? m_average + (deltaSeconds - m_average) * blend : deltaSeconds;

        m_overBudget = (m_average > m_settings.targetFrameSeconds) ? m_overBudget + deltaSeconds : 0.0f;
        if(m_overBudget < m_settings.sustainSeconds)
            return false;

        m_overBudget   = 0.0f;
        m_average      = 0.0f;
        m_cooldownLeft = m_settings.cooldownSeconds;
        return true;
    }

}    // namespace Tsukino::Renderer
