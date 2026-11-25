//--------------------------------------------------------------
//! @file   RecipeScene.hpp
//! @brief  レシピを全部まとめて動かすシーンの宣言（Tsukino.Sandbox.exe recipes で起動）
//--------------------------------------------------------------
#pragma once
#include <Tsukino/EngineIntegration/Scene/GameSceneBase.hpp>

// 名前空間 : Tsukino::Sandbox::Recipes
namespace Tsukino::Sandbox::Recipes {
    //--------------------------------------------------------------
    //! @class  RecipeScene
    //! @brief  レシピ（衝突音・追従・自作シェーダー・生成と破棄・センサー・Prefab）を1つに並べたシーン
    //--------------------------------------------------------------
    class RecipeScene : public Tsukino::EngineIntegration::GameSceneBase {
    public:
        void OnUpdate(Tsukino::EngineIntegration::EngineAPI& api, float deltaTime) override;
        void OnExit() override;

    private:
        void OnInitialize(Tsukino::EngineIntegration::EngineAPI& api) override;
    };
}    // namespace Tsukino::Sandbox::Recipes
