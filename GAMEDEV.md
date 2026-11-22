# TsukinoEngine でゲームを作る（ゲーム作者向けガイド）

ゲームのコードを書くのに要ることはここに全部ある。**エンジンのヘッダや .cpp を読む前に、まずこれだけで書く。**
足りないときだけ「その先を調べる」の順に見る。`External/` 配下（Jolt・entt・bgfx など）は読まない。

## シーンの雛形（自作の System クラス・ヘッダの追加は要らない）

```cpp
#include <Tsukino/EngineIntegration/Scene/GameSceneBase.hpp>
#include <Tsukino/EngineIntegration/Scene/SceneHelpers.hpp>
#include <Tsukino/EngineIntegration/ECS/Script/ScriptComponent.hpp>
#include <Tsukino/EngineIntegration/EngineContext.hpp>
#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/CollisionComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/RigidbodyComponent.hpp>
#include <Tsukino/Core/Log.hpp>

using namespace Tsukino::EngineIntegration;   // AddDefaultSystems, CreatePrimitive, PrimitiveShape, PhysicsBody, LoadAsset, PlayAudio
using namespace Tsukino::BuiltIn::ECS;        // 組み込みコンポーネント, ScriptComponent, ScriptContext

class MyScene : public GameSceneBase {
public:
    void OnUpdate(EngineAPI&, float deltaTime) override { m_scene.Update(deltaTime); }
    void OnExit() override {}
private:
    void OnInitialize(EngineAPI&) override {
        auto& registry = m_scene.GetRegistry();
        AddDefaultSystems(m_scene);                                        // 組み込みシステムを正しい順で全部
        AddCamera(m_scene, hlslpp::float3(0, 5, -10), hlslpp::float3(0, 1, 0));
        AddDirectionalLight(m_scene);                                      // 無いとモデルが真っ暗
        CreatePrimitive(m_scene, PrimitiveShape::Plane, hlslpp::float3(0, 0, 0), hlslpp::float3(20, 1, 20), PhysicsBody::Static);

        Tsukino::ECS::Entity ball = CreatePrimitive(m_scene, PrimitiveShape::Sphere, hlslpp::float3(0, 3, 0),
                                                    hlslpp::float3(1, 1, 1), PhysicsBody::Dynamic);
        auto sound = LoadAsset(m_scene, "Assets/Sounds/hit.wav");
        auto& script = registry.AddComponent<ScriptComponent>(ball);
        script.onStart = [](ScriptContext& ctx) { /* 最初の更新で1回 */ };
        script.onUpdate = [timer = 0.0f](ScriptContext& ctx, float dt) mutable { timer += dt; };  // 状態は保たれる
        script.onCollisionEnter = [sound](ScriptContext& ctx, Tsukino::ECS::Entity other) { PlayAudio(ctx.scene, sound); };
    }
};
```

- `m_scene`（`Tsukino::ECS::Scene`）は GameSceneBase のメンバ。最初のシーンは `EngineAPI::ChangeScene(std::make_unique<MyScene>())`。
- `ScriptContext` のメンバ：`scene` `registry` `engine`（EngineContext&）`entity`（自分）`time`（経過秒）、
  `Get<T>()`（自分のコンポーネント）`TryGet<T>()`（無ければ nullptr）`Destroy(e)` `DestroySelf()`。
- 他のエンティティを触るなら、その `Entity` をラムダに値で持たせる（`[ball](ScriptContext& ctx, float) {…}`）。
  消えているかもしれないので `ctx.registry.IsValid(ball)` で確かめてから使う。
- ラムダの中でエンティティを作る・コンポーネントを足す・破棄（予約）してよい。

## よく使う API（ヘッダは `Tsukino.EngineIntegration/include/Tsukino/EngineIntegration/Scene/SceneHelpers.hpp`）

| やること | 書き方 |
|---|---|
| 組み込みシステムを全部 | `AddDefaultSystems(scene)`（Script -1 → Physics 0 → Transform 1 → Camera 2 → Model 3 → Light 4 → Audio 5 → Lifetime 6） |
| 自作システムを足す | `scene.AddSystem(std::make_shared<MySystem>(), 優先度)`（`Tsukino::ECS::ISystem` の `Update(Registry&, float)`。小さいほど先） |
| 立方体・平面・球 | `CreatePrimitive(scene, PrimitiveShape::Cube/Plane/Sphere, 位置, 大きさ = 1, PhysicsBody::None/Static/Dynamic/Kinematic)` |
| 既存エンティティに見た目と物理 | `AttachPrimitive(scene, entity, shape, body)`（Prefab で作った物に。Transform の scale を使う） |
| 空のエンティティ | `scene.CreateEntity()`、`registry.AddComponent<T>(e)` |
| コンポーネント | `registry.GetComponent<T>(e)` / `HasComponent<T>(e)` / `try_get<T>(e)`（無ければ nullptr）/ `RemoveComponent<T>(e)` |
| 全部回す | `for(auto e : registry.View<A, B>()) { … }`（回している最中に A・B を足し外ししない。集めてから後で） |
| 破棄 | `scene.DestroyEntity(e)`（予約。全システムの更新の後に消える）。時間で消すなら `LifetimeComponent::remaining`（秒） |
| アセット | `LoadAsset(scene, "Assets/…")`（モデル・音・テクスチャ・シェーダー。失敗は無効なハンドル＋ログ） |
| 音 | `PlayAudio(scene, handle, 音量 = 1, ループ = false)` |
| エンジンの機能 | `GetEngineContext(scene)` または `ctx.engine`：`assetManager` `audioManager` `renderer` `inputSystem` `prefabFactory` `builtinAssets` |
| キー入力 | `ctx.engine.inputSystem->IsKeyDown(Tsukino::Input::KeyCode::Space)`（`IsKeyPressed` は押した瞬間。`Tsukino/Core/Input/InputSystem.hpp`） |
| 背景色 | `GetEngineContext(scene).renderer->SetClearColor(r, g, b, 1)` |
| ログ | `Tsukino::Core::Log::Info/Warn/Error(std::string)`。ファイルに出すのは `Log::SetLogFile("Logs/x.log")` を呼んでから |

