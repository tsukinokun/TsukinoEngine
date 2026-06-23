//-------------------------------------------------------------
//! @file   BoxCollider2DComponent.hpp
//! @brief  BoxCollider2DComponentクラスの宣言
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once
#include <hlsl++.h>
// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {
    //-------------------------------------------------------------
    //! @class  BoxCollider2DComponent
    //! @brief  2Dボックスコリジョンコンポーネント
    //-------------------------------------------------------------
    struct BoxCollider2DComponent {
        hlslpp::float2 offset;
        hlslpp::float2 size;
    };

}    // namespace Tsukino::BuiltIn::ECS
