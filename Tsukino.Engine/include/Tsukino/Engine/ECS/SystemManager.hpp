//-------------------------------------------------------------
//! @file   SystemManager.hpp
//! @brief  システムマネージャーの宣言
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once
#include <Tsukino/Core/DebugTools/DebugFeatures.hpp>

#include <memory>
#include <vector>
// 名前空間 : Tsukino::ECS
namespace Tsukino::ECS {
    // 前方宣言
    class ISystem;
    class Registry;
    //-------------------------------------------------------------
    //! @class  SystemManager
    //! @brief  システムの管理クラス
    //-------------------------------------------------------------
    class SystemManager {
    public:
        //-------------------------------------------------------------
        //! @brief  デフォルトコンストラクタ
        //-------------------------------------------------------------
        SystemManager() = default;

        //-------------------------------------------------------------
        //! @brief  デストラクタ
        //-------------------------------------------------------------
        ~SystemManager() = default;

        //-------------------------------------------------------------
        // システムの追加
        //! @param  system      [in] 追加するシステム
        //! @param  priority    [in] システムの優先度（小さいほど先に更新される）
        //-------------------------------------------------------------
        void AddSystem(std::shared_ptr<ISystem> system, int priority = 0);

        //-------------------------------------------------------------
        // システムの更新
        //! @param  registry    [in] ECS レジストリ
        //! @param  deltaTime   [in] 前フレームからの経過
        //-------------------------------------------------------------
        void Update(Tsukino::ECS::Registry& registry, float deltaTime);

        //-------------------------------------------------------------
        // システムのクリア
        //-------------------------------------------------------------
        void Clear();

        //-------------------------------------------------------------
        //! @struct SystemProfile
        //! @brief  システム1つぶんの計測結果
        //-------------------------------------------------------------
        struct SystemProfile {
            const char* name      = nullptr;    //!< システム名（型名の末尾。静的文字列を指す）
            double      averageMs = 0.0;        //!< Updateにかかった時間の移動平均（ミリ秒）
        };

        //-------------------------------------------------------------
        //! @brief  システム別の計測結果を取得する関数
        //! @return 登録順（＝優先度順）に並んだ計測結果
        //! @note   TSUKINO_ENABLE_FRAME_PROFILER が無効なときは常に空を返す
        //-------------------------------------------------------------
        [[nodiscard]]
        const std::vector<SystemProfile>& GetProfiles() const {
            return m_profiles;
        }

    private:
        //-------------------------------------------------------------
        //! @brief   システム構造体
        //! @details システムとその優先度を保持する構造体
        //-------------------------------------------------------------
        struct SystemEntry {
            std::shared_ptr<ISystem> system;      // システムの共有ポインタ
            int                      priority;    // システムの優先度

            // 計測用。name は typeid(*system).name() が返す静的文字列の途中を指すため、
            // 寿命はプロセス終了まで保証される（毎フレーム typeid を呼ばないよう追加時に一度だけ解決する）
            const char* name          = nullptr;
            double      accumulatedMs = 0.0;    // 現在の集計期間で積んだ合計（ミリ秒）
        };

        //! @brief 平均を採り直す間隔（秒）。FrameProfilerと揃えて表示のちらつきを防ぐ
        static constexpr float kAverageWindowSeconds = 0.5f;

        std::vector<SystemEntry> m_systems;    // システム構造体のリスト

        std::vector<SystemProfile> m_profiles;    // 表示用の計測結果（m_systemsと同じ並び）

        float m_windowElapsed = 0.0f;    // 現在の集計期間の経過時間（秒）
        int   m_windowFrames  = 0;       // 現在の集計期間のフレーム数
    };
}    // namespace Tsukino::ECS
