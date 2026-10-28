# コンポーネント一覧

**このファイルは自動生成です。直接編集しないでください。**

再生成: `vendor\premake5.exe gen-manifest`（または `generate-docs.bat`）

コンポーネント 54 個（Prefab 登録済み 36 個 / シリアライズ定義あり 23 個）

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
| `color` | `hlslpp::float3` | { "x", "y", "z" } | `hlslpp::float3(1.0f, 1.0f, 1.0f)` |
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

### BloomComponent

- 型: `Tsukino::BuiltIn::ECS::BloomComponent`
- Prefab 登録名: `BloomComponent`
- 定義: `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/BloomComponent.hpp`
- シリアライズ: `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Serialization/BloomComponentSerialization.hpp`

| JSON キー | C++ 型 | JSON 表現 | 既定値 |
|---|---|---|---|
| `enabled` | `bool` | true / false | `true` |
| `threshold` | `float` | number | `1.0f` |
| `softKnee` | `float` | number | `0.5f` |
| `intensity` | `float` | number | `0.6f` |
| `radius` | `float` | number | `1.0f` |
| `tint` | `hlslpp::float3` | { "x", "y", "z" } | `hlslpp::float3(1.0f, 1.0f, 1.0f)` |

### BoneFollowColliderComponent

- 型: `Tsukino::BuiltIn::ECS::BoneFollowColliderComponent`
- Prefab 登録名: `BoneFollowColliderComponent`
- 定義: `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/BoneFollowColliderComponent.hpp`
- シリアライズ: `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Serialization/BoneFollowColliderComponentSerialization.hpp`

| JSON キー | C++ 型 | JSON 表現 | 既定値 |
|---|---|---|---|
| `boneName` | `std::string` | string | — |
| `followHeight` | `bool` | true / false | `false` |
| `followSpeed` | `float` | number | `12.0f` |

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
| `referenceResolution` | `hlslpp::float2` | { "x", "y" } | `hlslpp::float2(0.0f, 0.0f)` |

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
| `maxWidth` | `float` | number | `0.0f` |
| `fontHandle` | `Tsukino::Asset::AssetRef` | "path/to/asset" | — |
| `scale` | `float` | number | `1.0f` |

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
| `doubleSided` | `bool` | true / false | `false` |
| `materials` | `std::vector<Tsukino::Asset::AssetRef>` | ? | — |

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

### OcclusionCutoutComponent

- 型: `Tsukino::BuiltIn::ECS::OcclusionCutoutComponent`
- Prefab 登録名: `OcclusionCutoutComponent`
- 定義: `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/OcclusionCutoutComponent.hpp`
- シリアライズ: `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Serialization/OcclusionCutoutComponentSerialization.hpp`

| JSON キー | C++ 型 | JSON 表現 | 既定値 |
|---|---|---|---|
| `enabled` | `bool` | true / false | `true` |

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

### SphereColorGradeComponent

- 型: `Tsukino::BuiltIn::ECS::SphereColorGradeComponent`
- Prefab 登録名: `SphereColorGradeComponent`
- 定義: `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/SphereColorGradeComponent.hpp`
- シリアライズ: `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Serialization/SphereColorGradeComponentSerialization.hpp`

| JSON キー | C++ 型 | JSON 表現 | 既定値 |
|---|---|---|---|
| `enabled` | `bool` | true / false | `true` |
| `center` | `hlslpp::float3` | { "x", "y", "z" } | `hlslpp::float3(0.0f, 0.0f, 0.0f)` |
| `radius` | `float` | number | `0.0f` |
| `edgeSoftness` | `float` | number | `1.0f` |
| `tint` | `hlslpp::float3` | { "x", "y", "z" } | `hlslpp::float3(1.0f, 1.0f, 1.0f)` |
| `strength` | `float` | number | `1.0f` |
| `fogColor` | `hlslpp::float3` | { "x", "y", "z" } | `hlslpp::float3(0.0f, 0.0f, 0.0f)` |
| `fogDensity` | `float` | number | `0.0f` |
| `edgeColor` | `hlslpp::float3` | { "x", "y", "z" } | `hlslpp::float3(0.0f, 0.0f, 0.0f)` |
| `edgeWidth` | `float` | number | `1.0f` |

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
| `fillMode` | `SpriteFillMode` | number（enum の整数値） | `SpriteFillMode::None` |
| `fillAmount` | `float` | number | `1.0f` |
| `fillStartAngle` | `float` | number | `0.0f` |
| `fillClockwise` | `bool` | true / false | `true` |
| `fillReverse` | `bool` | true / false | `false` |
| `sliceBorder` | `hlslpp::float4` | { "x", "y", "z", "w" } | `hlslpp::float4(0.0f, 0.0f, 0.0f, 0.0f)` |
| `sliceScale` | `float` | number | `1.0f` |
| `raycastTarget` | `bool` | true / false | `true` |

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

