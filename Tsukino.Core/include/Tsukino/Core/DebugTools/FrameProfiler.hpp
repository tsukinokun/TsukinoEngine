//-------------------------------------------------------------
//! @file   FrameProfiler.hpp
//! @brief  フレーム単位のCPU時間計測クラスの宣言
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once

#include <Tsukino/Core/DebugTools/DebugFeatures.hpp>

#include <vector>

// 名前空間 : Tsukino::Core::DebugTools
namespace Tsukino::Core::DebugTools {
    //-------------------------------------------------------------
    //! @enum  ProfileSlot
    //! @brief フレーム内の計測区間
    //! @note  システム単位の内訳はSystemManagerが自前で持つ。ここはメインループを
    //!        Update / Render に割った粒度だけを扱う。両者を足してもFrameに満たない場合、
    //!        差分はPresentのVSync待ちかドライバ側の時間である
    //-------------------------------------------------------------
    enum class ProfileSlot : int {
        Frame = 0,    //!< メインループ1周（WinMainが計測するdeltaTime）
        Update,       //!< EngineAPI::Update（＝全システムのUpdate）
        Render,       //!< EngineAPI::Render（＝描画コマンドのサブミット）

        Count
    };

    //-------------------------------------------------------------
    //! @class FrameProfiler
    //! @brief フレーム時間を区間ごとに集計し、移動平均で保持するシングルトン
    //! @note  TSUKINO_ENABLE_FRAME_PROFILER が未定義のときは全ての関数が空実装になり、
    //!        呼び出し側に #ifdef を書かなくても実行時コストがゼロになる
    //-------------------------------------------------------------
    class FrameProfiler {
    public:
        //-------------------------------------------------------------
        //! @brief  唯一のインスタンスを取得する関数
        //! @return フレームプロファイラの参照
        //-------------------------------------------------------------
        [[nodiscard]]
        static FrameProfiler& Get();

        //-------------------------------------------------------------
        //! @brief  現在時刻をミリ秒で取得する関数
        //! @return 高分解能カウンタによる時刻（ミリ秒）
        //! @note   区間計測の起点・終点の取得に使う。std::chronoではなく
        //!         QueryPerformanceCounterを使うのは、計測そのものが
        //!         ホットパスに乗るため呼び出しコストを最小にしたいから
        //-------------------------------------------------------------
        [[nodiscard]]
        static double NowMilliseconds();

        //-------------------------------------------------------------
        //! @brief 計測値を積む関数
        //! @param slot         [in] 計測区間
        //! @param milliseconds [in] 経過時間（ミリ秒）
        //-------------------------------------------------------------
        void AddSample(ProfileSlot slot, double milliseconds);

        //-------------------------------------------------------------
        //! @brief フレーム末尾で移動平均を更新する関数
        //! @param deltaTime [in] 前フレームからの経過時間（秒）
        //! @note  毎フレーム値を出すと数値が暴れて読めないため、
        //!        kAverageWindowSeconds ごとに平均を採り直して表示値とする
        //-------------------------------------------------------------
        void Tick(float deltaTime);

        //-------------------------------------------------------------
        //! @brief  区間の移動平均を取得する関数
        //! @param  slot [in] 計測区間
        //! @return 平均経過時間（ミリ秒）
        //-------------------------------------------------------------
        [[nodiscard]]
        double GetAverageMs(ProfileSlot slot) const;

        //-------------------------------------------------------------
        //! @struct SystemTiming
        //! @brief  システム1つぶんの計測結果
        //-------------------------------------------------------------
        struct SystemTiming {
            const char* name      = nullptr;    //!< システム名（静的文字列を指す）
            double      averageMs = 0.0;        //!< Updateにかかった時間の移動平均（ミリ秒）
        };

        //-------------------------------------------------------------
        //! @brief システム別の計測結果の登録を開始する関数（既存の内容を捨てる）
        //! @note  SystemManagerが平均を採り直したタイミングで呼ぶ。
        //!        HUD（ゲーム側のSystem）はRegistryしか受け取らずSystemManagerへ
        //!        辿れないため、計測結果はここへ集約して受け渡す。
        //!        Tsukino.Core は Tsukino.Engine に依存できない（依存の向きが逆）ので、
        //!        SystemManager側から押し込む形にしている
        //-------------------------------------------------------------
        void BeginSystemReport();

        //-------------------------------------------------------------
        //! @brief システム別の計測結果を1件登録する関数
        //! @param name      [in] システム名（静的文字列であること）
        //! @param averageMs [in] 平均経過時間（ミリ秒）
        //-------------------------------------------------------------
        void ReportSystemTime(const char* name, double averageMs);

        //-------------------------------------------------------------
        //! @brief  システム別の計測結果を取得する関数
        //! @return 優先度順に並んだ計測結果
        //-------------------------------------------------------------
        [[nodiscard]]
        const std::vector<SystemTiming>& GetSystemTimings() const {
            return m_systemTimings;
        }

    private:
        //! @brief 平均を採り直す間隔（秒）
        static constexpr float kAverageWindowSeconds = 0.5f;

        static constexpr int kSlotCount = static_cast<int>(ProfileSlot::Count);

        double m_accumulated[kSlotCount]{};    //!< 現在の集計期間で積んだ合計（ミリ秒）
        double m_average[kSlotCount]{};        //!< 表示用の移動平均（ミリ秒）

        float m_windowElapsed = 0.0f;    //!< 現在の集計期間の経過時間（秒）
        int   m_windowFrames  = 0;       //!< 現在の集計期間のフレーム数

        std::vector<SystemTiming> m_systemTimings;    //!< SystemManagerが押し込んだシステム別の計測結果
    };

    //-------------------------------------------------------------
    //! @class ScopedProfileTimer
    //! @brief スコープの生存期間を計測してFrameProfilerへ積むRAIIヘルパー
    //-------------------------------------------------------------
    class ScopedProfileTimer {
    public:
        //-------------------------------------------------------------
        //! @brief コンストラクタ（計測開始）
        //! @param slot [in] 計測区間
        //-------------------------------------------------------------
        explicit ScopedProfileTimer(ProfileSlot slot)
            : m_slot(slot)
            , m_startMs(FrameProfiler::NowMilliseconds()) {}

        //-------------------------------------------------------------
        //! @brief デストラクタ（計測終了・値を積む）
        //-------------------------------------------------------------
        ~ScopedProfileTimer() { FrameProfiler::Get().AddSample(m_slot, FrameProfiler::NowMilliseconds() - m_startMs); }

        ScopedProfileTimer(const ScopedProfileTimer&)            = delete;
        ScopedProfileTimer& operator=(const ScopedProfileTimer&) = delete;

    private:
        ProfileSlot m_slot;       //!< 計測区間
        double      m_startMs;    //!< 計測開始時刻（ミリ秒）
    };
}    // namespace Tsukino::Core::DebugTools
