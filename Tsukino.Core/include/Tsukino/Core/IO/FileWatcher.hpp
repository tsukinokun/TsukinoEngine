//-------------------------------------------------------------
//! @file   FileWatcher.hpp
//! @brief  ファイルの更新の監視（更新時刻のポーリング）
//! @detail 登録したファイルの最終更新時刻を一定間隔で調べ、変わったファイルのコールバックを呼びます。
//!         調整用データの読み直し（ホットリロード）など、開発中の道具に使うことを想定しています。
//-------------------------------------------------------------
#pragma once
#include <Tsukino/Core/Path.hpp>

#include <chrono>
#include <filesystem>
#include <functional>
#include <mutex>
#include <vector>

// 名前空間 : Tsukino::IO
namespace Tsukino::IO {
    //-------------------------------------------------------------
    //! @class  FileWatcher
    //! ファイルの更新を監視するクラスです。
    //! @note   Watch はどのスレッドから呼んでもよいです。Poll は1つのスレッド（通常はメインループ）から呼びます。
    //!         更新を見つけても、次の Poll まで更新時刻が変わらなかった（書き込みが落ち着いた）ときに
    //!         初めてコールバックを呼ぶため、書きかけのファイルを読むことはありません。
    //-------------------------------------------------------------
    class FileWatcher final {
    public:
        //---------------------------------------------------------
        // 監視するファイルを登録する
        //! @param  path      [in] 監視するファイルのパス
        //! @param  onChanged [in] 更新されたときに Poll の中から呼ばれる関数
        //! @note   登録した時点の更新時刻を基準にする（登録より前の更新では呼ばれない）
        //---------------------------------------------------------
        void Watch(const Tsukino::Core::Path& path, std::function<void()> onChanged);

        //---------------------------------------------------------
        // 更新されたファイルを調べ、コールバックを呼ぶ
        //! @note   前回の確認から GetInterval() 未満なら何もしない。毎フレーム呼んでよい
        //---------------------------------------------------------
        void Poll();

        //---------------------------------------------------------
        // 確認の間隔を設定する
        //! @param  interval [in] 更新時刻を調べる間隔
        //---------------------------------------------------------
        void SetInterval(std::chrono::milliseconds interval) noexcept { m_interval = interval; }

        //---------------------------------------------------------
        // 確認の間隔を取得する
        //! @return 更新時刻を調べる間隔
        //---------------------------------------------------------
        [[nodiscard]]
        std::chrono::milliseconds GetInterval() const noexcept { return m_interval; }

    private:
        //! 監視中のファイル1本ぶんの情報
        struct Entry {
            Tsukino::Core::Path             path;                //!< 監視するファイル
            std::function<void()>           onChanged;           //!< 更新されたときに呼ぶ関数
            std::filesystem::file_time_type knownTime;           //!< 最後に通知した（または登録した）ときの更新時刻
            std::filesystem::file_time_type pendingTime;         //!< 見つけたが、まだ通知していない更新時刻
            bool                            pending = false;    //!< 書き込みが落ち着くのを待っているか
        };

        std::mutex                            m_mutex;                                       //!< m_entries を守る
        std::vector<Entry>                    m_entries;                                     //!< 監視中のファイル
        std::chrono::milliseconds             m_interval = std::chrono::milliseconds(500);  //!< 確認の間隔
        std::chrono::steady_clock::time_point m_lastPoll{};                                  //!< 前回確認した時刻
    };
}    // namespace Tsukino::IO
