//--------------------------------------------------------------
//! @file   SceneHelpers.hpp
//! @brief  シーンを作るときの定型（システムの登録・カメラ・ライト・基本図形・音）を1行で書くための関数
//--------------------------------------------------------------
#pragma once
#include <Tsukino/Engine/Asset/AssetHandle.hpp>
#include <Tsukino/Engine/ECS/Scene.hpp>
#include <Tsukino/Core/ECS/Entity/Entity.hpp>
#include <Tsukino/Core/typedef.hpp>

#include <hlsl++.h>

#include <string>

// 名前空間 : Tsukino::EngineIntegration
namespace Tsukino::EngineIntegration {
    struct EngineContext;    // 前方宣言

    //! CreatePrimitive で置く形（組み込みモデル。いずれも原点中心・大きさ 1）
    enum class PrimitiveShape : u8 {
        Cube,      //!< 立方体（1 x 1 x 1）
        Plane,     //!< 平面（XZ の 1 x 1、Y = 0、+Y 向き）
        Sphere,    //!< 球（直径 1）
    };

    //! CreatePrimitive で付ける物理の種類
    enum class PhysicsBody : u8 {
        None,         //!< 見た目だけ（当たり判定も剛体も付けない）
        Static,       //!< 動かない（床・壁）
        Dynamic,      //!< 重力と衝突で動く。位置は物理が毎フレーム上書きする
        Kinematic,    //!< 自分で TransformComponent::position を書き換えて動かす。物理はそこへ付いてくる
    };

    //--------------------------------------------------------------
    //! @brief 3D ゲームに要る組み込みシステムを、正しい順番でまとめて登録します。
    //! @param scene [in,out] 登録先のシーン
    //! @note  Script → Physics → Transform → Camera → Model → Light → Audio → Lifetime の順。
    //!        ScriptComponent・AudioComponent・LifetimeComponent もこれだけで動く。
    //!        ゲーム固有のシステムは scene.AddSystem(…, 優先度) で足す（小さいほど先。Script は -1、Physics は 0）
    //--------------------------------------------------------------
    void AddDefaultSystems(Tsukino::ECS::Scene& scene);

    //--------------------------------------------------------------
    //! @brief 注視点を見るメインカメラを置きます。
    //! @param scene  [in,out] 置き先のシーン
    //! @param eye    [in]     カメラの位置
    //! @param target [in]     見る点
    //! @return カメラのエンティティ
    //--------------------------------------------------------------
    Tsukino::ECS::Entity AddCamera(Tsukino::ECS::Scene& scene, const hlslpp::float3& eye, const hlslpp::float3& target);

    //--------------------------------------------------------------
    //! @brief 影を落とすディレクショナルライトを置きます（無いと通常のモデルは真っ暗）。
    //! @param scene     [in,out] 置き先のシーン
    //! @param direction [in]     光の向き
    //! @param intensity [in]     強さ
    //! @return ライトのエンティティ
    //--------------------------------------------------------------
    Tsukino::ECS::Entity AddDirectionalLight(Tsukino::ECS::Scene& scene,
                                             const hlslpp::float3& direction = hlslpp::float3(0.4f, -1.0f, 0.6f),
                                             float                 intensity = 3.0f);

    //--------------------------------------------------------------
    //! @brief 基本図形を置きます（Unity の GameObject.CreatePrimitive に当たる）。
    //! @param scene    [in,out] 置き先のシーン
    //! @param shape    [in]     形
    //! @param position [in]     位置（中心）
    //! @param scale    [in]     大きさ（Transform の scale。各辺の長さになる）
    //! @param body     [in]     物理の種類（None 以外なら、大きさに合った当たり判定と剛体も付ける）
    //! @return 置いたエンティティ（TransformComponent・ModelComponent、物理ありなら CollisionComponent・RigidbodyComponent）
    //! @note  当たり判定は scale から作る：Cube は各辺の半分、Sphere は scale の最大値を直径、
    //!        Plane は厚さ 1 の箱で上面を Y = position.y に合わせる（薄いとすり抜けるため）。
    //!        後から変えるなら CollisionComponent / RigidbodyComponent を直接書き換える
    //--------------------------------------------------------------
    Tsukino::ECS::Entity CreatePrimitive(Tsukino::ECS::Scene&  scene,
                                         PrimitiveShape        shape,
                                         const hlslpp::float3& position,
                                         const hlslpp::float3& scale = hlslpp::float3(1.0f, 1.0f, 1.0f),
                                         PhysicsBody           body  = PhysicsBody::None);

    //--------------------------------------------------------------
    //! @brief 既にあるエンティティ（Prefab から作ったものなど）に、基本図形の見た目と物理を付けます。
    //! @param scene  [in,out] エンティティのシーン
    //! @param entity [in]     付け先（TransformComponent が無ければ付ける。大きさはその scale を使う）
    //! @param shape  [in]     形
    //! @param body   [in]     物理の種類
    //! @note  既に付いている ModelComponent / CollisionComponent / RigidbodyComponent は上書きする
    //--------------------------------------------------------------
    void AttachPrimitive(Tsukino::ECS::Scene& scene, Tsukino::ECS::Entity entity, PrimitiveShape shape, PhysicsBody body = PhysicsBody::None);

    //--------------------------------------------------------------
    //! @brief シーンのエンジンコンテキスト（AssetManager・AudioManager など）を取得します。
    //! @param scene [in] シーン
    //! @return エンジンコンテキスト
    //--------------------------------------------------------------
    EngineContext& GetEngineContext(Tsukino::ECS::Scene& scene);

    //--------------------------------------------------------------
    //! @brief アセットを読み込みます（モデル・音・テクスチャ・シェーダーなど、拡張子で種類が決まる）。
    //! @param scene [in] シーン
    //! @param path  [in] アセットのルートからの相対パス（例: "Assets/Sounds/hit.wav"）
    //! @return ハンドル（失敗すると無効なハンドル。理由はログに出る）
    //--------------------------------------------------------------
    Tsukino::Asset::AssetHandle LoadAsset(Tsukino::ECS::Scene& scene, const std::string& path);

    //--------------------------------------------------------------
    //! @brief 読み込んだ音を鳴らします。
    //! @param scene  [in] シーン
    //! @param sound  [in] LoadAsset で読んだ音（.wav / .mp3 / .flac）
    //! @param volume [in] 音量（0〜1）
    //! @param loop   [in] ループするか
    //! @return true: AudioManager::Play を呼んだ / false: 音ではない・読めていない（ログに理由を出す）
    //! @note  名前が PlaySound でないのは、Windows の mmsystem.h が PlaySound をマクロで定義しているため
    //--------------------------------------------------------------
    bool PlayAudio(Tsukino::ECS::Scene& scene, Tsukino::Asset::AssetHandle sound, float volume = 1.0f, bool loop = false);
}    // namespace Tsukino::EngineIntegration
