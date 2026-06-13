//-------------------------------------------------------------
//! @file   DebugCameraTag.hpp
//! @brief  DebugCameraTagクラスの宣言
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once
// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {
    //-------------------------------------------------------------
    //! @class  DebugCameraTag
    //! @brief  デバッグカメラ用のタグコンポーネント
    //-------------------------------------------------------------
    struct DebugCameraTag {
        bool dummy = true;
    };
}    // namespace Tsukino::BuiltIn::ECS
