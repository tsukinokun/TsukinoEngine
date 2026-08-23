//-------------------------------------------------------------
//! @file   FrameProfiler.cpp
//! @brief  フレーム単位のCPU時間計測クラスの実装
//! @author 山﨑愛
//-------------------------------------------------------------
#include <Tsukino/Core/DebugTools/FrameProfiler.hpp>

#include <Windows.h>

// 名前空間 : Tsukino::Core::DebugTools
namespace Tsukino::Core::DebugTools {
    //-------------------------------------------------------------
    //! @brief 唯一のインスタンスを取得する
    //-------------------------------------------------------------
    FrameProfiler& FrameProfiler::Get() {
        static FrameProfiler instance;
        return instance;
    }

    //-------------------------------------------------------------
    //! @brief 現在時刻をミリ秒で取得する
    //-------------------------------------------------------------
    double FrameProfiler::NowMilliseconds() {
        //---------------------------------------------------------
        // 周波数はシステム起動中は不変なので一度だけ引く
        //---------------------------------------------------------
        static const double invFrequency = [] {
            LARGE_INTEGER frequency{};
            ::QueryPerformanceFrequency(&frequency);
            return 1000.0 / static_cast<double>(frequency.QuadPart);
        }();

        LARGE_INTEGER counter{};
        ::QueryPerformanceCounter(&counter);
        return static_cast<double>(counter.QuadPart) * invFrequency;
    }

    //-------------------------------------------------------------
    //! @brief 計測値を積む
    //-------------------------------------------------------------
    void FrameProfiler::AddSample(ProfileSlot slot, double milliseconds) {
#ifdef TSUKINO_ENABLE_FRAME_PROFILER
        const int index = static_cast<int>(slot);
        if(index < 0 || index >= kSlotCount)
            return;

        m_accumulated[index] += milliseconds;
#else
        (void)slot;
        (void)milliseconds;
#endif
    }

    //-------------------------------------------------------------
    //! @brief フレーム末尾で移動平均を更新する
    //-------------------------------------------------------------
    void FrameProfiler::Tick(float deltaTime) {
#ifdef TSUKINO_ENABLE_FRAME_PROFILER
        m_windowElapsed += deltaTime;
        ++m_windowFrames;

        if(m_windowElapsed < kAverageWindowSeconds)
            return;

        //---------------------------------------------------------
        // 集計期間ぶんの平均を確定させ、次の期間のために積算をリセットする
        //---------------------------------------------------------
        const double frames = static_cast<double>(m_windowFrames);
        for(int i = 0; i < kSlotCount; ++i) {
            m_average[i]     = m_accumulated[i] / frames;
            m_accumulated[i] = 0.0;
        }

        m_windowElapsed = 0.0f;
        m_windowFrames  = 0;
#else
        (void)deltaTime;
#endif
    }

    //-------------------------------------------------------------
    //! @brief システム別の計測結果の登録を開始する
    //-------------------------------------------------------------
    void FrameProfiler::BeginSystemReport() {
        // clear() は確保済みの容量を維持するため、ウォームアップ後は追加確保が起きない
        m_systemTimings.clear();
    }

    //-------------------------------------------------------------
    //! @brief システム別の計測結果を1件登録する
    //-------------------------------------------------------------
    void FrameProfiler::ReportSystemTime(const char* name, double averageMs) {
        m_systemTimings.push_back(SystemTiming{name, averageMs});
    }

    //-------------------------------------------------------------
    //! @brief 区間の移動平均を取得する
    //-------------------------------------------------------------
    double FrameProfiler::GetAverageMs(ProfileSlot slot) const {
        const int index = static_cast<int>(slot);
        if(index < 0 || index >= kSlotCount)
            return 0.0;

        return m_average[index];
    }
}    // namespace Tsukino::Core::DebugTools
