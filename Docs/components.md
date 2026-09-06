# コンポーネント一覧

**このファイルは自動生成です。直接編集しないでください。**

再生成: `vendor\premake5.exe gen-manifest`（または `generate-docs.bat`）

コンポーネント 33 個（Prefab 登録済み 19 個 / シリアライズ定義あり 17 個）

## 読み方

- **JSON キー** … `cereal::make_nvp` の第1引数。Prefab JSON にはこの名前で書く
- **JSON 表現** … Prefab JSON 上での値の形。`?` は自動判定できなかったもので、
  その場合は定義ヘッダを直接確認する
- コンポーネント JSON のルートキーは **Prefab 登録名**そのもの
- `EntityRef` は `"#EntityName"`。名前はグループファイル（`*.Group.json`）の `key` を指す
- `AssetRef` は作業ディレクトリ相対のパス文字列
- **シリアライズ定義が無いコンポーネントは Prefab に書けない**。コードから `AddComponent` する
- **ここに載るのはエンジン組み込みのコンポーネントだけ**。
  ゲーム固有のコンポーネントはゲーム側リポジトリで定義・登録するため含まれない
- 登録済みだがシリアライズ定義が無いものは、名前だけ登録されていて Prefab からは生成できない

## Prefab で使えるコンポーネント

（登録済み かつ シリアライズ定義あり）

### AmbientParticleComponent

- 型: `Tsukino::BuiltIn::ECS::AmbientParticleComponent`
- Prefab 登録名: `AmbientParticleComponent`
- 定義: `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/AmbientParticleComponent.hpp`
- シリアライズ: `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Serialization/AmbientParticleComponentSerialization.hpp`

| JSON キー | C++ 型 | JSON 表現 | 既定値 |
|---|---|---|---|
| `enabled` | `bool` | true / false | `true` |
| `count` | `unsigned int` | number | `3000` |
| `volumeSize` | `hlslpp::float3` | { "x", "y", "z" } | `hlslpp::float3(2400.0f, 900.0f, 2400.0f)` |
| `seed` | `unsigned int` | number | `12345` |
| `color` | `hlslpp::float3` | { "x", "y", "z" } | `hlslpp::float3(1.0f, 0.32f, 0.06f)` |
| `intensity` | `float` | number | `1.0f` |
| `minSize` | `float` | number | `0.8f` |
| `maxSize` | `float` | number | `3.5f` |
| `minBrightness` | `float` | number | `0.15f` |
| `maxBrightness` | `float` | number | `1.6f` |
| `twinkle` | `float` | number | `0.6f` |
| `driftVelocity` | `hlslpp::float3` | { "x", "y", "z" } | `hlslpp::float3(10.0f, 20.0f, 3.0f)` |
| `swayAmplitude` | `float` | number | `14.0f` |
| `swayFrequency` | `float` | number | `0.8f` |
| `minSpeedScale` | `float` | number | `0.35f` |
| `maxSpeedScale` | `float` | number | `1.7f` |
| `edgeFadeStart` | `float` | number | `0.65f` |
| `nearFadeDistance` | `float` | number | `60.0f` |

### CameraComponent

- 型: `Tsukino::BuiltIn::ECS::CameraComponent`
- Prefab 登録名: `CameraComponent`
- 定義: `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/CameraComponent.hpp`
- シリアライズ: `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Serialization/CameraComponentSerialization.hpp`

| JSON キー | C++ 型 | JSON 表現 | 既定値 |
|---|---|---|---|
| `projectionType` | `ProjectionType` | number（enum の整数値） | `ProjectionType::Perspective` |
| `orthoSize` | `float` | number | `720.0f` |
| `fov` | `float` | number | `45.0f` |
| `aspectRatio` | `float` | number | `16.0f / 9.0f` |
| `nearZ` | `float` | number | `0.1f` |
| `farZ` | `float` | number | `1000.0f` |
| `useLookAt` | `bool` | true / false | `false` |
| `lookAtTarget` | `hlslpp::float3` | { "x", "y", "z" } | `hlslpp::float3(0, 0, 0)` |
| `isPrimary` | `bool` | true / false | `true` |

### CollisionComponent

- 型: `Tsukino::BuiltIn::ECS::CollisionComponent`
- Prefab 登録名: `CollisionComponent`
- 定義: `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/CollisionComponent.hpp`
- シリアライズ: `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Serialization/CollisionComponentSerialization.hpp`

| JSON キー | C++ 型 | JSON 表現 | 既定値 |
|---|---|---|---|
| `type` | `ColliderType` | number（enum の整数値） | `ColliderType::Box` |
| `extent` | `hlslpp::float3` | { "x", "y", "z" } | `{0.5f, 0.5f, 0.5f}` |
| `offsetPosition` | `hlslpp::float3` | { "x", "y", "z" } | `{0.0f, 0.0f, 0.0f}` |
| `offsetRotation` | `hlslpp::quaternion` | { "x", "y", "z", "w" } | `hlslpp::quaternion::identity()` |
| `isSensor` | `bool` | true / false | `false` |

