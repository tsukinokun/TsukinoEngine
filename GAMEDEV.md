# TsukinoEngine でゲームを作る（ゲーム作者向けガイド）

ゲームのコードに要ることはここに全部ある。**エンジンのヘッダ・.cpp・レシピを読む前に、これだけで書く。**
足りないときだけ末尾の「その先を調べる」へ。`External/` 配下（Jolt・entt・bgfx など）は読まない。

## シーンの書き方（自作の System クラスやヘッダは要らない）

シーンは `GameSceneBase` の派生（`OnInitialize(EngineAPI&)` で作り、`OnUpdate(EngineAPI&, float dt)` で `m_scene.Update(dt);`）。
エンティティの振る舞いは `ScriptComponent` のラムダで書く。

```cpp
#include <Tsukino/EngineIntegration/Scene/SceneHelpers.hpp>
#include <Tsukino/EngineIntegration/ECS/Script/ScriptComponent.hpp>
#include <Tsukino/EngineIntegration/EngineContext.hpp>
#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>   // 組み込みは Tsukino/BuiltIn/ECS/Component/<型名>.hpp
#include <Tsukino/Core/Log.hpp>
using namespace Tsukino::EngineIntegration;   // SceneHelpers の関数と列挙型
using namespace Tsukino::BuiltIn::ECS;        // 組み込みコンポーネント, ScriptComponent, ScriptContext

void MyScene::OnInitialize(Tsukino::EngineIntegration::EngineAPI&) {
    auto& registry = m_scene.GetRegistry();
    AddDefaultSystems(m_scene);                       // 組み込みシステムを正しい順で全部（Script・Audio・Lifetime も動く）
    AddCamera(m_scene, hlslpp::float3(0, 5, -10), hlslpp::float3(0, 1, 0));
    AddDirectionalLight(m_scene);                     // 無いとモデルが真っ暗
    CreatePrimitive(m_scene, PrimitiveShape::Plane, hlslpp::float3(0, 0, 0), hlslpp::float3(20, 1, 20), PhysicsBody::Static);

    Tsukino::ECS::Entity ball = CreatePrimitive(m_scene, PrimitiveShape::Sphere, hlslpp::float3(0, 3, 0), hlslpp::float3(1, 1, 1), PhysicsBody::Dynamic);
    auto sound   = LoadAsset(m_scene, "Assets/Sounds/hit.wav");
    auto& script = registry.AddComponent<ScriptComponent>(ball);
    script.onStart          = [](ScriptContext& ctx) { /* 最初の更新で1回 */ };
    script.onUpdate         = [timer = 0.0f](ScriptContext& ctx, float dt) mutable { timer += dt; };   // ラムダの状態は保たれる
    script.onCollisionEnter = [sound](ScriptContext& ctx, Tsukino::ECS::Entity other) { PlayAudio(ctx.scene, sound); };
}
```

- `ScriptContext`：`scene` `registry` `engine`（EngineContext&）`entity`（自分）`time`（経過秒）`Get<T>()` `TryGet<T>()`（無ければ nullptr）`Destroy(e)` `DestroySelf()`。
- 他のエンティティは `Entity` をラムダに値で持たせ（`[ball](…)`）、使う前に `ctx.registry.IsValid(ball)`。
- ラムダの中でエンティティを作る・コンポーネントを足す・破棄（予約）してよい。
- 結果を外から読みたい値（回数など）はシーンのクラスのメンバに置き、ラムダには `this` を持たせる。
- ヘッダで `Entity` だけ使うなら `<Tsukino/Core/ECS/Entity/Entity.hpp>`。ソースは BOM なし UTF-8 でよい（`/utf-8` 指定済み）。

## 関数（`Tsukino/EngineIntegration/Scene/SceneHelpers.hpp`、namespace `Tsukino::EngineIntegration`）

```cpp
void           AddDefaultSystems(Scene&);   // Script -1 → Physics 0 → Transform 1 → Camera 2 → Model 3 → Light 4 → Audio 5 → Lifetime 6
Entity         AddCamera(Scene&, const float3& eye, const float3& target);
Entity         AddDirectionalLight(Scene&, const float3& direction = {0.4,-1,0.6}, float intensity = 3);
Entity         CreatePrimitive(Scene&, PrimitiveShape /*Cube|Plane|Sphere*/, const float3& position, const float3& scale = {1,1,1},
                               PhysicsBody = PhysicsBody::None /*None|Static|Dynamic|Kinematic*/);   // 見た目＋大きさに合った当たり判定＋剛体
void           AttachPrimitive(Scene&, Entity, PrimitiveShape, PhysicsBody = None);   // 既存（Prefab で作った物など）に。Transform の scale を使う
EngineContext& GetEngineContext(Scene&);   // assetManager audioManager renderer inputSystem prefabFactory builtinAssets（ラムダでは ctx.engine）
AssetHandle    LoadAsset(Scene&, const std::string& path);   // "Assets/…"。失敗は無効（handle.IsValid() が false）＋ログ
bool           PlayAudio(Scene&, AssetHandle sound, float volume = 1, bool loop = false);   // 鳴らせたら true
```

