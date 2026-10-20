//-------------------------------------------------------------
//! @file   HotReloadable.hpp
//! @brief  保存し直すと読み直す値のキャッシュ
//-------------------------------------------------------------
#pragma once
#include <Tsukino/Core/DebugTools/DebugFeatures.hpp>
#include <Tsukino/Core/IO/HotReload.hpp>
#include <Tsukino/Core/Path.hpp>

#include <functional>
#include <utility>

// 名前空間 : Tsukino::IO
namespace Tsukino::IO {
    //-------------------------------------------------------------
    //! @class  HotReloadable
    //! ファイルから読んだ値を覚え、保存し直されたら読み直すキャッシュです（関数内staticに置いて使う）。
    //! @tparam T 読み込み先の型（既定構築できること）
    //! @note   作った時点で1度読み、HotReload へ登録する。読み直しは一時のTへ読み、
    //!         読めたときだけ値を差し替える（壊れたファイルなら前の値のまま）。
    //!         差し替えは同じオブジェクトへの代入なので、Get() で受け取った参照はそのまま使える。
    //!         ただし中のvectorの要素へのポインタ・参照をフレームをまたいで持たないこと
    //-------------------------------------------------------------
    template <class T>
    class HotReloadable final {
    public:
        //! 既定値のTを受け取り、読んで、読めたかを返す関数
        using Loader = std::function<bool(T&)>;

        //-------------------------------------------------------------
        //! 読み込み、監視を登録します。
        //! @param  path   [in] 読むファイルのパス（監視にも使う）
        //! @param  loader [in] 読み込む関数
        //-------------------------------------------------------------
        HotReloadable(const Tsukino::Core::Path& path, Loader loader)
            : m_loader(std::move(loader)) {
            (void)m_loader(m_value);
#ifdef TSUKINO_ENABLE_HOT_RELOAD
            HotReload::Watch(path, [this] { return Reload(); });
#else
            (void)path;
#endif
        }

        HotReloadable(const HotReloadable&)            = delete;
        HotReloadable& operator=(const HotReloadable&) = delete;

        //-------------------------------------------------------------
        //! 今の値を取得します。
        //! @return 今の値
        //-------------------------------------------------------------
        [[nodiscard]]
        const T& Get() const noexcept { return m_value; }

    private:
        //-------------------------------------------------------------
        //! 読み直します。
        //! @return 読み直せたか（false なら値は前のまま）
        //-------------------------------------------------------------
        bool Reload() {
            T value{};
            if(!m_loader(value))
                return false;
            m_value = std::move(value);
            return true;
        }

        Loader m_loader;      //!< 読み込む関数
        T      m_value{};    //!< 今の値
    };
}    // namespace Tsukino::IO
