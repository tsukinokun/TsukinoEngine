//----------------------------------------------------------------------------
//! @file   AssetLoadQueue.hpp
//! @brief  アセットを裏スレッドで順に読み込むキュー
//! @detail ロード画面などで、本体スレッドを止めずに多数のアセットを読み込むために使います。
//!         AssetManager::Load は任意のスレッドから呼べるので、このキューは専用のスレッドを1本立てて
//!         積まれたパスを順に Load するだけです。読み込んだアセットは AssetManager に入るため、
//!         以降の Load は表を引くだけで即座に返ります。
//!
//!         GPU リソースの生成（テクスチャのアップロードなど）は描画側が本体スレッドで行うため、
//!         このキューは扱いません。ロード画面で先に転送しておきたい場合は、TakeLoaded() で
//!         受け取ったハンドルを本体スレッドから描画側へ渡してください。
//!
//!         使い方：
//!             AssetLoadQueue queue(assetManager);
//!             queue.Enqueue("Game/Assets/Textures/a.png");
//!             queue.Start();
//!             // 毎フレーム queue.Progress() を表示し、queue.IsFinished() になったら次へ
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/Core/Path.hpp>
#include <Tsukino/Engine/Asset/AssetHandle.hpp>

#include <atomic>
#include <mutex>
#include <thread>
#include <vector>

// 名前空間 : Tsukino::Asset
namespace Tsukino::Asset {
    class AssetManager;

    //! アセットを裏スレッドで順に読み込むキューです。
    class AssetLoadQueue {
    public:

        //! 読み込み先のアセット管理を受け取ります。
        //! @param  [in] assetManager 読み込み先のアセット管理（キューより長く生きていること）
        explicit AssetLoadQueue(AssetManager& assetManager);

        //! 読み込みを打ち切り、スレッドの終了を待ちます。
        ~AssetLoadQueue();

        AssetLoadQueue(const AssetLoadQueue&)            = delete;
        AssetLoadQueue& operator=(const AssetLoadQueue&) = delete;

        //! 読み込むアセットを積みます。Start() より前に呼びます。
        //! @param  [in] path 読み込むアセットのパス
        void Enqueue(const Tsukino::Core::Path& path);

        //! 裏スレッドで読み込みを始めます。2回目以降の呼び出しは無視します。
        void Start();

        //! 読み込みを打ち切ります。読み込み中の1件が終わった時点で止まります。
        void Cancel();

        //! 読み込みの進み具合を返します。
        //! @return 0〜1（積んだものが無ければ 1）
        [[nodiscard]] float Progress() const;

        //! すべて読み終えたか（または打ち切られたか）を返します。
        //! @return 終わっていれば true
        [[nodiscard]] bool IsFinished() const { return m_finished.load(); }

        //! 前回の呼び出し以降に読み終えたアセットのハンドルを取り出します（失敗したものは含みません）。
        //! @return 読み終えたハンドル
        [[nodiscard]] std::vector<AssetHandle> TakeLoaded();

    private:

        //! 裏スレッドの本体です。積まれたパスを順に読み込みます。
        void Run();

        AssetManager&                   m_assetManager;           // 読み込み先
        std::vector<Tsukino::Core::Path> m_paths;                 // 読み込むパス（Start 後は読み取りのみ）
        std::thread                     m_thread;                 // 読み込みのスレッド
        std::atomic<size_t>             m_completed = 0;          // 読み終えた件数（失敗を含む）
        std::atomic<bool>               m_cancel    = false;      // 打ち切りの指示
        std::atomic<bool>               m_finished  = false;      // 終わったか
        std::mutex                      m_loadedMutex;            // m_loaded を守る
        std::vector<AssetHandle>        m_loaded;                 // 本体スレッドへ渡すまでのハンドル
    };

}    // namespace Tsukino::Asset
