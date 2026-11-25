# Making a game with TsukinoEngine

Everything needed to write game code is here. **Write from this file alone; do not open engine headers, .cpp files or recipes first.**
Only if something is missing, see "Looking further" at the end. Never read `External/` (Jolt, entt, bgfx, ...).

## Writing a scene (no custom System classes or extra headers needed)

A scene derives from `GameSceneBase`: build it in `OnInitialize(EngineAPI&)`, and `OnUpdate(EngineAPI&, float dt)` calls `m_scene.Update(dt);`.
Entity behaviour goes in `ScriptComponent` lambdas.

```cpp
#include <Tsukino/EngineIntegration/Scene/SceneHelpers.hpp>
#include <Tsukino/EngineIntegration/ECS/Script/ScriptComponent.hpp>
#include <Tsukino/EngineIntegration/EngineContext.hpp>
#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>   // built-ins: Tsukino/BuiltIn/ECS/Component/<Type>.hpp
#include <Tsukino/Core/Log.hpp>
using namespace Tsukino::EngineIntegration;   // SceneHelpers functions and enums
using namespace Tsukino::BuiltIn::ECS;        // built-in components, ScriptComponent, ScriptContext

void MyScene::OnInitialize(Tsukino::EngineIntegration::EngineAPI&) {
    auto& registry = m_scene.GetRegistry();
    AddDefaultSystems(m_scene);                       // all built-in systems in the right order (Script, Audio, Lifetime included)
    AddCamera(m_scene, hlslpp::float3(0, 5, -10), hlslpp::float3(0, 1, 0));
    AddDirectionalLight(m_scene);                     // without it models render black
    CreatePrimitive(m_scene, PrimitiveShape::Plane, hlslpp::float3(0, 0, 0), hlslpp::float3(20, 1, 20), PhysicsBody::Static);

    Tsukino::ECS::Entity ball = CreatePrimitive(m_scene, PrimitiveShape::Sphere, hlslpp::float3(0, 3, 0), hlslpp::float3(1, 1, 1), PhysicsBody::Dynamic);
    auto sound   = LoadAsset(m_scene, "Assets/Sounds/hit.wav");
    auto& script = registry.AddComponent<ScriptComponent>(ball);
    script.onStart          = [](ScriptContext& ctx) { /* once, on the first update */ };
    script.onUpdate         = [timer = 0.0f](ScriptContext& ctx, float dt) mutable { timer += dt; };   // lambda state persists
    script.onCollisionEnter = [sound](ScriptContext& ctx, Tsukino::ECS::Entity other) { PlayAudio(ctx.scene, sound); };
}
```

- `ScriptContext`: `scene` `registry` `engine` (EngineContext&) `entity` (self) `time` (seconds) `Get<T>()` `TryGet<T>()` (nullptr if absent) `Destroy(e)` `DestroySelf()`.
- To use another entity, capture its `Entity` by value (`[ball](…)`) and check `ctx.registry.IsValid(ball)` first.
- Lambdas may create entities, add components and destroy (deferred).
- Values to read from outside (counts etc.): make them scene members and capture `this`.
- A header that only needs `Entity`: `<Tsukino/Core/ECS/Entity/Entity.hpp>`. Sources are plain UTF-8 (no BOM needed; `/utf-8` is set).

## Functions (`Tsukino/EngineIntegration/Scene/SceneHelpers.hpp`, namespace `Tsukino::EngineIntegration`)

Namespaces: `Scene` `Entity` `Registry` `EntityRef` are `Tsukino::ECS`; `AssetHandle` `AssetRef` are `Tsukino::Asset`; `float3` `float4` `quaternion` are `hlslpp`.
Read hlslpp components with `float(v.x)`; build values with `hlslpp::float4(r, g, b, a)` (no `hlslpp::store` needed).

