//-------------------------------------------------------------
//! @file   TransformComponent.hpp
//! @brief  TransformComponentクラスの宣言
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once
#include <Tsukino/Core/Math/Matrix.hpp>
#include <Tsukino/Core/ECS/Entity/Entity.hpp>
// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {
    //-------------------------------------------------------------
    //! @class  TransformComponent
    //! @brief  位置、回転、スケールを管理するクラス
    //-------------------------------------------------------------
    struct TransformComponent {
        hlslpp::float3     position = hlslpp::float3(0.0f, 0.0f, 0.0f);    // 位置
        hlslpp::quaternion rotation = hlslpp::quaternion(0.0f, 0.0f, 0.0f, 1.0f);    // 回転
        hlslpp::float3     scale = hlslpp::float3(1.0f, 1.0f, 1.0f);       // スケール

        Tsukino::Core::Math::matrix localMatrix;    // ローカル行列
        Tsukino::Core::Math::matrix worldMatrix;    // ワールド行列

        Tsukino::ECS::Entity parent = entt::null;    // 親エンティティ
        bool                 dirty = false;    // ワールド行列が更新されているか
    };

}    // namespace Tsukino::BuiltIn::ECS
