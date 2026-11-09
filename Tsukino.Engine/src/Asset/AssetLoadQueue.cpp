//----------------------------------------------------------------------------
//! @file   AssetLoadQueue.cpp
//! @brief  アセットを裏スレッドで順に読み込むキューの実装
//----------------------------------------------------------------------------
#include <Tsukino/Engine/Asset/AssetLoadQueue.hpp>

#include <Tsukino/Engine/Asset/AssetManager.hpp>

#include <Tsukino/Core/Log.hpp>

#include <objbase.h>

#include <algorithm>

// 名前空間 : Tsukino::Asset
namespace Tsukino::Asset {

    //----------------------------------------------------------------------------
    //! 読み込み先のアセット管理を受け取ります。
    //----------------------------------------------------------------------------
    AssetLoadQueue::AssetLoadQueue(AssetManager& assetManager)
        : m_assetManager(assetManager) {
    }

    //----------------------------------------------------------------------------
    //! 読み込みを打ち切り、スレッドの終了を待ちます。
    //----------------------------------------------------------------------------
    AssetLoadQueue::~AssetLoadQueue() {
        Cancel();
        for(std::thread& thread : m_threads) {
            thread.join();
        }
    }

    //----------------------------------------------------------------------------
    //! 読み込むアセットを積みます。
    //----------------------------------------------------------------------------
    void AssetLoadQueue::Enqueue(const Tsukino::Core::Path& path) {
        // 走り始めた後に積むと、裏スレッドが読んでいる配列を書き換えることになる
        if(!m_threads.empty()) {
            Tsukino::Core::Log::Warn("AssetLoadQueue: Enqueue after Start is ignored: " + path.string());
            return;
        }
        m_paths.push_back(path);
    }

    //----------------------------------------------------------------------------
    //! 裏スレッドで読み込みを始めます。
    //----------------------------------------------------------------------------
    void AssetLoadQueue::Start() {
        if(!m_threads.empty() || m_finished.load()) {
            return;
        }
        if(m_paths.empty()) {
            m_finished = true;
            return;
        }

        //----------------------------------------------------------------------------
        // 初回のテクスチャ変換（ブロック圧縮など）は CPU を使い切るので、コア数に合わせて手分けする。
        // 本体スレッドの描画（ロード画面）のために1コアは空けておく
        //----------------------------------------------------------------------------
        constexpr unsigned int kMaxWorkers = 16;
        const unsigned int     cores       = std::thread::hardware_concurrency();
        const unsigned int     workers     = std::clamp(cores > 1 ? cores - 1 : 1u, 1u, kMaxWorkers);

        m_running = workers;
        for(unsigned int i = 0; i < workers; ++i) {
            m_threads.emplace_back([this] { Run(); });
        }
    }

    //----------------------------------------------------------------------------
    //! 読み込みを打ち切ります。
    //----------------------------------------------------------------------------
    void AssetLoadQueue::Cancel() {
        m_cancel = true;
    }

    //----------------------------------------------------------------------------
    //! 読み込みの進み具合を返します。
    //----------------------------------------------------------------------------
    float AssetLoadQueue::Progress() const {
        if(m_paths.empty()) {
            return 1.0f;
        }
        return static_cast<float>(m_completed.load()) / static_cast<float>(m_paths.size());
    }

    //----------------------------------------------------------------------------
    //! 前回の呼び出し以降に読み終えたアセットのハンドルを取り出します。
    //----------------------------------------------------------------------------
    std::vector<AssetHandle> AssetLoadQueue::TakeLoaded() {
        std::lock_guard<std::mutex> lock(m_loadedMutex);
        std::vector<AssetHandle>    taken;
        taken.swap(m_loaded);
        return taken;
    }

    //----------------------------------------------------------------------------
    //! 裏スレッドの本体です。
    //----------------------------------------------------------------------------
    void AssetLoadQueue::Run() {
        // テクスチャの変換は WIC（COM）を使うため、このスレッドでも COM を初期化しておく
        const HRESULT comResult = CoInitializeEx(nullptr, COINIT_MULTITHREADED);

        for(size_t index = m_nextIndex++; index < m_paths.size(); index = m_nextIndex++) {
            if(m_cancel.load()) {
                break;
            }
            const Tsukino::Core::Path& path   = m_paths[index];
            const AssetHandle          handle = m_assetManager.Load(path);
            if(handle.IsValid()) {
                std::lock_guard<std::mutex> lock(m_loadedMutex);
                m_loaded.push_back(handle);
            } else {
                // 1件の失敗で残りを止めると、ロード画面から先へ進めなくなる。記録して続ける
                Tsukino::Core::Log::Warn("AssetLoadQueue: Failed to load " + path.string());
            }
            ++m_completed;
        }

        if(SUCCEEDED(comResult)) {
            CoUninitialize();
        }

        // 最後に抜けるスレッドが終わりの印を立てる
        if(--m_running == 0) {
            m_finished = true;
        }
    }

}    // namespace Tsukino::Asset
