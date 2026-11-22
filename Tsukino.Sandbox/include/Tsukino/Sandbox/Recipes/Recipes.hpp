//--------------------------------------------------------------
//! @file   Recipes.hpp
//! @brief  レシピ（やりたいことごとの短いお手本）の宣言
//! @note   使い方と挙動の説明は Tsukino.Sandbox/Recipes/README.md にまとめてある。
//!         各レシピの実装は Tsukino.Sandbox/src/Recipes/ の同名の .cpp。
//!         全部を1つのシーンで動かす例は RecipeScene（Tsukino.Sandbox.exe recipes で起動）。
//--------------------------------------------------------------
#pragma once
#include <Tsukino/Core/ECS/EntityRef/EntityRef.hpp>
#include <Tsukino/Core/ECS/Event/ScopedConnection.hpp>
#include <Tsukino/Core/ECS/System/ISystem.hpp>
#include <Tsukino/Engine/Asset/AssetHandle.hpp>
#include <Tsukino/Engine/ECS/Scene.hpp>

#include <hlsl++.h>

// 名前空間 : Tsukino（前方宣言）
namespace Tsukino {
    namespace EngineIntegration {
        struct EngineContext;
    }
    namespace BuiltIn::ECS {
        struct CollisionEnterEvent;
    }
}    // namespace Tsukino

// 名前空間 : Tsukino::Sandbox::Recipes
namespace Tsukino::Sandbox::Recipes {

    //==============================================================
    // シーンの土台（RecipeScene.cpp）
    //==============================================================

    //--------------------------------------------------------------
    //! @brief 3D の描画と物理に要る組み込みシステムを、正しい順番で登録します。
    //! @param scene [in,out] 登録先のシーン
    //--------------------------------------------------------------
    void AddCoreSystems(Tsukino::ECS::Scene& scene);

    //--------------------------------------------------------------
    //! @brief メインカメラ（注視点を見る）とディレクショナルライトを置きます。
    //! @param scene  [in,out] 置き先のシーン
    //! @param eye    [in]     カメラの位置
    //! @param target [in]     カメラが見る点
    //--------------------------------------------------------------
    void AddCameraAndLight(Tsukino::ECS::Scene& scene, const hlslpp::float3& eye, const hlslpp::float3& target);

    //==============================================================
    // 物理（PhysicsRecipe.cpp）
    //==============================================================

    //--------------------------------------------------------------
    //! @brief 動かない床を置きます（見た目は組み込みの平面、当たり判定は厚みのある箱）。
    //! @param scene   [in,out] 置き先のシーン
    //! @param context [in]     エンジンのコンテキスト（組み込みモデルを使う）
    //! @param size    [in]     床の一辺の長さ
    //! @return 床のエンティティ
    //--------------------------------------------------------------
    Tsukino::ECS::Entity AddStaticFloor(Tsukino::ECS::Scene& scene, Tsukino::EngineIntegration::EngineContext& context, float size);

    //--------------------------------------------------------------
    //! @brief 重力で動く球を置きます。
    //! @param scene    [in,out] 置き先のシーン
    //! @param context  [in]     エンジンのコンテキスト（組み込みモデルを使う）
    //! @param position [in]     最初の位置
    //! @param diameter [in]     直径
    //! @return 球のエンティティ
    //--------------------------------------------------------------
    Tsukino::ECS::Entity AddDynamicSphere(Tsukino::ECS::Scene&                       scene,
                                          Tsukino::EngineIntegration::EngineContext& context,
                                          const hlslpp::float3&                      position,
                                          float                                      diameter);

    //==============================================================
    // 衝突したら音を鳴らす（CollisionSoundRecipe.cpp）
    //==============================================================

    //! 衝突した瞬間に鳴らす音の設定です（付けたエンティティが何かに当たると鳴る）。
    struct CollisionSoundComponent {
        Tsukino::Asset::AssetHandle sound;               // 鳴らす音（AssetManager::Load した .wav / .mp3 / .flac）
        float                       volume    = 1.0f;    // 音量（0〜1）
        int                         playCount = 0;       // 鳴らした回数（確認用）
    };

