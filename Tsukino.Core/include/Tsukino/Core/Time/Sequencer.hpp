//----------------------------------------------------------------------------
//! @file   Sequencer.hpp
//! @brief  時間差で処理を呼ぶ小さな段取り表
//! @detail 「0.2 秒後に音を鳴らし、0.9 秒後に光らせる」のような演出の段取りを、
//!         コールバックの予約として書けるようにします。時間は Update に渡した分だけ進むので、
//!         ゲームの時間（ポーズで止まる）で進めるか、実時間で進めるかは呼ぶ側が決めます。
//----------------------------------------------------------------------------
#pragma once
#include <algorithm>
#include <functional>
#include <utility>
#include <vector>

// 名前空間 : Tsukino::Core
namespace Tsukino::Core {

    //! 時間差で処理を呼ぶ段取り表です。
    //! @note   コールバックが捕まえたエンティティやポインタは、呼ばれる時点でまだ有効とは限らない。
    //!         持ち主が先に消えるときは Clear で予約を取り消すこと
    class Sequencer {
    public:

        //! 処理を予約します。
        //! @param  [in] delay    今から何秒後に呼ぶか
        //! @param  [in] callback 呼ぶ処理
        void Schedule(float delay, std::function<void()> callback) {
            m_entries.push_back({m_now + delay, m_order++, std::move(callback)});
        }

        //! 時間を進め、時刻が来た処理を時刻順（同じ時刻なら予約順）に呼びます。
        //! 呼ばれた処理の中から新たに予約してもかまいません。
        //! @param  [in] deltaTime 経過時間（秒）
        void Update(float deltaTime) {
            m_now += deltaTime;
            for(;;) {
                // 呼んだ処理の中で予約が増えることがあるため、1件ずつ取り出して呼ぶ
                auto due = std::min_element(m_entries.begin(), m_entries.end(), [](const Entry& a, const Entry& b) {
                    return a.time != b.time ? a.time < b.time : a.order < b.order;
                });
                if(due == m_entries.end() || due->time > m_now) {
                    return;
                }
                std::function<void()> callback = std::move(due->callback);
                m_entries.erase(due);
                callback();
            }
        }

        //! 予約をすべて取り消します。
        void Clear() { m_entries.clear(); }

        //! 予約が残っているかを返します。
        //! @return 残っていれば true。
        [[nodiscard]]
        bool IsBusy() const noexcept { return !m_entries.empty(); }

    private:
        //! 予約1件です。
        struct Entry {
            float                 time;        // 呼ぶ時刻
            unsigned              order;       // 予約順（同じ時刻のときの並び）
            std::function<void()> callback;    // 呼ぶ処理
        };

        std::vector<Entry> m_entries;         // 予約
        float              m_now   = 0.0f;    // 現在時刻
        unsigned           m_order = 0;       // 次の予約順
    };

}    // namespace Tsukino::Core