### DebugCameraComponent

- 型: `Tsukino::BuiltIn::ECS::DebugCameraComponent`
- Prefab 登録名: `DebugCameraComponent`
- 定義: `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/DebugCameraComponent.hpp`
- シリアライズ: `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Serialization/DebugCameraComponentSerialization.hpp`

| JSON キー | C++ 型 | JSON 表現 | 既定値 |
|---|---|---|---|
| `moveSpeed` | `float` | number | `10.0f` |
| `sprintSpeed` | `float` | number | `30.0f` |
| `mouseSens` | `float` | number | `0.15f` |

### DirectionalLightComponent

- 型: `Tsukino::BuiltIn::ECS::DirectionalLightComponent`
- Prefab 登録名: `DirectionalLightComponent`
- 定義: `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/DirectionalLightComponent.hpp`
- シリアライズ: `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Serialization/DirectionalLightComponentSerialization.hpp`

| JSON キー | C++ 型 | JSON 表現 | 既定値 |
|---|---|---|---|
| `direction` | `hlslpp::float3` | { "x", "y", "z" } | `{0.0f, -1.0f, 0.0f}` |
| `color` | `hlslpp::float3` | { "x", "y", "z" } | `{1.0f, 1.0f, 1.0f}` |
| `intensity` | `float` | number | `1.0f` |
| `castShadow` | `bool` | true / false | `true` |

### EffectComponent

- 型: `Tsukino::BuiltIn::ECS::EffectComponent`
- Prefab 登録名: `EffectComponent`
- 定義: `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/EffectComponent.hpp`
- シリアライズ: `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Serialization/EffectComponentSerialization.hpp`

| JSON キー | C++ 型 | JSON 表現 | 既定値 |
|---|---|---|---|
| `effectAsset` | `Tsukino::Asset::AssetRef` | "path/to/asset" | — |
| `playSpeed` | `float` | number | `1.0f` |
| `looping` | `bool` | true / false | `false` |

### FogComponent

- 型: `Tsukino::BuiltIn::ECS::FogComponent`
- Prefab 登録名: `FogComponent`
- 定義: `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/FogComponent.hpp`
- シリアライズ: `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Serialization/FogComponentSerialization.hpp`

| JSON キー | C++ 型 | JSON 表現 | 既定値 |
|---|---|---|---|
| `enabled` | `bool` | true / false | `true` |
| `color` | `hlslpp::float3` | { "x", "y", "z" } | `hlslpp::float3(0.55f, 0.60f, 0.65f)` |
| `density` | `float` | number | `0.015f` |
| `startDistance` | `float` | number | `10.0f` |
| `maxOpacity` | `float` | number | `1.0f` |
| `heightFogEnabled` | `bool` | true / false | `true` |
| `height` | `float` | number | `2.0f` |
| `heightFalloff` | `float` | number | `0.25f` |
| `heightDensity` | `float` | number | `0.05f` |
| `sunColor` | `hlslpp::float3` | { "x", "y", "z" } | `hlslpp::float3(1.0f, 0.85f, 0.65f)` |
| `sunScatterPower` | `float` | number | `8.0f` |
| `noiseEnabled` | `bool` | true / false | `true` |
| `noiseScale` | `float` | number | `0.03f` |
| `noiseIntensity` | `float` | number | `0.4f` |
| `windDirection` | `hlslpp::float3` | { "x", "y", "z" } | `hlslpp::float3(1.0f, 0.0f, 0.3f)` |
| `windSpeed` | `float` | number | `0.5f` |

### FontComponent

- 型: `Tsukino::BuiltIn::ECS::FontComponent`
- Prefab 登録名: `FontComponent`
- 定義: `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/FontComponent.hpp`
- シリアライズ: `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Serialization/FontComponentSerialization.hpp`

| JSON キー | C++ 型 | JSON 表現 | 既定値 |
|---|---|---|---|
| `color` | `hlslpp::float4` | { "x", "y", "z", "w" } | `{1, 1, 1, 1}` |
| `origin` | `hlslpp::float2` | { "x", "y" } | `{0, 0}` |
| `horizontalAlign` | `HorizontalAlign` | number（enum の整数値） | `HorizontalAlign::Left` |
| `verticalAlign` | `VerticalAlign` | number（enum の整数値） | `VerticalAlign::Top` |
| `outlineColor` | `hlslpp::float4` | { "x", "y", "z", "w" } | `{0, 0, 0, 1}` |
| `outlineWidth` | `float` | number | `0.0f` |
| `sortOrder` | `int` | number | `0` |

### ModelComponent

