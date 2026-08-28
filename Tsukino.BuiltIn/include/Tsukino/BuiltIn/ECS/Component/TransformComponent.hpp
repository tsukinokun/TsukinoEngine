//-------------------------------------------------------------
//! @file   TransformComponent.hpp
//! @brief  TransformComponentクラスの宣言
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once
#include <Tsukino/Core/Math/Matrix.hpp>
#include <Tsukino/Core/ECS/Entity/Entity.hpp>
#include <Tsukino/Core/ECS/EntityRef/EntityRef.hpp>
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

        //--------------------------------------------------------------
        // 親エンティティ。
        // EntityRef は Entity への暗黙変換と operator= を持つ透過ラッパーなので
        // 「parent = entt::null;」「parent == entt::null」といった従来の書き方は
        // そのまま使える。EntityRef にしてあるのは Prefab(JSON) 側に
        // "parent": "#PenguinCenter" と名前で階層を書けるようにするため
        // （生の entt::entity は index+version でプロセス内でしか意味を持たない）
        //--------------------------------------------------------------
        Tsukino::ECS::EntityRef parent;

        //--------------------------------------------------------------
        // ローカル／ワールド行列の再計算が必要か。
        // 既定を true にしているのは、Tsukino::Core::Math::matrix の既定
        // コンストラクタが単位行列ではなく「ゼロ行列」のため。
        // dirty を立て忘れたまま生成されたエンティティは worldMatrix が
        // ゼロのままになり、それを読む描画側から消えてしまう
        //--------------------------------------------------------------
        bool dirty = true;
    };

}    // namespace Tsukino::BuiltIn::ECS
