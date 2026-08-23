//-------------------------------------------------------------
//! @file   SystemManager.cpp
//! @brief  システムマネージャーの実装
//! @author 山﨑愛
//-------------------------------------------------------------
#include <Tsukino/Engine/ECS/SystemManager.hpp>
#include <Tsukino/Core/ECS/System/ISystem.hpp>
#include <Tsukino/Core/ECS/Registry/Registry.hpp>
#include <Tsukino/Core/DebugTools/FrameProfiler.hpp>

#include <algorithm>
#include <cstring>
#include <typeinfo>
// 名前空間 : Tsukino::ECS
namespace Tsukino::ECS {
    namespace {
        //-------------------------------------------------------------
        //! @brief  型名から表示用の短い名前（末尾の要素）を取り出す関数
        //! @param  fullName [in] typeid::name() が返す文字列
        //! @return 最後の "::" の直後を指すポインタ（無ければ先頭）
        //! @note   MSVCのtypeid::name()は "class Tsukino::BuiltIn::ECS::ModelSystem" を返す。
        //!         HUDの幅に収めるため名前空間を落として "ModelSystem" だけにする。
        //!         返すのは引数の文字列の途中を指すポインタなので、寿命は引数に従う
        //!         （typeid::name()の戻り値は静的記憶域なのでプロセス終了まで有効）
        //-------------------------------------------------------------
        [[nodiscard]]
        const char* ExtractShortTypeName(const char* fullName) {
            if(!fullName)
                return "";

            const char* shortName = fullName;
            for(const char* cursor = fullName; *cursor != '\0'; ++cursor) {
                if(cursor[0] == ':' && cursor[1] == ':')
                    shortName = cursor + 2;
            }

            return shortName;
        }
    }    // namespace

    //-------------------------------------------------------------
    //! @brief  システムの追加
    //-------------------------------------------------------------
    void SystemManager::AddSystem(std::shared_ptr<ISystem> system, int priority) {
        SystemEntry entry{};
        // typeid は毎フレームではなく追加時に一度だけ引く（HUD表示用の名前解決）
        entry.name     = (system != nullptr) ? ExtractShortTypeName(typeid(*system).name()) : "";
        entry.system   = std::move(system);
        entry.priority = priority;

        m_systems.push_back(std::move(entry));

        // 追加時に優先度で昇順ソート（priority が小さい要素が前になる）。
        // stable_sort なのは、同じ優先度で登録された複数のシステムが
        // 「登録した順に実行される」ことに依存しているため
        // （例：EnemyAnimationSystem が書いた next を同フレームで AnimationSystem が消費する、
        //   といった同一優先度内の前後関係。std::sort だとこの順序が保証されない）
        std::stable_sort(m_systems.begin(), m_systems.end(), [](const SystemEntry& a, const SystemEntry& b) { return a.priority < b.priority; });

        //-------------------------------------------------------------
        // 表示用の計測結果もソート後の並びで作り直す
        //-------------------------------------------------------------
        m_profiles.clear();
        m_profiles.reserve(m_systems.size());
        for(const SystemEntry& sortedEntry : m_systems) {
            m_profiles.push_back(SystemProfile{sortedEntry.name, 0.0});
        }
    }

    //-------------------------------------------------------------
    //! @brief  システムの更新
    //-------------------------------------------------------------
    void SystemManager::Update(Tsukino::ECS::Registry& registry, float deltaTime) {
#ifdef TSUKINO_ENABLE_FRAME_PROFILER
        for(auto& entry : m_systems) {
            if(!entry.system)
                continue;

            const double beginMs = Tsukino::Core::DebugTools::FrameProfiler::NowMilliseconds();
            entry.system->Update(registry, deltaTime);
            entry.accumulatedMs += Tsukino::Core::DebugTools::FrameProfiler::NowMilliseconds() - beginMs;
        }

        //-------------------------------------------------------------
        // 集計期間ぶんの平均を確定させる
        // 毎フレームの生値は数値が暴れて読めないため、一定時間ごとに均す
        //-------------------------------------------------------------
        m_windowElapsed += deltaTime;
        ++m_windowFrames;

        if(m_windowElapsed >= kAverageWindowSeconds) {
            const double frames = static_cast<double>(m_windowFrames);

            //---------------------------------------------------------
            // HUDを描くのはゲーム側のSystemで、そちらはRegistryしか受け取らず
            // SystemManagerへ辿れない。計測結果はFrameProfilerへ押し込んで受け渡す
            //---------------------------------------------------------
            Tsukino::Core::DebugTools::FrameProfiler& profiler = Tsukino::Core::DebugTools::FrameProfiler::Get();
            profiler.BeginSystemReport();

            const size_t count = std::min(m_systems.size(), m_profiles.size());
            for(size_t i = 0; i < count; ++i) {
                m_profiles[i].averageMs    = m_systems[i].accumulatedMs / frames;
                m_systems[i].accumulatedMs = 0.0;

                profiler.ReportSystemTime(m_profiles[i].name, m_profiles[i].averageMs);
            }

            m_windowElapsed = 0.0f;
            m_windowFrames  = 0;
        }
#else
        for(auto& entry : m_systems) {
            if(entry.system) {
                entry.system->Update(registry, deltaTime);
            }
        }
#endif
    }

    //-------------------------------------------------------------
    //! @brief  システムのクリア
    //-------------------------------------------------------------
    void SystemManager::Clear() {
        m_systems.clear();
        m_profiles.clear();
        m_windowElapsed = 0.0f;
        m_windowFrames  = 0;
    }

}    // namespace Tsukino::ECS
