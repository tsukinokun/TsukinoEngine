//--------------------------------------------------------------
//! @file   CustomShaderRecipe.cpp
//! @brief  レシピ: モデルを自作のシェーダーで描き、色などの値を毎フレーム渡す
//! @note   要点（詳しくは Recipes/README.md の「自作シェーダー」）
//!         - シェーダーは .sc で書き、ゲームの premake5.lua の tsukino_shaders("Assets/Shaders") のフォルダに置く
//!           （ビルド時に変換される。足したらビルドし直すだけでよい）
//!         - AssetManager::Load に .sc のパスを渡して読み、CustomShaderComponent::pixelShader に入れる
//!         - CustomShaderComponent::params[0〜3] の値が、シェーダーの uniform vec4 u_user0[4] に届く
//!         - 描画は ModelSystem が行う（ModelComponent と同じエンティティに付けるだけ。描画コマンドは自分で積まない）
//--------------------------------------------------------------
#include <Tsukino/Sandbox/Recipes/Recipes.hpp>

#include <Tsukino/EngineIntegration/EngineContext.hpp>
#include <Tsukino/BuiltIn/BuiltInAssets.hpp>
#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/ModelComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/CustomShaderComponent.hpp>
#include <Tsukino/Engine/Asset/AssetManager.hpp>
#include <Tsukino/Core/Path.hpp>

#include <cmath>

// 名前空間 : Tsukino::Sandbox::Recipes
namespace Tsukino::Sandbox::Recipes {

    //--------------------------------------------------------------
    //! @brief 自作シェーダーで描く立方体を置きます。
    //--------------------------------------------------------------
    Tsukino::ECS::Entity AddColorCycleCube(Tsukino::ECS::Scene& scene, Tsukino::EngineIntegration::EngineContext& context, const hlslpp::float3& position) {
        Tsukino::ECS::Registry& registry = scene.GetRegistry();
        Tsukino::ECS::Entity    cube     = scene.CreateEntity();

        auto& transform    = registry.AddComponent<Tsukino::BuiltIn::ECS::TransformComponent>(cube);
        transform.position = position;

        auto& model       = registry.AddComponent<Tsukino::BuiltIn::ECS::ModelComponent>(cube);
        model.modelHandle = context.builtinAssets->models.cube;

        // ゲームなら "Assets/Shaders/RecipeColor.ps.sc" のように、tsukino_shaders() のフォルダの .sc のパスで読む
        auto& shader       = registry.AddComponent<Tsukino::BuiltIn::ECS::CustomShaderComponent>(cube);
        shader.pixelShader = context.assetManager->Load(Tsukino::Core::Path("Tsukino.Sandbox/Assets/Shaders/RecipeColor.ps.sc"));

        registry.AddComponent<ColorCycleComponent>(cube);
        return cube;
    }

    //--------------------------------------------------------------
    //! @brief 色を時間で行き来させ、CustomShaderComponent::params[0] へ書きます。
    //--------------------------------------------------------------
    void ColorCycleSystem::Update(Tsukino::ECS::Registry& registry, float deltaTime) {
        constexpr float kTwoPi = 6.28318530718f;

        auto view = registry.View<ColorCycleComponent, Tsukino::BuiltIn::ECS::CustomShaderComponent>();
        for(auto [entity, cycle, shader] : view.each()) {
            cycle.elapsed += deltaTime;
            cycle.mix      = 0.5f - 0.5f * std::cos(kTwoPi * cycle.elapsed / cycle.period);    // 0 → 1 → 0 を period 秒で
            shader.params[0] = hlslpp::lerp(cycle.colorA, cycle.colorB, hlslpp::float4(cycle.mix, cycle.mix, cycle.mix, cycle.mix));
        }
    }
}    // namespace Tsukino::Sandbox::Recipes