## 実装を読まないと分からない挙動

- **Transform**：`TransformComponent` の `position` / `rotation`（クォータニオン）/ `scale` を書き換えたら `dirty = true`。立てないと反映されない。
- **座標系**は左手系（+X 右・+Y 上・+Z 奥）、1 = 1 m。組み込みモデルは原点中心・大きさ 1（Plane は XZ の 1×1、上向き）。
- **当たり判定の大きさ**：`CollisionComponent::extent` は**半分の大きさ**で、**scale を見ない**。`CreatePrimitive` / `AttachPrimitive` は
  scale から自動で入れる（Cube は scale/2、Sphere は scale の最大値が直径、Plane は厚さ 1 の箱で上面が position.y）。
  後で scale を変えたら extent も自分で直す。Sphere は `extent.x` が半径。`isSensor = true` は押し返さず重なりだけ検出。
- **Dynamic**：重力（-9.81）で動き、**位置は物理が毎フレーム上書き**する（position を書いても戻る）。
  動かすなら `registry.AddComponent<ImpulseRequestComponent>(e).impulse = …`（1回分の衝撃。処理後に外れる）か、
  `RigidbodyComponent::force`（毎フレーム加わり続ける）。`linearVelocity` は物理から読み戻した値で、書いても効かない。
  反発 `restitution`（既定 0）、摩擦 `friction`、`gravityFactor`、回転は既定で全軸固定（`freezeRotationX/Y/Z`）。
- **Kinematic**：自分で `position` を書き換えて（`dirty = true`）動かす。物理がそこへ付いてくる。
- **衝突イベント（onCollisionEnter / CollisionEnterEvent）が出る組み合わせ**：
  Dynamic×Static、Dynamic×Dynamic、Kinematic×Dynamic、Dynamic か Kinematic × センサー（`isSensor`）。
  **Kinematic×Static と Kinematic×Kinematic では出ない**（当てたい側をセンサーにする）。触れ始めの1回だけで、離れたときのイベントは無い。
  両方に ScriptComponent があれば両方の onCollisionEnter がそれぞれ1回呼ばれる（`other` が相手）。物理の更新中に同期で呼ばれる。
- **破棄は予約**：`DestroyEntity` / `ctx.Destroy` の後もそのフレームの間は `IsValid` が true のまま。数を数えるなら自分で管理する。
  システムの中で `Registry::DestroyEntity`（即時）を呼ばない。
- **アセットのパス**はアセットのルートからの相対（ゲームでは `Assets/…`）。区切りは `/`。

## 自作シェーダー（`CustomShaderComponent`）

1. `Assets/Shaders/<名前>.ps.sc` を置く（ゲームの `premake5.lua` の `tsukino_shaders("Assets/Shaders")` がビルド時に変換。足したらビルドし直すだけ）。
2. 中身の最小形：
   ```glsl
   $input v_worldPos, v_normal, v_texcoord0, v_curClip, v_prevClip
   #include <bgfx_shader.sh>
   uniform vec4 u_user0[4];
   void main() { gl_FragColor = u_user0[0]; }
   ```
3. ModelComponent を持つエンティティに `registry.AddComponent<CustomShaderComponent>(e).pixelShader = LoadAsset(scene, "Assets/Shaders/<名前>.ps.sc");`。
   `params[0〜3]`（float4）が `u_user0[0〜3]` に届く（毎フレーム書き換えてよい）。ライティングと影は掛からない。
- 色だけ変えたいならシェーダー不要：`MaterialPropertyBlockComponent::baseColor`。

## Prefab（JSON）

- グループ：`{"Entities":[{"key":"名前","value":"Assets/Prefabs/X.json"}, …]}` を
  `auto map = GetEngineContext(scene).prefabFactory->InstantiateGroup("…/Group.json", registry);`（`map["名前"]` が Entity）。1体だけなら `Instantiate(path, registry)`。
- Prefab：`{"Components":[{"key":"TransformComponent","value":"Assets/Prefabs/X_Transform.json"}, …]}`。
- コンポーネント：`{"TransformComponent":{"position":{"x":0,"y":1,"z":0},"rotation":{"x":0,"y":0,"z":0,"w":1},"scale":{"x":1,"y":1,"z":1}}}`。
  組み込みは型名がキー。フィールドは `Docs/components.md`。
- 自作コンポーネントは cereal の `load`（と `save`）を**同じ名前空間に**書き、`prefabFactory->RegisterComponent<T>("キー名")` で登録する。
  `Tsukino::ECS::EntityRef` のフィールドは JSON で `"#名前"` と書くと、InstantiateGroup の後で同じグループの Entity に解決される（Entity として使える）。
  例：`Tsukino.Sandbox/src/Recipes/PrefabRecipe.cpp` と `Tsukino.Sandbox/Assets/Prefabs/Recipe/`。

## その先を調べる順

1. `Tsukino.Sandbox/src/Recipes/RecipeScene.cpp`（物理・衝突音・追従・自作シェーダー・生成と破棄・センサーを1シーンで）
2. `Docs/api-index.md`（全公開型 → ヘッダの1行索引）→ `Docs/api/<Module>.md`（1本だけ）/ `Docs/components.md`
3. 該当するヘッダ1本。エンジンの .cpp は最後の手段
