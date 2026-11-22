//--------------------------------------------------------------
//! @file   RecipeScene.cpp
//! @brief  レシピ: SceneHelpers と ScriptComponent だけで、物理・衝突・音・追従・自作シェーダー・生成と破棄を1つのシーンに書く
//! @note   ゲームのシーンもこの形で書ける（自作の System クラスやヘッダは要らない）。説明は GAMEDEV.md
//--------------------------------------------------------------
#include <Tsukino/Sandbox/Recipes/RecipeScene.hpp>
#include <Tsukino/Sandbox/Recipes/Recipes.hpp>

#include <Tsukino/EngineIntegration/EngineContext.hpp>
#include <Tsukino/EngineIntegration/Scene/SceneHelpers.hpp>
#include <Tsukino/EngineIntegration/ECS/Script/ScriptComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/CollisionComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/RigidbodyComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/CustomShaderComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/LifetimeComponent.hpp>
#include <Tsukino/Renderer/Renderer.hpp>
#include <Tsukino/Core/Log.hpp>

#include <algorithm>
#include <cmath>
#include <string>

// 名前空間 : Tsukino::Sandbox::Recipes
namespace Tsukino::Sandbox::Recipes {
    using namespace Tsukino::EngineIntegration;
    using namespace Tsukino::BuiltIn::ECS;

