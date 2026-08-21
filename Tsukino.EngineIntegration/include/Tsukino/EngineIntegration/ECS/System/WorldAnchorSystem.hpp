//-------------------------------------------------------------
//! @file   WorldAnchorSystem.hpp
//! @brief  WorldAnchorSystemクラスの宣言
//-------------------------------------------------------------
#pragma once
#include <Tsukino/Core/ECS/System/ISystem.hpp>
// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {
    //-------------------------------------------------------------
    //! @class  WorldAnchorSystem
    //! @brief  WorldAnchorComponentを持つエンティティについて、targetのワールド座標
    //!         （+worldOffset）をメインカメラでスクリーン座標へ投影し、自分自身の
    //!         TransformComponent.positionへ書き込むシステム。
    //!         カメラ行列が確定した後（CameraSystemの後）かつ、書き込んだ座標を
    //!         FontRendererSystem等が読むworldMatrixへ反映する2回目のTransformSystem
    //!         （UI用）より前に実行すること
    //-------------------------------------------------------------
    class WorldAnchorSystem : public Tsukino::ECS::ISystem {
    public:
        //-------------------------------------------------------------
        //! @brief 更新処理
        //-------------------------------------------------------------
        void Update(Tsukino::ECS::Registry& registry, float deltaTime) override;
    };
}    // namespace Tsukino::BuiltIn::ECS
