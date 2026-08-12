//-------------------------------------------------------------
//! @file   LuckGameSampleSceneUISystem.hpp
//! @brief  LuckGameSampleSceneUISystemクラスの宣言
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once
#include <Tsukino/Core/ECS/System/ISystem.hpp>

// 名前空間 : LuckGameSampleScene::ECS
namespace LuckGameSampleScene::ECS {

    //-------------------------------------------------------------
    //! @class  LuckGameSampleSceneUISystem
    //! @brief  CPU役/プレイヤー役/メッセージの各ラベル(FontComponent::text)を、
    //!         GameStateComponentやPlayerComponentの状態に応じて更新するシステム。
    //!         優先度は FontRendererSystem より前に登録すること。
    //-------------------------------------------------------------
    class LuckGameSampleSceneUISystem : public Tsukino::ECS::ISystem {
    public:
        void Update(Tsukino::ECS::Registry& registry, float deltaTime) override;
    };

}    // namespace LuckGameSampleScene::ECS
