//--------------------------------------------------------------
//! @file	Memory.hpp
//! @brief  共通型定義
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
#include <memory>
// 名前空間 : Tsukino::Core
namespace Tsukino::Core {
    // スマートポインタのエイリアステンプレート
    template <typename T>
    using Ref = std::shared_ptr<T>;

    //--------------------------------------------------------------
    //! @brief  Ref<T> を作成する関数
    //! @tparam T 作成する Ref の型
    //! @param  args [in] Ref<T> を作成するための引数
    //! @return 作成された Ref<T>
    //--------------------------------------------------------------
    template <typename T, typename... Args>
    constexpr Ref<T> CreateRef(Args&&... args) {
        return std::make_shared<T>(std::forward<Args>(args)...);
    }

}    // namespace Tsukino::Core
