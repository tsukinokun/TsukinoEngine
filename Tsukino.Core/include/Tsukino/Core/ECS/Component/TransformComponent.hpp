//-------------------------------------------------------------
//! @file   TransformComponent.hpp
//! @brief  TransformComponentクラスの宣言
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once
#include <hlsl++.h>
#include <Tsukino/Core/Math/Matrix.hpp>
#include <Tsukino/Core/ECS/Entity/Entity.hpp>
// 名前空間 : Tsukino::ECS
namespace Tsukino::ECS {
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

}    // Tsukino::ECS
