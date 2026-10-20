//-------------------------------------------------------------
//! @file   FileWatcher.cpp
//! @brief  FileWatcher の実装
//-------------------------------------------------------------
#include <Tsukino/Core/IO/FileWatcher.hpp>

#include <Tsukino/Core/IO/FileSystem.hpp>

#include <utility>

// 名前空間 : Tsukino::IO
namespace Tsukino::IO {
    //-------------------------------------------------------------
    //! 監視するファイルを登録します。
    //-------------------------------------------------------------
    void FileWatcher::Watch(const Tsukino::Core::Path& path, std::function<void()> onChanged) {
        Entry entry;
        entry.path      = path;
        entry.onChanged = std::move(onChanged);
        entry.knownTime = FileSystem::GetLastWriteTime(path);

        std::lock_guard<std::mutex> lock(m_mutex);
        m_entries.push_back(std::move(entry));
    }

    //-------------------------------------------------------------
    //! 更新されたファイルを調べ、コールバックを呼びます。
    //-------------------------------------------------------------
    void FileWatcher::Poll() {
        const auto now = std::chrono::steady_clock::now();
        if(now - m_lastPoll < m_interval)
            return;
        m_lastPoll = now;

        // 呼ぶ関数を集めてから、ロックの外で呼ぶ（コールバックの中から Watch が呼ばれてもよいように）
        std::vector<std::function<void()>> callbacks;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            for(Entry& entry : m_entries) {
                const std::filesystem::file_time_type time = FileSystem::GetLastWriteTime(entry.path);

                // 消えている（保存の途中で一瞬消えるエディタもある）ときは、現れるまで待つ
                if(time == std::filesystem::file_time_type::min())
                    continue;

                if(time == entry.knownTime) {
                    entry.pending = false;
                    continue;
                }

                // 前回見つけた更新時刻のまま変わっていなければ、書き込みが落ち着いたとみなして通知
                if(entry.pending && time == entry.pendingTime) {
                    entry.knownTime = time;
                    entry.pending   = false;
                    callbacks.push_back(entry.onChanged);
                    continue;
                }

                // 新しい更新を見つけた。次の確認まで待つ
                entry.pending     = true;
                entry.pendingTime = time;
            }
        }

        for(const std::function<void()>& callback : callbacks) {
            if(callback)
                callback();
        }
    }
}    // namespace Tsukino::IO
