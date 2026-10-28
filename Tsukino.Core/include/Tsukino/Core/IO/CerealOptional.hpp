//----------------------------------------------------------------------------
//! @file   CerealOptional.hpp
//! @brief  省略できる項目を cereal で読む関数
//! @detail 書かれていない項目を既定値のまま残して読みます。後から項目を足した型の load で使います。
//----------------------------------------------------------------------------
#pragma once
#include <cereal/cereal.hpp>

// 名前空間 : Tsukino::IO
namespace Tsukino::IO {

    //! 名前の付いた項目を1つ読み、無ければ value を変えずに残します。
    //! @param  [in]     archive 読み込み中のアーカイブ
    //! @param  [in]     name    項目の名前
    //! @param  [in,out] value   読み込み先（無ければ元の値のまま）
    //! @return 項目があって読めたか。
    //! @note   cereal の JSONInputArchive は、名前が見つからないときに読み取り位置を進める前に例外を投げるため、
    //!         ここで捕まえても続きの項目は正しく読める。入れ子の型の load の中でもこの関数を使えば、
    //!         どの深さの項目も省略できる
    template <class Archive, class T>
    bool LoadOptional(Archive& archive, const char* name, T& value) {
        try {
            archive(cereal::make_nvp(name, value));
            return true;
        } catch(const cereal::Exception&) {
            return false;
        }
    }

}    // namespace Tsukino::IO