```cpp
void           AddDefaultSystems(Scene&);   // Script -1, Physics 0, Transform 1, Camera 2, Model 3, Light 4, Audio 5, Lifetime 6
Entity         AddCamera(Scene&, const float3& eye, const float3& target);
Entity         AddDirectionalLight(Scene&, const float3& direction = {0.4,-1,0.6}, float intensity = 3);
Entity         CreatePrimitive(Scene&, PrimitiveShape /*Cube|Plane|Sphere*/, const float3& position, const float3& scale = {1,1,1},
                               PhysicsBody = PhysicsBody::None /*None|Static|Dynamic|Kinematic*/);   // model + collider sized to scale + rigidbody
void           AttachPrimitive(Scene&, Entity, PrimitiveShape, PhysicsBody = None);   // for existing entities (e.g. from a Prefab); uses Transform scale
EngineContext& GetEngineContext(Scene&);   // assetManager audioManager renderer inputSystem prefabFactory builtinAssets (in lambdas: ctx.engine)
AssetHandle    LoadAsset(Scene&, const std::string& path);   // "Assets/…"; on failure handle.IsValid() is false and the log says why
bool           PlayAudio(Scene&, AssetHandle sound, float volume = 1, bool loop = false);   // true if played
```

Also: `scene.CreateEntity()` / `scene.DestroyEntity(e)` (deferred) / `registry.AddComponent<T>(e)` `GetComponent<T>(e)` `HasComponent<T>(e)`
`try_get<T>(e)` `RemoveComponent<T>(e)` `IsValid(e)` / `for(auto e : registry.View<A, B>())` (do not add or remove A/B while iterating) /
custom system `scene.AddSystem(std::make_shared<S>(), priority)` (`Tsukino::ECS::ISystem::Update(Registry&, float)`) /
keys `ctx.engine.inputSystem->IsKeyDown(Tsukino::Input::KeyCode::Space)` (`IsKeyPressed` = this frame only) /
background `GetEngineContext(scene).renderer->SetClearColor(r, g, b, 1)` / log `Tsukino::Core::Log::Info/Warn/Error(std::string)`.

## Main components (namespace `Tsukino::BuiltIn::ECS`)

| Type | Main fields |
|---|---|
| `TransformComponent` | `float3 position` `quaternion rotation` `float3 scale` `EntityRef parent` `bool dirty` `worldMatrix` (read-only) |
| `ModelComponent` | `AssetRef modelHandle` `bool visible` `float opacity` |
| `CollisionComponent` | `ColliderType type` (`Box`/`Sphere`/`Capsule`) `float3 extent` (half size) `float3 offsetPosition` `bool isSensor` |
| `RigidbodyComponent` | `RigidbodyType type` (`Static`/`Dynamic`/`Kinematic`) `mass` `friction` `restitution` `gravityFactor` `float3 force` `freezeRotationX/Y/Z` `isGrounded` (read-only) |
| `ImpulseRequestComponent` | `float3 impulse` `float3 angularImpulse` (applied once on the next physics update, then removed) |
| `LifetimeComponent` | `float remaining` (seconds; destroyed at 0) |
| `CustomShaderComponent` | `AssetRef pixelShader` `AssetRef vertexShader` `std::array<float4, 4> params` (assign `LoadAsset` results to `AssetRef`) |
| `MaterialPropertyBlockComponent` | `std::optional<float4> baseColor` `std::optional<float3> emissive` `metallic` `roughness` (just a colour change: no shader needed) |

## Behaviour you cannot guess

- After changing `position` / `rotation` / `scale`, set `dirty = true` or nothing updates. Left-handed (+X right, +Y up, +Z forward), 1 unit = 1 m.
- Built-in models are centred, size 1 (Plane is 1×1 on XZ, facing up). Resize with scale.
- Collider `extent` is half size and ignores scale. CreatePrimitive derives it from scale (Cube: scale/2; Sphere: max scale is the diameter;
  Plane: a 1-thick box whose top is at position.y). Fix extent yourself if you change scale later. `isSensor` detects overlap without pushing.
- **Dynamic** bodies fall (gravity -9.81) and physics overwrites their position every frame. Move them with ImpulseRequestComponent or `force`.
  `linearVelocity` is read back from physics; writing it does nothing. `restitution` defaults to 0; rotation is frozen on all axes by default.