- 型: `Tsukino::BuiltIn::ECS::ModelComponent`
- Prefab 登録名: `ModelComponent`
- 定義: `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/ModelComponent.hpp`
- シリアライズ: `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Serialization/ModelComponentSerialization.hpp`

| JSON キー | C++ 型 | JSON 表現 | 既定値 |
|---|---|---|---|
| `modelHandle` | `Tsukino::Asset::AssetRef` | "path/to/asset" | — |
| `visible` | `bool` | true / false | `true` |
| `opacity` | `float` | number | `1.0f` |

### MotionBlurComponent

- 型: `Tsukino::BuiltIn::ECS::MotionBlurComponent`
- Prefab 登録名: `MotionBlurComponent`
- 定義: `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/MotionBlurComponent.hpp`
- シリアライズ: `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Serialization/MotionBlurComponentSerialization.hpp`

| JSON キー | C++ 型 | JSON 表現 | 既定値 |
|---|---|---|---|
| `enabled` | `bool` | true / false | `true` |
| `strength` | `float` | number | `1.0f` |
| `maxBlurRadius` | `float` | number | `0.03f` |
| `sampleCount` | `int` | number | `8` |
| `targetFps` | `float` | number | `60.0f` |

### PointLightComponent

- 型: `Tsukino::BuiltIn::ECS::PointLightComponent`
- Prefab 登録名: `PointLightComponent`
- 定義: `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/PointLightComponent.hpp`
- シリアライズ: `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Serialization/PointLightComponentSerialization.hpp`

| JSON キー | C++ 型 | JSON 表現 | 既定値 |
|---|---|---|---|
| `color` | `hlslpp::float3` | { "x", "y", "z" } | `{1.0f, 1.0f, 1.0f}` |
| `intensity` | `float` | number | `1.0f` |
| `range` | `float` | number | `10.0f` |
| `enabled` | `bool` | true / false | `true` |

### RigidbodyComponent

- 型: `Tsukino::BuiltIn::ECS::RigidbodyComponent`
- Prefab 登録名: `RigidbodyComponent`
- 定義: `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/RigidbodyComponent.hpp`
- シリアライズ: `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Serialization/RigidbodyComponentSerialization.hpp`

| JSON キー | C++ 型 | JSON 表現 | 既定値 |
|---|---|---|---|
| `type` | `RigidbodyType` | number（enum の整数値） | `RigidbodyType::Dynamic` |
| `mass` | `float` | number | `1.0f` |
| `friction` | `float` | number | `0.5f` |
| `restitution` | `float` | number | `0.0f` |
| `gravityFactor` | `float` | number | `1.0f` |
| `linearVelocity` | `hlslpp::float3` | { "x", "y", "z" } | `{0, 0, 0}` |
| `angularVelocity` | `hlslpp::float3` | { "x", "y", "z" } | `{0, 0, 0}` |
| `freezePositionX` | `bool` | true / false | `false` |
| `freezePositionY` | `bool` | true / false | `false` |
| `freezePositionZ` | `bool` | true / false | `false` |
| `freezeRotationX` | `bool` | true / false | `true` |
| `freezeRotationY` | `bool` | true / false | `true` |
| `freezeRotationZ` | `bool` | true / false | `true` |

### SkyAtmosphereComponent

- 型: `Tsukino::BuiltIn::ECS::SkyAtmosphereComponent`
- Prefab 登録名: `SkyAtmosphereComponent`
- 定義: `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/SkyAtmosphereComponent.hpp`
- シリアライズ: `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Serialization/SkyAtmosphereComponentSerialization.hpp`

| JSON キー | C++ 型 | JSON 表現 | 既定値 |
|---|---|---|---|
| `rayleighScattering` | `float` | number | `1.0f` |
| `mieScattering` | `float` | number | `0.1f` |
| `mieAnisotropy` | `float` | number | `0.76f` |
| `atmosphereHeight` | `float` | number | `8000.0f` |
| `planetRadius` | `float` | number | `6371000.0f` |
| `sunIntensity` | `float` | number | `20.0f` |
| `sunDiskSize` | `float` | number | `0.02f` |
| `groundColor` | `hlslpp::float3` | { "x", "y", "z" } | `hlslpp::float3(0.1f, 0.08f, 0.05f)` |

### SpotLightComponent

- 型: `Tsukino::BuiltIn::ECS::SpotLightComponent`
- Prefab 登録名: `SpotLightComponent`
- 定義: `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/SpotLightComponent.hpp`
- シリアライズ: `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Serialization/SpotLightComponentSerialization.hpp`

| JSON キー | C++ 型 | JSON 表現 | 既定値 |
|---|---|---|---|
| `color` | `hlslpp::float3` | { "x", "y", "z" } | `{1.0f, 1.0f, 1.0f}` |
| `intensity` | `float` | number | `1.0f` |
| `range` | `float` | number | `10.0f` |
| `innerConeDeg` | `float` | number | `20.0f` |
| `outerConeDeg` | `float` | number | `30.0f` |
| `enabled` | `bool` | true / false | `true` |

