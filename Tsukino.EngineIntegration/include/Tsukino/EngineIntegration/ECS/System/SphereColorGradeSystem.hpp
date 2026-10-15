//--------------------------------------------------------------
//! @file   SphereColorGradeSystem.hpp
//! @brief  球形の色調補正システムの宣言
//--------------------------------------------------------------
#pragma once
#include <Tsukino/Core/ECS/System/ISystem.hpp>
// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {
    //--------------------------------------------------------------
    //! @class  SphereColorGradeSystem
    //! @brief  SphereColorGradeComponentのパラメータをRendererへ転送するシステム
    //--------------------------------------------------------------
    class SphereColorGradeSystem : public Tsukino::ECS::ISystem {
    public:
        //--------------------------------------------------------------
        //! @brief 更新処理
        //--------------------------------------------------------------
        void Update(Tsukino::ECS::Registry& registry, float deltaTime) override;
    };
}    // namespace Tsukino::BuiltIn::ECS