- **Kinematic** bodies: write `position` yourself (and `dirty = true`); physics follows.
- **Collision events** (onCollisionEnter) fire for Dynamic×Static, Dynamic×Dynamic, Kinematic×Dynamic, and Dynamic or Kinematic × sensor.
  **Not for Kinematic×Static or Kinematic×Kinematic** (make the target a sensor). Once when contact starts; there is no exit event.
  If both have a ScriptComponent, each gets one call (`other` is the other entity). Called synchronously during the physics update.
- **Destroy is deferred**: `IsValid` stays true for the rest of the frame. Keep your own count. Never call `Registry::DestroyEntity` (immediate) in a system.

## Custom shader

1. Put `Assets/Shaders/<name>.ps.sc` in place and add one line `tsukino_shaders("Assets/Shaders")` directly under `project "Game"` in the game's
   `premake5.lua` (outside any filter, e.g. after `tsukino_link()`). Shaders compile during the build; syntax errors show as build errors.
2. Minimal shader (keep the `$input` line as is; it matches the built-in vertex shader):
   ```glsl
   $input v_worldPos, v_normal, v_texcoord0, v_curClip, v_prevClip
   #include <bgfx_shader.sh>
   uniform vec4 u_user0[4];
   void main() { gl_FragColor = u_user0[0]; }
   ```
3. On an entity with a ModelComponent: `registry.AddComponent<CustomShaderComponent>(e).pixelShader = LoadAsset(scene, "Assets/Shaders/<name>.ps.sc");`.
   `params[0..3]` reach `u_user0[0..3]` (update every frame if needed, e.g. from `ctx.time` in onUpdate). No lighting or shadows.

## Prefab (JSON): one file with a reference, complete example

`Assets/Prefabs/Pair.json` (keys are entity names, then component type names; `"#Name"` refers to another entity in the same file):

```json
{ "Entities": {
    "Center": { "Components": {
        "TransformComponent": { "position": {"x":0,"y":1,"z":0}, "rotation": {"x":0,"y":0,"z":0,"w":1}, "scale": {"x":1,"y":1,"z":1} } } },
    "Mover": { "Components": {
        "TransformComponent": { "position": {"x":2,"y":1,"z":0}, "rotation": {"x":0,"y":0,"z":0,"w":1}, "scale": {"x":1,"y":1,"z":1} },
        "OrbitComponent": { "center": "#Center", "radius": 1.5 } } } } }
```

```cpp
#include <Tsukino/Engine/ECS/Prefab/PrefabFactory.hpp>   // needed to use prefabFactory (EngineContext only forward-declares it)
#include <Tsukino/Core/ECS/EntityRef/EntityRef.hpp>
#include <cereal/cereal.hpp>
struct OrbitComponent { Tsukino::ECS::EntityRef center; float radius = 1.0f; };
template <class Archive> void load(Archive& archive, OrbitComponent& c) {   // same namespace as the type; every field read here must be in the JSON
    archive(cereal::make_nvp("center", c.center), cereal::make_nvp("radius", c.radius));
}
// in OnInitialize
auto& factory = *GetEngineContext(m_scene).prefabFactory;
factory.RegisterComponent<OrbitComponent>("OrbitComponent");   // before InstantiateGroup; built-ins are registered by type name
auto entities = factory.InstantiateGroup("Assets/Prefabs/Pair.json", registry);   // name -> Entity
AttachPrimitive(m_scene, entities["Center"], PrimitiveShape::Cube);   // visuals can be added in code instead of JSON
// entities["Mover"]'s OrbitComponent::center now refers to Center
```

- `EntityRef` resolves `"#Name"` only for types with a `load`. It converts to `Entity` (pass it to `IsValid` etc.); set it in code with `c.center = entity;`.
- `{}` attaches a component with defaults. A string value instead of an object is read as a file path. A single prefab: `factory.Instantiate(path, registry)`
  with `{"Components": {…}}`. Missing fields are logged as errors. Built-in component JSON fields: `Docs/components.md`.

## Looking further (only if not covered above)

1. `Docs/api-index.md` (every public type → header) → that one header / `Docs/components.md`
2. Working samples: `Tsukino.Sandbox/src/Recipes/RecipeScene.cpp`, `PrefabRecipe.cpp`. Engine .cpp files last.
