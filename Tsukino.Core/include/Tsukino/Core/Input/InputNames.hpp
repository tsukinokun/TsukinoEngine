//----------------------------------------------------------------------------
//! @file   InputNames.hpp
//! @brief  キー・ゲームパッドのボタンと名前の相互変換
//! @detail 設定ファイル（JSON など）に書く綴りと、画面に出す表記の2種類を扱います。
//!         設定ファイルの綴りは列挙子の名前そのもの（"W" "Space" "LButton" "LeftShoulder" など）です。
//----------------------------------------------------------------------------
#pragma once

#include <Tsukino/Core/Input/GamepadCodes.hpp>
#include <Tsukino/Core/Input/KeyCodes.hpp>

#include <string_view>
// 名前空間 : Tsukino::Input
namespace Tsukino::Input {

    //! 設定ファイルの綴りからキーコードを得ます。
    //! @param  [in] name 列挙子と同じ綴り（大文字小文字は区別しない）
    //! @return 対応するキーコード。見つからなければ KeyCode::None
    [[nodiscard]]
    KeyCode KeyCodeFromName(std::string_view name);

    //! 設定ファイルの綴りからゲームパッドのボタンを得ます。
    //! @param  [in] name 列挙子と同じ綴り（大文字小文字は区別しない）
    //! @return 対応するボタン。見つからなければ GamepadButton::Count
    [[nodiscard]]
    GamepadButton GamepadButtonFromName(std::string_view name);

    //! キーを画面に出すときの表記を得ます（"SPACE" "左クリック" など）。
    //! @param  [in] code キーコード
    //! @return 表記。対応が無ければ空文字
    [[nodiscard]]
    const wchar_t* GetKeyDisplayName(KeyCode code);

    //! ゲームパッドのボタンを画面に出すときの表記を得ます（"A" "LB" "L3" など。Xbox 配置）。
    //! @param  [in] button ボタン
    //! @return 表記。対応が無ければ空文字
    [[nodiscard]]
    const wchar_t* GetGamepadButtonDisplayName(GamepadButton button);

}    // namespace Tsukino::Input
