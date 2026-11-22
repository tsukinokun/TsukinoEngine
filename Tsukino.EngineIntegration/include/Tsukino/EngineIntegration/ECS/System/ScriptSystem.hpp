//--------------------------------------------------------------
//! @file   ScriptSystem.hpp
//! @brief  ScriptComponent の処理を呼ぶシステム
//--------------------------------------------------------------
#pragma once
#include <Tsukino/EngineIntegration/ECS/Script/ScriptComponent.hpp>
#include <Tsukino/Core/ECS/Event/ScopedConnection.hpp>
#include <Tsukino/Core/ECS/System/ISystem.hpp>

// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {
    struct CollisionEnterEvent;    // 前方宣言

    //--------------------------------------------------------------
    //! @class  ScriptSystem
    //! @brief  ScriptComponent の onStart / onUpdate / onCollisionEnter を呼ぶシステム
    //! @note   物理より前に回すと、onUpdate で書き換えた位置（Kinematic など）が同じフレームの物理に効く。
    //!         AddDefaultSystems はそう登録する。
    //--------------------------------------------------------------
    class ScriptSystem : public Tsukino::ECS::ISystem {
    public:
        //--------------------------------------------------------------
        //! @brief 衝突イベントの購読を始めます。
        //! @param scene [in] 所属するシーン（ScriptContext に渡す。システムより長く生きる）
        //--------------------------------------------------------------
        explicit ScriptSystem(Tsukino::ECS::Scene& scene);

        //--------------------------------------------------------------
        //! @brief onStart（初回だけ）と onUpdate を呼びます。
        //! @param registry  [in] エンティティレジストリ
        //! @param deltaTime [in] デルタタイム（秒）
        //--------------------------------------------------------------
        void Update(Tsukino::ECS::Registry& registry, float deltaTime) override;

    private:
        //--------------------------------------------------------------
        //! @brief 当たった側（self）の onCollisionEnter を呼びます。
        //--------------------------------------------------------------
        void OnCollisionEnter(const CollisionEnterEvent& event);

        Tsukino::ECS::Scene*           m_scene = nullptr;
        float                          m_time  = 0.0f;    // シーンの経過時間（秒）
        Tsukino::ECS::ScopedConnection m_connection;      // 破棄されると購読も外れる
    };
}    // namespace Tsukino::BuiltIn::ECS
