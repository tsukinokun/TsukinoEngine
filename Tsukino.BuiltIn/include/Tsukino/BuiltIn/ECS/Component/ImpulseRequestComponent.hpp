//-------------------------------------------------------------
//! @file   ImpulseRequestComponent.hpp
//! @brief  ImpulseRequestComponentクラスの宣言
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once
#include <Tsukino/Core/ECS/Entity/Entity.hpp>

#include <hlsl++.h>
// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {
    //-------------------------------------------------------------
    //! @class  ImpulseRequestComponent
    //! @brief  物理エンジンへの衝撃要求を表すコンポーネント
    //-------------------------------------------------------------
    struct ImpulseRequestComponent {
        hlslpp::float3 impulse;    //!< 衝撃のベクトル (方向と大きさ)
    };

}    // namespace Tsukino::BuiltIn::ECS