その他：`scene.CreateEntity()` / `scene.DestroyEntity(e)`（予約）/ `registry.AddComponent<T>(e)` `GetComponent<T>(e)` `HasComponent<T>(e)`
`try_get<T>(e)` `RemoveComponent<T>(e)` `IsValid(e)` / `for(auto e : registry.View<A, B>())`（回している間に A・B を足し外ししない）/
自作システム `scene.AddSystem(std::make_shared<S>(), 優先度)`（`Tsukino::ECS::ISystem::Update(Registry&, float)`）/
キー `ctx.engine.inputSystem->IsKeyDown(Tsukino::Input::KeyCode::Space)`（`IsKeyPressed` は押した瞬間）/
背景色 `GetEngineContext(scene).renderer->SetClearColor(r, g, b, 1)` / ログ `Tsukino::Core::Log::Info/Warn/Error(std::string)`。

## 主なコンポーネント（namespace `Tsukino::BuiltIn::ECS`）

| 型 | 主な項目 |
|---|---|
| `TransformComponent` | `float3 position` `quaternion rotation` `float3 scale` `EntityRef parent` `bool dirty` `worldMatrix`（読み取り） |
| `ModelComponent` | `AssetRef modelHandle` `bool visible` `float opacity` |
| `CollisionComponent` | `ColliderType type`（`Box`/`Sphere`/`Capsule`）`float3 extent`（半分の大きさ）`float3 offsetPosition` `bool isSensor` |
| `RigidbodyComponent` | `RigidbodyType type`（`Static`/`Dynamic`/`Kinematic`）`mass` `friction` `restitution` `gravityFactor` `float3 force` `freezeRotationX/Y/Z` `isGrounded`（読み取り） |
| `ImpulseRequestComponent` | `float3 impulse` `float3 angularImpulse`（付けると次の物理更新で1回加わって外れる） |
| `LifetimeComponent` | `float remaining`（秒。0 で破棄） |
| `CustomShaderComponent` | `AssetRef pixelShader` `AssetRef vertexShader` `std::array<float4, 4> params`（`AssetRef` には `LoadAsset` の戻り値を入れる） |
| `MaterialPropertyBlockComponent` | `std::optional<float4> baseColor` `std::optional<float3> emissive` `metallic` `roughness`（色だけ変えるならこれ。シェーダー不要） |

## 実装を読まないと分からない挙動

- **Transform** の `position` / `rotation` / `scale` を書き換えたら `dirty = true`（立てないと反映されない）。左手系（+X 右・+Y 上・+Z 奥）、1 = 1 m。
- 組み込みモデルは原点中心・大きさ 1（Plane は XZ の 1×1、上向き）。大きさは scale で変える。
- **当たり判定** `extent` は半分の大きさで scale を見ない。CreatePrimitive は scale から入れる（Cube は scale/2、Sphere は scale の最大値が直径、
  Plane は厚さ 1 の箱で上面が position.y）。後で scale を変えたら extent も直す。`isSensor` は押し返さず重なりだけ検出。
- **Dynamic** は重力（-9.81）で動き、位置は物理が毎フレーム上書きする（position を書いても戻る）。動かすなら ImpulseRequestComponent か `force`。
  `linearVelocity` は読み戻しの値で、書いても効かない。反発 `restitution` は既定 0、回転は既定で全軸固定。
- **Kinematic** は自分で position を書き換えて（`dirty = true`）動かす。物理がそこへ付いてくる。
- **衝突イベント**（onCollisionEnter）が出る組み合わせ：Dynamic×Static、Dynamic×Dynamic、Kinematic×Dynamic、Dynamic か Kinematic × センサー。
  **Kinematic×Static と Kinematic×Kinematic では出ない**（当てたい側をセンサーにする）。触れ始めの1回だけで、離れたときは無い。
  両方に ScriptComponent があれば両方がそれぞれ1回呼ばれる（`other` が相手）。物理の更新中に同期で呼ばれる。
