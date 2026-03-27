//-------------------------------------------------------------
//! @file   TransformComponent.hpp
//! @brief  TransformComponentクラスの宣言
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once
#define NOMINMAX
#include <Tsukino/Core/Math/Matrix.hpp>
#include <Tsukino/Core/ECS/Entity/Entity.hpp>

#include <hlsl++.h>
// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {
    //-------------------------------------------------------------
    //! @class  TransformComponent
    //! @brief  位置、回転、スケールを管理するクラス
    //-------------------------------------------------------------
    struct TransformComponent {
        hlslpp::float3     position;    // 位置
        hlslpp::quaternion rotation;    // 回転
        hlslpp::float3     scale;       // スケール

        Tsukino::Core::Math::matrix localMatrix;    // ローカル行列
        Tsukino::Core::Math::matrix worldMatrix;    // ワールド行列

        Tsukino::ECS::Entity parent;           // 親エンティティ
        bool                 dirty = false;    // ワールド行列が更新されているか
    };

}    // namespace Tsukino::BuiltIn::ECS