### UIButtonSoundComponent

- 型: `Tsukino::BuiltIn::ECS::UIButtonSoundComponent`
- Prefab 登録名: `UIButtonSoundComponent`
- 定義: `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/UIButtonSoundComponent.hpp`
- シリアライズ: `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Serialization/UIButtonSoundComponentSerialization.hpp`

| JSON キー | C++ 型 | JSON 表現 | 既定値 |
|---|---|---|---|
| `hoverSound` | `Tsukino::Asset::AssetRef` | "path/to/asset" | — |
| `clickSound` | `Tsukino::Asset::AssetRef` | "path/to/asset" | — |
| `hoverVolume` | `float` | number | `1.0f` |
| `clickVolume` | `float` | number | `1.0f` |
| `bus` | `u8` | number | `0` |

### UIRectComponent

- 型: `Tsukino::BuiltIn::ECS::UIRectComponent`
- Prefab 登録名: `UIRectComponent`
- 定義: `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/UIRectComponent.hpp`
- シリアライズ: `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Serialization/UIRectComponentSerialization.hpp`

| JSON キー | C++ 型 | JSON 表現 | 既定値 |
|---|---|---|---|
| `size` | `hlslpp::float2` | { "x", "y" } | `hlslpp::float2(0.0f, 0.0f)` |
| `pivot` | `hlslpp::float2` | { "x", "y" } | `hlslpp::float2(0.5f, 0.5f)` |

## Prefab から生成できないコンポーネント

登録もしくはシリアライズ定義が無いため、コードから直接扱う。

| コンポーネント | 登録 | シリアライズ | 定義 |
|---|---|---|---|
| `AnimationControllerComponent` | — | — | `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/AnimationControllerComponent.hpp` |
| `AnimationPlayerComponent` | — | — | `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/AnimationPlayerComponent.hpp` |
| `AudioComponent` | — | — | `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/AudioComponent.hpp` |
| `BoxCollider2DComponent` | — | — | `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/BoxCollider2DComponent.hpp` |
| `BurstParticleComponent` | — | — | `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/BurstParticleComponent.hpp` |
| `CharacterControllerComponent` | — | — | `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/CharacterControllerComponent.hpp` |
| `DebugCameraTag` | `DebugCameraTag` | — | `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/DebugCameraTag.hpp` |
| `DraggableComponent` | `DraggableComponent` | — | `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/DraggableComponent.hpp` |
| `ImpulseRequestComponent` | — | — | `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/ImpulseRequestComponent.hpp` |
| `LifetimeComponent` | `LifetimeComponent` | — | `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/LifetimeComponent.hpp` |
| `MaterialPropertyBlockComponent` | — | — | `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/MaterialPropertyBlockComponent.hpp` |
| `MotionVectorComponent` | — | — | `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/MotionVectorComponent.hpp` |
| `NodeWorldMatrixComponent` | — | — | `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/NodeWorldMatrixComponent.hpp` |
| `NodeWorldPoseComponent` | — | — | `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/NodeWorldPoseComponent.hpp` |
| `PointerTargetComponent` | — | — | `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/PointerTargetComponent.hpp` |
| `RimGlowComponent` | — | — | `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/RimGlowComponent.hpp` |
| `RootMotionComponent` | — | — | `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/RootMotionComponent.hpp` |
| `ScreenModelComponent` | — | — | `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/ScreenModelComponent.hpp` |
| `ScrollBarComponent` | `ScrollBarComponent` | — | `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/ScrollBarComponent.hpp` |
| `ScrollViewComponent` | `ScrollViewComponent` | — | `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/ScrollViewComponent.hpp` |
| `SkeletonOutputComponent` | — | — | `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/SkeletonOutputComponent.hpp` |
| `SpringBoneComponent` | — | — | `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/SpringBoneComponent.hpp` |
| `UIButtonFeedbackComponent` | `UIButtonFeedbackComponent` | — | `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/UIButtonFeedbackComponent.hpp` |
| `UICanvasGroupComponent` | `UICanvasGroupComponent` | — | `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/UICanvasGroupComponent.hpp` |
| `UIClipComponent` | `UIClipComponent` | — | `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/UIClipComponent.hpp` |
| `UIMotionComponent` | `UIMotionComponent` | — | `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/UIMotionComponent.hpp` |
| `UISliderComponent` | `UISliderComponent` | — | `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/UISliderComponent.hpp` |
| `UIToggleComponent` | `UIToggleComponent` | — | `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/UIToggleComponent.hpp` |
| `UITweenComponent` | `UITweenComponent` | — | `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/UITweenComponent.hpp` |
| `UIVisibilityComponent` | `UIVisibilityComponent` | — | `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/UIVisibilityComponent.hpp` |
| `WorldAnchorComponent` | — | — | `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/WorldAnchorComponent.hpp` |