    //--------------------------------------------------------------
    //! @brief 全レシピを並べます。
    //--------------------------------------------------------------
    void RecipeScene::OnInitialize(Tsukino::EngineIntegration::EngineAPI& /*api*/) {
        Tsukino::ECS::Registry& registry = m_scene.GetRegistry();
        GetEngineContext(m_scene).renderer->SetClearColor(0.10f, 0.12f, 0.18f, 1.0f);

        // 土台：システム（正しい順）・カメラ・ライト・床
        AddDefaultSystems(m_scene);
        AddCamera(m_scene, hlslpp::float3(0.0f, 5.0f, -10.0f), hlslpp::float3(0.0f, 1.0f, 0.0f));
        AddDirectionalLight(m_scene);
        CreatePrimitive(m_scene, PrimitiveShape::Plane, hlslpp::float3(0.0f, 0.0f, 0.0f), hlslpp::float3(12.0f, 1.0f, 12.0f), PhysicsBody::Static);

        //----------------------------------------------------------
        // 1. 落ちて床に当たったら音を鳴らす球（Dynamic × Static は衝突イベントが出る）
        //----------------------------------------------------------
        const Tsukino::Asset::AssetHandle sound = LoadAsset(m_scene, "Tsukino.Sandbox/Assets/Sounds/cat1.wav");
        Tsukino::ECS::Entity ball = CreatePrimitive(m_scene, PrimitiveShape::Sphere, hlslpp::float3(-2.0f, 4.0f, 0.0f), hlslpp::float3(1.0f, 1.0f, 1.0f), PhysicsBody::Dynamic);
        registry.GetComponent<RigidbodyComponent>(ball).restitution = 0.3f;    // 少し跳ねる（既定は 0）
        registry.AddComponent<ScriptComponent>(ball).onCollisionEnter = [sound, count = 0](ScriptContext& ctx, Tsukino::ECS::Entity /*other*/) mutable {
            PlayAudio(ctx.scene, sound);
            Tsukino::Core::Log::Info("[Recipe] ball hit something (count=" + std::to_string(++count) + ")");
        };

        //----------------------------------------------------------
        // 2. 別のエンティティを追いかける（相手の Entity をラムダに持たせるだけ。破棄済みかは IsValid で確かめる）
        //----------------------------------------------------------
        Tsukino::ECS::Entity follower = CreatePrimitive(m_scene, PrimitiveShape::Sphere, hlslpp::float3(-4.0f, 1.0f, 2.0f), hlslpp::float3(0.3f, 0.3f, 0.3f));
        registry.AddComponent<ScriptComponent>(follower).onUpdate = [ball](ScriptContext& ctx, float deltaTime) {
            if(!ctx.registry.IsValid(ball))
                return;
            auto&                self = ctx.Get<TransformComponent>();
            const hlslpp::float3 goal = ctx.registry.GetComponent<TransformComponent>(ball).position + hlslpp::float3(0.0f, 1.0f, 0.0f);
            const float          t    = std::clamp(2.0f * deltaTime, 0.0f, 1.0f);
            self.position             = hlslpp::lerp(self.position, goal, hlslpp::float3(t, t, t));
            self.dirty                = true;    // Transform を書き換えたら必ず立てる
        };

        //----------------------------------------------------------
        // 3. 自作シェーダーで描き、色を時間で変える（params[0] が u_user0[0] に届く）
        //----------------------------------------------------------
        Tsukino::ECS::Entity colored = CreatePrimitive(m_scene, PrimitiveShape::Cube, hlslpp::float3(2.0f, 0.5f, 0.0f));
        registry.AddComponent<CustomShaderComponent>(colored).pixelShader = LoadAsset(m_scene, "Tsukino.Sandbox/Assets/Shaders/RecipeColor.ps.sc");
        registry.AddComponent<ScriptComponent>(colored).onUpdate = [](ScriptContext& ctx, float /*deltaTime*/) {
            const float mix                                  = 0.5f - 0.5f * std::cos(ctx.time * 3.14159265f);    // 0 → 1 → 0 を2秒で
            ctx.Get<CustomShaderComponent>().params[0] = hlslpp::lerp(hlslpp::float4(1.0f, 0.2f, 0.2f, 1.0f), hlslpp::float4(0.2f, 0.4f, 1.0f, 1.0f), hlslpp::float4(mix, mix, mix, mix));
        };

        //----------------------------------------------------------
        // 4. 一定間隔で生成し、時間で消す（LifetimeComponent。自分で消すなら ctx.Destroy(entity)）
        //----------------------------------------------------------
        Tsukino::ECS::Entity spawner = m_scene.CreateEntity();
        registry.AddComponent<ScriptComponent>(spawner).onUpdate = [timer = 0.0f](ScriptContext& ctx, float deltaTime) mutable {
            timer += deltaTime;
            if(timer < 1.0f)
                return;
            timer -= 1.0f;
            Tsukino::ECS::Entity cube = CreatePrimitive(ctx.scene, PrimitiveShape::Cube, hlslpp::float3(0.0f, 3.0f, 3.0f), hlslpp::float3(0.3f, 0.3f, 0.3f));
            ctx.registry.AddComponent<LifetimeComponent>(cube).remaining = 3.0f;
            Tsukino::Core::Log::Info("[Recipe] spawned a cube");
        };

        //----------------------------------------------------------
        // 5. 自分で動かす物（Kinematic）で当たりを取る：Kinematic × Static では衝突イベントが出ないので、
        //    当てられる側をセンサー（isSensor）にする。当たったら相手を破棄する
        //----------------------------------------------------------
        Tsukino::ECS::Entity mover = CreatePrimitive(m_scene, PrimitiveShape::Cube, hlslpp::float3(-5.0f, 0.5f, -2.0f), hlslpp::float3(0.5f, 0.5f, 0.5f), PhysicsBody::Kinematic);
        registry.AddComponent<ScriptComponent>(mover).onUpdate = [](ScriptContext& ctx, float deltaTime) {
            auto& transform       = ctx.Get<TransformComponent>();
            transform.position.x += 2.0f * deltaTime;    // Kinematic は Transform を書き換えて動かす（物理が付いてくる）
            transform.dirty       = true;
        };
        Tsukino::ECS::Entity sensor = CreatePrimitive(m_scene, PrimitiveShape::Cube, hlslpp::float3(-1.0f, 0.5f, -2.0f), hlslpp::float3(0.5f, 0.5f, 0.5f), PhysicsBody::Static);
        registry.GetComponent<CollisionComponent>(sensor).isSensor   = true;
        registry.AddComponent<ScriptComponent>(sensor).onCollisionEnter = [](ScriptContext& ctx, Tsukino::ECS::Entity /*other*/) {
            Tsukino::Core::Log::Info("[Recipe] sensor touched, destroying it");
            ctx.DestroySelf();    // 予約。全システムの更新が終わってから消える
        };

        // 6. Prefab JSON で複数のエンティティと参照（EntityRef）を作る（PrefabRecipe.cpp）
        AddPrefabRecipe(m_scene);

        Tsukino::Core::Log::Info("[Recipe] RecipeScene initialized.");
    }

    //--------------------------------------------------------------
    //! @brief 登録したシステムを回します。
    //--------------------------------------------------------------
    void RecipeScene::OnUpdate(Tsukino::EngineIntegration::EngineAPI& /*api*/, float deltaTime) {
        m_scene.Update(deltaTime);
    }

    //--------------------------------------------------------------
    //! @brief シーンの終了処理（Scene::Shutdown は呼び出し側が行う）。
    //--------------------------------------------------------------
    void RecipeScene::OnExit() {}
}    // namespace Tsukino::Sandbox::Recipes