### SpriteComponent

- 型: `Tsukino::BuiltIn::ECS::SpriteComponent`
- Prefab 登録名: `SpriteComponent`
- 定義: `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/SpriteComponent.hpp`
- シリアライズ: `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Serialization/SpriteComponentSerialization.hpp`

| JSON キー | C++ 型 | JSON 表現 | 既定値 |
|---|---|---|---|
| `textureHandle` | `Tsukino::Asset::AssetRef` | "path/to/asset" | — |
| `blendMode` | `SpriteBlendMode` | number（enum の整数値） | `SpriteBlendMode::Alpha` |
| `space` | `SpriteSpace` | number（enum の整数値） | `SpriteSpace::Screen` |
| `tintColor` | `hlslpp::float4` | { "x", "y", "z", "w" } | `hlslpp::float4(1.0f, 1.0f, 1.0f, 1.0f)` |
| `uvRect` | `hlslpp::float4` | { "x", "y", "z", "w" } | `hlslpp::float4(0.0f, 0.0f, 1.0f, 1.0f)` |
| `sortOrder` | `int` | number | `0` |

### TerrainGenerationRequestComponent

- 型: `Tsukino::BuiltIn::ECS::TerrainGenerationRequestComponent`
- Prefab 登録名: `TerrainGenerationRequestComponent`
- 定義: `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/TerrainGenerationRequestComponent.hpp`
- シリアライズ: `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Serialization/TerrainGenerationRequestComponentSerialization.hpp`

| JSON キー | C++ 型 | JSON 表現 | 既定値 |
|---|---|---|---|
| `amplitude` | `float` | number | `10.0f` |
| `noiseFrequency` | `float` | number | `0.05f` |
| `seed` | `uint32_t` | ? | `0` |
| `noiseType` | `TerrainNoiseType` | number（enum の整数値） | `TerrainNoiseType::Noise` |
| `collisionModelHandle` | `Tsukino::Asset::AssetRef` | "path/to/asset" | — |

### TransformComponent

- 型: `Tsukino::BuiltIn::ECS::TransformComponent`
- Prefab 登録名: `TransformComponent`
- 定義: `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp`
- シリアライズ: `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Serialization/TransformComponentSerialization.hpp`

| JSON キー | C++ 型 | JSON 表現 | 既定値 |
|---|---|---|---|
| `position` | `hlslpp::float3` | { "x", "y", "z" } | `hlslpp::float3(0.0f, 0.0f, 0.0f)` |
| `rotation` | `hlslpp::quaternion` | { "x", "y", "z", "w" } | `hlslpp::quaternion(0.0f, 0.0f, 0.0f, 1.0f)` |
| `scale` | `hlslpp::float3` | { "x", "y", "z" } | `hlslpp::float3(1.0f, 1.0f, 1.0f)` |
| `parent` | `Tsukino::ECS::EntityRef` | "#EntityName" | — |

## Prefab から生成できないコンポーネント

登録もしくはシリアライズ定義が無いため、コードから直接扱う。

| コンポーネント | 登録 | シリアライズ | 定義 |
|---|---|---|---|
| `AnimationControllerComponent` | — | — | `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/AnimationControllerComponent.hpp` |
| `AnimationPlayerComponent` | — | — | `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/AnimationPlayerComponent.hpp` |
| `AudioComponent` | — | — | `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/AudioComponent.hpp` |
| `BoxCollider2DComponent` | — | — | `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/BoxCollider2DComponent.hpp` |
| `CharacterControllerComponent` | — | — | `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/CharacterControllerComponent.hpp` |
| `DebugCameraTag` | `DebugCameraTag` | — | `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/DebugCameraTag.hpp` |
| `DraggableComponent` | `DraggableComponent` | — | `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/DraggableComponent.hpp` |
| `HighlightComponent` | — | — | `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/HighlightComponent.hpp` |
| `ImpulseRequestComponent` | — | — | `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/ImpulseRequestComponent.hpp` |
| `MotionVectorComponent` | — | — | `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/MotionVectorComponent.hpp` |
| `NodeWorldMatrixComponent` | — | — | `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/NodeWorldMatrixComponent.hpp` |
| `NodeWorldPoseComponent` | — | — | `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/NodeWorldPoseComponent.hpp` |
| `RootMotionComponent` | — | — | `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/RootMotionComponent.hpp` |
| `SkeletonOutputComponent` | — | — | `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/SkeletonOutputComponent.hpp` |
| `SpringBoneComponent` | — | — | `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/SpringBoneComponent.hpp` |
| `WorldAnchorComponent` | — | — | `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/WorldAnchorComponent.hpp` |