- **破棄は予約**：DestroyEntity の後もそのフレームの間は `IsValid` が true。数を数えるなら自分で管理する。システムの中で `Registry::DestroyEntity`（即時）を呼ばない。

## 自作シェーダー

1. `Assets/Shaders/<名前>.ps.sc` を置き、ゲームの `premake5.lua` の `project "Game"` の直下（filter の外。`tsukino_link()` の次の行など）に
   `tsukino_shaders("Assets/Shaders")` を1行書く。ビルド時に変換され、文法エラーはビルドのエラーに出る。
2. 最小形（入力の行は組み込みの頂点シェーダーに合わせてこのまま）：
   ```glsl
   $input v_worldPos, v_normal, v_texcoord0, v_curClip, v_prevClip
   #include <bgfx_shader.sh>
   uniform vec4 u_user0[4];
   void main() { gl_FragColor = u_user0[0]; }
   ```
3. ModelComponent を持つエンティティに `registry.AddComponent<CustomShaderComponent>(e).pixelShader = LoadAsset(scene, "Assets/Shaders/<名前>.ps.sc");`。
   `params[0〜3]` が `u_user0[0〜3]` に届く（毎フレーム書き換えてよい。時間で変えるなら onUpdate で `ctx.time` から計算）。ライティングと影は掛からない。

## Prefab（JSON）— 参照つきの2体を作る完全な例

ファイル（パスはリポジトリ直下から。`key` が名前・型名、`value` がファイル）：

```text
Assets/Prefabs/Pair/Group.json      {"Entities":[{"key":"Center","value":"Assets/Prefabs/Pair/Center.json"},
                                                 {"key":"Mover","value":"Assets/Prefabs/Pair/Mover.json"}]}
Assets/Prefabs/Pair/Center.json     {"Components":[{"key":"TransformComponent","value":"Assets/Prefabs/Pair/Center_Transform.json"}]}
Assets/Prefabs/Pair/Mover.json      {"Components":[{"key":"TransformComponent","value":"Assets/Prefabs/Pair/Mover_Transform.json"},
                                                   {"key":"OrbitComponent","value":"Assets/Prefabs/Pair/Mover_Orbit.json"}]}
Assets/Prefabs/Pair/Center_Transform.json  {"TransformComponent":{"position":{"x":0,"y":1,"z":0},"rotation":{"x":0,"y":0,"z":0,"w":1},"scale":{"x":1,"y":1,"z":1}}}
Assets/Prefabs/Pair/Mover_Transform.json   （同じ形。position だけ変える）
Assets/Prefabs/Pair/Mover_Orbit.json       {"OrbitComponent":{"center":"#Center","radius":1.5}}
```

コード：

```cpp
#include <Tsukino/Core/ECS/EntityRef/EntityRef.hpp>
#include <cereal/cereal.hpp>
struct OrbitComponent { Tsukino::ECS::EntityRef center; float radius = 1.0f; };
template <class Archive> void load(Archive& archive, OrbitComponent& c) {   // 型と同じ名前空間に置く。読む項目は JSON に全部書く
    archive(cereal::make_nvp("center", c.center), cereal::make_nvp("radius", c.radius));
}
// OnInitialize の中で
auto& factory = *GetEngineContext(m_scene).prefabFactory;
factory.RegisterComponent<OrbitComponent>("OrbitComponent");   // InstantiateGroup より前に。組み込みは型名で登録済み
auto entities = factory.InstantiateGroup("Assets/Prefabs/Pair/Group.json", registry);   // 名前 → Entity
AttachPrimitive(m_scene, entities["Center"], PrimitiveShape::Cube);   // 見た目は JSON に書かずコードで付けてよい
// entities["Mover"] の OrbitComponent::center は Center に解決済み
```

- `EntityRef` は `"#名前"` で同じグループの Entity に解決される（`load` を書いた型だけ）。`Entity` へ暗黙に変換でき（`IsValid(c.center)` などにそのまま渡せる）、コードからは `c.center = entity;`。
- 1体だけなら `factory.Instantiate("…/X.json", registry)`。組み込みコンポーネントの JSON の項目は `Docs/components.md`。

## その先を調べる（ここに無いときだけ）

1. `Docs/api-index.md`（全公開型 → ヘッダの1行索引）→ 該当するヘッダ1本 / `Docs/components.md`
2. 動くお手本：`Tsukino.Sandbox/src/Recipes/RecipeScene.cpp`・`PrefabRecipe.cpp`。エンジンの .cpp は最後の手段
