//--------------------------------------------------------------
//! @file   Recipes.hpp
//! @brief  レシピ（やりたいことごとの短いお手本）の宣言
//! @note   説明は GAMEDEV.md と Tsukino.Sandbox/Recipes/README.md。
//!         全部を1つのシーンで動かす例は RecipeScene（Tsukino.Sandbox.exe recipes で起動）。
//--------------------------------------------------------------
#pragma once
#include <Tsukino/Engine/ECS/Scene.hpp>

// 名前空間 : Tsukino::Sandbox::Recipes
namespace Tsukino::Sandbox::Recipes {
    //--------------------------------------------------------------
    //! @brief Prefab JSON のグループから、参照（EntityRef）でつながった2体を作ります（PrefabRecipe.cpp）。
    //! @param scene [in,out] 作り先のシーン
    //--------------------------------------------------------------
    void AddPrefabRecipe(Tsukino::ECS::Scene& scene);
}    // namespace Tsukino::Sandbox::Recipes
