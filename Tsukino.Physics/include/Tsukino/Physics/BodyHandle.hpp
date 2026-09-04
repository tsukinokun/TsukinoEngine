//----------------------------------------------------------------------------
//! @file   BodyHandle.hpp
//! @brief  物理ボディ／キャラクターの不透明ハンドル
//! @detail 物理エンジン（Jolt）の型を上位モジュールへ露出させないための薄い
//!         ラッパです。BodyHandle の値は Jolt のボディID と同一表現に保って
//!         あるため、Tsukino.Physics の内部では無変換で相互に読み替えられます。
//----------------------------------------------------------------------------
#pragma once
#include <cstdint>

// 名前空間 : Tsukino::Physics
namespace Tsukino::Physics {

    //! 無効なハンドルを表す値（Jolt の cInvalidBodyID と同一）
    inline constexpr uint32_t cInvalidHandleValue = 0xffffffffu;

    //------------------------------------------------------------------------
    //! 物理ボディを指す不透明ハンドル
    //------------------------------------------------------------------------
    struct BodyHandle {
        uint32_t value = cInvalidHandleValue;    //!< 物理エンジン内部のボディ識別子

        //! 有効なボディを指しているかどうかを返します。
        //! @return 有効なら true
        bool IsValid() const { return value != cInvalidHandleValue; }

        //! 同じボディを指しているかどうかを返します。
        //! @param  [in] rhs 比較対象
        //! @return 同一なら true
        bool operator==(const BodyHandle& rhs) const { return value == rhs.value; }

        //! 異なるボディを指しているかどうかを返します。
        //! @param  [in] rhs 比較対象
        //! @return 異なるなら true
        bool operator!=(const BodyHandle& rhs) const { return value != rhs.value; }
    };

    //------------------------------------------------------------------------
    //! キャラクターコントローラーを指す不透明ハンドル
    //------------------------------------------------------------------------
    struct CharacterHandle {
        uint32_t value = cInvalidHandleValue;    //!< PhysicsWorld が払い出す識別子

        //! 有効なキャラクターを指しているかどうかを返します。
        //! @return 有効なら true
        bool IsValid() const { return value != cInvalidHandleValue; }

        //! 同じキャラクターを指しているかどうかを返します。
        //! @param  [in] rhs 比較対象
        //! @return 同一なら true
        bool operator==(const CharacterHandle& rhs) const { return value == rhs.value; }

        //! 異なるキャラクターを指しているかどうかを返します。
        //! @param  [in] rhs 比較対象
        //! @return 異なるなら true
        bool operator!=(const CharacterHandle& rhs) const { return value != rhs.value; }
    };

}    // namespace Tsukino::Physics
