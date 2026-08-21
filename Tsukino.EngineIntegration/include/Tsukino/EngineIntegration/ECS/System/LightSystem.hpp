//--------------------------------------------------------------
//! @file   LightSystem.hpp
//! @brief  ライトシステムの宣言
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
#include <Tsukino/Core/ECS/System/ISystem.hpp>
// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {
    //--------------------------------------------------------------
    //! @class  LightSystem
    //! @brief  DirectionalLight/PointLight/SpotLightコンポーネントを読んで
    //!         Rendererにライト情報（ディレクショナルは影付き1灯、
    //!         点光源・スポットライトはディファードLightingパス用の配列）を渡すシステム
    //--------------------------------------------------------------
    class LightSystem : public Tsukino::ECS::ISystem {
    public:
        //--------------------------------------------------------------
        //! @brief システムの更新
        //! @param registry  [in] ECSレジストリ
        //! @param deltaTime [in] 前フレームからの経過時間
        //--------------------------------------------------------------
        void Update(Tsukino::ECS::Registry& registry, float deltaTime) override;
    };

    //--------------------------------------------------------------
    //! @brief 旧名（DirectionalLightSystem）のエイリアス
    //! @note  既存シーンの AddSystem<DirectionalLightSystem>() 呼び出しを
    //!        無修正で通すために残してある。実体はLightSystem。
    //--------------------------------------------------------------
    using DirectionalLightSystem = LightSystem;
}    // namespace Tsukino::BuiltIn::ECS