    //! 衝突イベントを受け取り、CollisionSoundComponent の音を鳴らすシステムです。
    class CollisionSoundSystem : public Tsukino::ECS::ISystem {
    public:
        //--------------------------------------------------------------
        //! @brief 衝突イベントの購読を始めます。
        //! @param eventBus [in] シーンのイベントバス（PhysicsSystem に渡したものと同じ）
        //! @param registry [in] シーンのレジストリ
        //--------------------------------------------------------------
        CollisionSoundSystem(Tsukino::ECS::EventBus& eventBus, Tsukino::ECS::Registry& registry);

        void Update(Tsukino::ECS::Registry& registry, float deltaTime) override;

    private:
        void OnCollisionEnter(const Tsukino::BuiltIn::ECS::CollisionEnterEvent& event);

        Tsukino::ECS::Registry*        m_registry = nullptr;
        Tsukino::ECS::ScopedConnection m_connection;    // 破棄時に購読を自動で解除する
    };

    //==============================================================
    // 別のエンティティを参照する（FollowRecipe.cpp）
    //==============================================================

    //! 指定したエンティティ（target）の位置へ、offset だけずらして追いかけます。
    struct FollowComponent {
        Tsukino::ECS::EntityRef target;                     // 追いかける相手（作るときに entity を入れる）
        hlslpp::float3          offset = hlslpp::float3(0.0f, 1.0f, 0.0f);    // 相手からのずれ
        float                   speed  = 2.0f;              // 1秒あたりに縮める距離の割合
    };

    //! FollowComponent を持つエンティティを、相手の位置へ近づけるシステムです。
    class FollowSystem : public Tsukino::ECS::ISystem {
    public:
        void Update(Tsukino::ECS::Registry& registry, float deltaTime) override;
    };

    //==============================================================
    // 自作シェーダーで描く（CustomShaderRecipe.cpp）
    //==============================================================

    //! 2つの色の間を時間で行き来させる設定です（色は CustomShaderComponent::params でシェーダーへ渡す）。
    struct ColorCycleComponent {
        hlslpp::float4 colorA  = hlslpp::float4(1.0f, 0.2f, 0.2f, 1.0f);    // 色A
        hlslpp::float4 colorB  = hlslpp::float4(0.2f, 0.4f, 1.0f, 1.0f);    // 色B
        float          period  = 2.0f;                                     // A → B → A の1往復の秒数
        float          elapsed = 0.0f;                                     // 経過時間（秒）
        float          mix     = 0.0f;                                     // 今の混ぜ具合（0 = A、1 = B。確認用）
    };

    //! ColorCycleComponent の色を毎フレーム CustomShaderComponent::params へ書くシステムです。
    class ColorCycleSystem : public Tsukino::ECS::ISystem {
    public:
        void Update(Tsukino::ECS::Registry& registry, float deltaTime) override;
    };

    //--------------------------------------------------------------
    //! @brief 自作シェーダー（Tsukino.Sandbox/Assets/Shaders/RecipeColor.ps.sc）で描く立方体を置きます。
    //! @param scene    [in,out] 置き先のシーン
    //! @param context  [in]     エンジンのコンテキスト
    //! @param position [in]     位置
    //! @return 立方体のエンティティ
    //--------------------------------------------------------------
    Tsukino::ECS::Entity AddColorCycleCube(Tsukino::ECS::Scene& scene, Tsukino::EngineIntegration::EngineContext& context, const hlslpp::float3& position);

    //==============================================================
    // 実行中に生成・破棄する（SpawnRecipe.cpp）
    //==============================================================

    //! 一定の間隔でエンティティを生成する設定です。
    struct SpawnerComponent {
        float interval     = 1.0f;    // 生成の間隔（秒）
        float lifetime     = 3.0f;    // 生成したものが消えるまでの秒数（LifetimeComponent に入れる）
        float timer        = 0.0f;    // 次の生成までの経過時間
        int   spawnedCount = 0;       // 生成した数（確認用）
    };

    //! SpawnerComponent の位置へ、一定の間隔で小さな立方体を生成するシステムです。
    class SpawnerSystem : public Tsukino::ECS::ISystem {
    public:
        //! @param scene [in] 生成先のシーン（CreateEntity を使う）
        explicit SpawnerSystem(Tsukino::ECS::Scene& scene) : m_scene(&scene) {}

        void Update(Tsukino::ECS::Registry& registry, float deltaTime) override;

    private:
        Tsukino::ECS::Scene* m_scene = nullptr;
    };
}    // namespace Tsukino::Sandbox::Recipes
