# Tsukino.BuiltIn の公開 API

**このファイルは自動生成です。直接編集しないでください。**

型の在処だけ知りたいときは `../api-index.md` を見る。

## 全公開型の索引

メンバ名のみ。詳細が要るときはヘッダを開く。

- **Tsukino::BuiltIn::BuiltInAssets** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/BuiltInAssets.hpp`
  - shaders, fonts, models, Initialize()
- **Tsukino::BuiltIn::BuiltInFonts** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/BuiltInFonts.hpp`
  - defaultFont, Initialize()
- **Tsukino::BuiltIn::BuiltInModels** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/BuiltInModels.hpp`
  - cube, plane, sphere, Initialize()
- **Tsukino::BuiltIn::BuiltInShaders** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/BuiltInShaders.hpp`
  - spriteVS, spritePS, spriteWorldVS, modelVS, modelPS, screenModelPS, staticModelVS, debugVS, debugPS, shadowVS, shadowStaticVS, shadowPS, skyVS, skyPS, tonemapVS, tonemapPS, gbufferPS, lightingPS, motionBlurPS, bloomPrefilterPS, bloomDownsamplePS, bloomUpsamplePS, fogPS, sphereColorGradePS, ambientParticleVS, ambientParticlePS, iblIrradiancePS, iblSpecularPrefilterPS, iblBRDFLUTPS, Initialize()
- **Tsukino::BuiltIn::ECS::AmbientParticleComponent** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/AmbientParticleComponent.hpp`
  - enabled, count, volumeSize, seed, color, intensity, minSize, maxSize, minBrightness, maxBrightness, twinkle, driftVelocity, swayAmplitude, swayFrequency, minSpeedScale, maxSpeedScale, edgeFadeStart, nearFadeDistance
- **Tsukino::BuiltIn::ECS::AnimationControllerComponent** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/AnimationControllerComponent.hpp`
  - blendAlpha, isTransitioning, next, outgoing
- **Tsukino::BuiltIn::ECS::AnimationControllerComponent::NextAnimation** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/AnimationControllerComponent.hpp`
  - clip, animationIndex, fadeTime, immediate, isLooping, clipStartTime, clipEndTime, inPlace
- **Tsukino::BuiltIn::ECS::AnimationControllerComponent::OutgoingAnimation** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/AnimationControllerComponent.hpp`
  - clip, animationIndex, elapsedTime, isLooping, clipStartTime, clipEndTime, inPlace
- **Tsukino::BuiltIn::ECS::AnimationPlayerComponent** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/AnimationPlayerComponent.hpp`
  - currentClipId, animationIndex, elapsedTime, playbackSpeed, isLooping, isPlaying, isFinished, clipStartTime, clipEndTime, inPlace, rootMotionNodeName, rootMotionNodeIndex, rootMotionResolved, rootMotionLockActive, rootMotionLockX, rootMotionLockZ
- **Tsukino::BuiltIn::ECS::AudioComponent** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/AudioComponent.hpp`
  - audioHandle, playOnAwake, loop, volume, pitch, pan, bus, isPlaying, playTrigger, stopTrigger
- **Tsukino::BuiltIn::ECS::BloomComponent** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/BloomComponent.hpp`
  - enabled, threshold, softKnee, intensity, radius, tint
- **Tsukino::BuiltIn::ECS::BoneFollowColliderComponent** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/BoneFollowColliderComponent.hpp`
  - boneName, followHeight, followSpeed, baseOffset, restBonePosition, currentShift, hasRest, boneNodeIndex, resolvedAgainstModel
- **Tsukino::BuiltIn::ECS::BoxCollider2DComponent** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/BoxCollider2DComponent.hpp`
  - offset, size
- **Tsukino::BuiltIn::ECS::BurstParams** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Particle/BurstEmitter.hpp`
  - count, speedMin, speedMax, upwardBias, spread, lifeMin, lifeMax, sizeMin, sizeMax, endSizeScale, gravity, drag, radius, delayMax, colorA, colorB, endAlpha, additive, texturePixels
- **Tsukino::BuiltIn::ECS::BurstParticleComponent** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/BurstParticleComponent.hpp`
  - velocity, gravity, drag, age, life, startSize, endSize, startColor, endColor, texturePixels
- **Tsukino::BuiltIn::ECS::CameraComponent** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/CameraComponent.hpp`
  - projectionType, orthoSize, referenceResolution, fov, aspectRatio, nearZ, farZ, useLookAt, lookAtTarget, viewMatrix, projectionMatrix, viewProjMatrix, invViewProjMatrix, isPrimary, dirty
- **Tsukino::BuiltIn::ECS::CameraUtility::Ray** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Camera/CameraUtility.hpp`
  - origin, direction, GetPoint()
- **Tsukino::BuiltIn::ECS::CharacterControllerComponent** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/CharacterControllerComponent.hpp`
  - radius, halfHeight, maxSlopeDeg, mass, gravityFactor, centerOffset, isInitialized, moveInput, jumpRequested, jumpSpeed, verticalVelocity, isGrounded
- **Tsukino::BuiltIn::ECS::CollisionComponent** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/CollisionComponent.hpp`
  - bodyID, type, extent, offsetPosition, offsetRotation, heightfieldSamples, heightfieldSize, heightfieldOffset, heightfieldScale, isSensor, isInitialized, onCollisionEnter, IsValid()
- **Tsukino::BuiltIn::ECS::CollisionEnterEvent** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Event/CollisionEnterEvent.hpp`
  - self, other, normal
- **Tsukino::BuiltIn::ECS::CustomShaderComponent** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/CustomShaderComponent.hpp`
  - pixelShader, vertexShader, params
- **Tsukino::BuiltIn::ECS::DebugCameraComponent** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/DebugCameraComponent.hpp`
  - moveSpeed, sprintSpeed, mouseSens, yaw, pitch, isActive
- **Tsukino::BuiltIn::ECS::DebugCameraTag** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/DebugCameraTag.hpp`
  - dummy
- **Tsukino::BuiltIn::ECS::DirectionalLightComponent** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/DirectionalLightComponent.hpp`
  - direction, color, intensity, castShadow, shadow
- **Tsukino::BuiltIn::ECS::DraggableComponent** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/DraggableComponent.hpp`
  - isDragging, dragOffset
- **Tsukino::BuiltIn::ECS::EffectComponent** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/EffectComponent.hpp`
  - effectAsset, effectPath, handle, playSpeed, looping, stopped, active, scale, followRotation
- **Tsukino::BuiltIn::ECS::FogComponent** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/FogComponent.hpp`
  - enabled, color, density, startDistance, maxOpacity, heightFogEnabled, height, heightFalloff, heightDensity, sunColor, sunScatterPower, noiseEnabled, noiseScale, noiseIntensity, windDirection, windSpeed, useCustomDistanceOrigin, distanceOrigin
- **Tsukino::BuiltIn::ECS::FontComponent** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/FontComponent.hpp`
  - text, fontHandle, color, origin, horizontalAlign, verticalAlign, outlineColor, outlineWidth, maxWidth, scale, sortOrder
- **Tsukino::BuiltIn::ECS::ImpulseRequestComponent** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/ImpulseRequestComponent.hpp`
  - impulse, angularImpulse
- **Tsukino::BuiltIn::ECS::LifetimeComponent** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/LifetimeComponent.hpp`
  - remaining
- **Tsukino::BuiltIn::ECS::MaterialPropertyBlockComponent** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/MaterialPropertyBlockComponent.hpp`
  - baseColor, emissive, metallic, roughness, shadingModel, toonThreshold, toonSmoothness, toonShadeColor, toonSpecularSize
- **Tsukino::BuiltIn::ECS::ModelComponent** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/ModelComponent.hpp`
  - modelHandle, visible, opacity, doubleSided, materials
- **Tsukino::BuiltIn::ECS::MotionBlurComponent** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/MotionBlurComponent.hpp`
  - enabled, strength, maxBlurRadius, sampleCount, targetFps
- **Tsukino::BuiltIn::ECS::MotionVectorComponent** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/MotionVectorComponent.hpp`
  - kMaxBones, prevWorld, prevBones, prevBoneCount, valid
- **Tsukino::BuiltIn::ECS::NodeWorldMatrixComponent** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/NodeWorldMatrixComponent.hpp`
  - matrices
- **Tsukino::BuiltIn::ECS::NodeWorldPoseComponent** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/NodeWorldPoseComponent.hpp`
  - poses
- **Tsukino::BuiltIn::ECS::OcclusionCutoutComponent** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/OcclusionCutoutComponent.hpp`
  - enabled, ditherFade
- **Tsukino::BuiltIn::ECS::PointLightComponent** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/PointLightComponent.hpp`
  - color, intensity, range, enabled
- **Tsukino::BuiltIn::ECS::PointerTargetComponent** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/PointerTargetComponent.hpp`
  - hovered, clicked, pressed, hoverEntered, hoverExited
- **Tsukino::BuiltIn::ECS::RigidbodyComponent** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/RigidbodyComponent.hpp`
  - type, isTypeDirty, mass, friction, restitution, gravityFactor, linearVelocity, angularVelocity, force, torque, isInitialized, isGrounded, groundCheckDistance, groundCheckRadius, freezePositionX, freezePositionY, freezePositionZ, freezeRotationX, freezeRotationY, freezeRotationZ, isFreezeDirty
- **Tsukino::BuiltIn::ECS::RimGlowComponent** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/RimGlowComponent.hpp`
  - active, rimColor, rimIntensity, rimPower, glow
- **Tsukino::BuiltIn::ECS::RootMotionComponent** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/RootMotionComponent.hpp`
  - deltaPosition, deltaRotation
- **Tsukino::BuiltIn::ECS::ScreenModelComponent** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/ScreenModelComponent.hpp`
  - screenPosition, pixelsPerUnit, sortOrder, anchor
- **Tsukino::BuiltIn::ECS::ScrollBarComponent** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/ScrollBarComponent.hpp`
  - thumb, size, minThumbLength
- **Tsukino::BuiltIn::ECS::ScrollViewComponent** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/ScrollViewComponent.hpp`
  - content, scrollBar, contentHeight, enabled, wheelStep, keyStep, followSpeed, dragThreshold, offset, targetOffset, pointerDown, dragScrolling, pressPointerY, pressOffset, thumbDragging, thumbGrabOffset, ScrollTo(), ScrollToTop()
- **Tsukino::BuiltIn::ECS::SkeletonOutputComponent** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/SkeletonOutputComponent.hpp`
  - kMaxBones, localMatrices, boneCount
- **Tsukino::BuiltIn::ECS::SkyAtmosphereComponent** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/SkyAtmosphereComponent.hpp`
  - rayleighScattering, mieScattering, mieAnisotropy, atmosphereHeight, planetRadius, sunIntensity, sunDiskSize, groundColor
- **Tsukino::BuiltIn::ECS::SphereColorGradeComponent** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/SphereColorGradeComponent.hpp`
  - enabled, center, radius, edgeSoftness, tint, strength, fogColor, fogDensity, edgeColor, edgeWidth
- **Tsukino::BuiltIn::ECS::SpotLightComponent** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/SpotLightComponent.hpp`
  - color, intensity, range, innerConeDeg, outerConeDeg, enabled
- **Tsukino::BuiltIn::ECS::SpringBoneComponent** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/SpringBoneComponent.hpp`
  - chainDefs, chains, resolved, enabled
- **Tsukino::BuiltIn::ECS::SpringBoneComponent::ChainDef** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/SpringBoneComponent.hpp`
  - name, anchorNodeName, rootNodeName, excludeNodeNames, maxDepth, settings, colliders
- **Tsukino::BuiltIn::ECS::SpringBoneComponent::ColliderDef** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/SpringBoneComponent.hpp`
  - attachNodeName, localOffset, radius
- **Tsukino::BuiltIn::ECS::SpriteComponent** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/SpriteComponent.hpp`
  - textureHandle, blendMode, space, tintColor, uvRect, sortOrder, fillMode, fillAmount, fillStartAngle, fillClockwise, fillReverse, sliceBorder, sliceScale, raycastTarget
- **Tsukino::BuiltIn::ECS::TerrainGenerationRequestComponent** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/TerrainGenerationRequestComponent.hpp`
  - amplitude, noiseFrequency, seed, noiseType, collisionModelHandle
- **Tsukino::BuiltIn::ECS::TransformComponent** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp`
  - position, rotation, scale, localMatrix, worldMatrix, parent, dirty
- **Tsukino::BuiltIn::ECS::UIButtonFeedbackComponent** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/UIButtonFeedbackComponent.hpp`
  - hoverScale, pressScale, clickPunch, punchSeconds, speed, interactable, currentScale, punchElapsed
- **Tsukino::BuiltIn::ECS::UIButtonSoundComponent** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/UIButtonSoundComponent.hpp`
  - hoverSound, clickSound, hoverVolume, clickVolume, bus
- **Tsukino::BuiltIn::ECS::UICanvas** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/UI/UICanvas.hpp`
  - scale, offset, size, ToPixel(), ToUI(), ToPixel(), Fit()
- **Tsukino::BuiltIn::ECS::UICanvasGroupComponent** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/UICanvasGroupComponent.hpp`
  - alpha
- **Tsukino::BuiltIn::ECS::UIClipComponent** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/UIClipComponent.hpp`
  - size
- **Tsukino::BuiltIn::ECS::UIClipUtility::ClipBounds** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/UI/UIClipUtility.hpp`
  - left, top, right, bottom, Contains(), Overlaps(), Height()
- **Tsukino::BuiltIn::ECS::UIMotionComponent** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/UIMotionComponent.hpp`
  - offset, scale, rotation, pivot, pivotIsSelf, source
- **Tsukino::BuiltIn::ECS::UIRectComponent** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/UIRectComponent.hpp`
  - size, pivot
- **Tsukino::BuiltIn::ECS::UIRectUtility::LocalRect** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/UI/UIRectUtility.hpp`
  - center, size
- **Tsukino::BuiltIn::ECS::UISliderComponent** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/UISliderComponent.hpp`
  - track, trackLength, minValue, maxValue, changeThreshold, interactable, value, dragging, dragStarted, dragEnded, changed
- **Tsukino::BuiltIn::ECS::UIToggleComponent** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/UIToggleComponent.hpp`
  - group, interactable, isOn, changed
- **Tsukino::BuiltIn::ECS::UIToggleGroupComponent** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/UIToggleComponent.hpp`
  - changed
- **Tsukino::BuiltIn::ECS::UITween** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/UITweenComponent.hpp`
  - property, from, to, duration, delay, repeatDelay, ease, loop, holdAtEnd, unscaledTime, tag, elapsed
- **Tsukino::BuiltIn::ECS::UITweenComponent** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/UITweenComponent.hpp`
  - tweens
- **Tsukino::BuiltIn::ECS::UIVisibilityComponent** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/UIVisibilityComponent.hpp`
  - visible
- **Tsukino::BuiltIn::ECS::UnscaledTimeContext** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/UI/UnscaledTimeContext.hpp`
  - deltaTime
- **Tsukino::BuiltIn::ECS::WorldAnchorComponent** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/WorldAnchorComponent.hpp`
  - target, useFixedWorldPosition, fixedWorldPosition, worldOffset, screenOffset, visible

## 列挙型

- **Tsukino::BuiltIn::ECS::CameraComponent::ProjectionType** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/CameraComponent.hpp` — カメラの投影方法を指定 (Perspective: 3D用、Orthographic: 2D用)
  - `Perspective`
  - `Orthographic`
- **Tsukino::BuiltIn::ECS::ColliderType** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/CollisionComponent.hpp`
  - `Box`
  - `Sphere`
  - `Capsule`
  - `Heightfield`
- **Tsukino::BuiltIn::ECS::HorizontalAlign** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/FontComponent.hpp` — テキストの水平方向の基準位置
  - `Left = 0`
  - `Center`
  - `Right`
- **Tsukino::BuiltIn::ECS::RigidbodyType** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/RigidbodyComponent.hpp` — 剛体の種類を定義する列挙型
  - `Static`
  - `Kinematic`
  - `Dynamic`
- **Tsukino::BuiltIn::ECS::SpriteBlendMode** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/SpriteComponent.hpp` — スプライトの合成方法
  - `Alpha = 0`
  - `Additive`
- **Tsukino::BuiltIn::ECS::SpriteFillMode** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/SpriteComponent.hpp` — スプライトの一部だけを塗る方法
  - `None = 0`
  - `Radial`
  - `Horizontal`
  - `Vertical`
- **Tsukino::BuiltIn::ECS::SpriteSpace** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/SpriteComponent.hpp` — スプライトをどの空間の座標として扱うか
  - `Screen = 0`
  - `World`
- **Tsukino::BuiltIn::ECS::TerrainNoiseType** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/TerrainGenerationRequestComponent.hpp`
  - `Flat`
  - `Noise`
- **Tsukino::BuiltIn::ECS::UITweenLoop** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/UITweenComponent.hpp` — トゥイーンの繰り返し方です。
  - `Once`
  - `Loop`
  - `PingPong`
- **Tsukino::BuiltIn::ECS::UITweenProperty** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/UITweenComponent.hpp` — トゥイーンで動かす値です。
  - `OffsetX`
  - `OffsetY`
  - `Scale`
  - `ScaleX`
  - `ScaleY`
  - `Rotation`
  - `Alpha`
- **Tsukino::BuiltIn::ECS::VerticalAlign** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/FontComponent.hpp` — テキストの垂直方向の基準位置
  - `Top = 0`
  - `Middle`
  - `Bottom`

## 名前空間の関数

### Tsukino::BuiltIn::ECS::BurstEmitter — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Particle/BurstEmitter.hpp`

| シグネチャ | 説明 |
|---|---|
| `void Emit(Tsukino::ECS::Scene &scene, Tsukino::Asset::AssetHandle texture, const BurstParams &params, const hlslpp::float3 &position, std::mt19937 &rng)` |  |

### Tsukino::BuiltIn::ECS::CameraUtility — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Camera/CameraUtility.hpp`

| シグネチャ | 説明 |
|---|---|
| `bool IntersectPlane(const Ray &ray, const hlslpp::float3 &planePoint, const hlslpp::float3 &planeNormal, float &distance)` |  |
| `bool IntersectRect(const Ray &ray, const Tsukino::Core::Math::matrix &worldMatrix, const hlslpp::float3 &axisU, const hlslpp::float3 &axisV, const hlslpp::float2 &size, float &distance, hlslpp::float2 &uv)` |  |
| `Ray ScreenPointToRay(const CameraComponent &camera, const hlslpp::float2 &screenPoint, const hlslpp::float2 &screenSize)` |  |
| `bool WorldToScreenPoint(const CameraComponent &camera, const hlslpp::float3 &worldPoint, const hlslpp::float2 &screenSize, hlslpp::float2 &screenPoint)` |  |

### Tsukino::BuiltIn::ECS::TransformUtility — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Transform/TransformUtility.hpp`

| シグネチャ | 説明 |
|---|---|
| `void DestroyHierarchy(Tsukino::ECS::Registry &registry, Tsukino::ECS::Entity root)` |  |
| `template <class T> Tsukino::ECS::Entity FindNearestWith(Tsukino::ECS::Registry &registry, Tsukino::ECS::Entity entity)` | 指定エンティティから親を遡り、Tを持つ最も近いエンティティ（自身を含む）を返す |
| `Tsukino::Core::Math::matrix GetParentWorldMatrix(Tsukino::ECS::Registry &registry, const TransformComponent &transform)` | 有効な親のワールド行列を取得する |
| `hlslpp::float3 GetWorldPosition(const TransformComponent &transform) noexcept` | ワールド座標を取り出す |
| `hlslpp::float2 GetWorldScale2D(const TransformComponent &transform) noexcept` | ワールド行列に掛かっているXYスケールを取り出す |
| `bool IsAncestorOf(Tsukino::ECS::Registry &registry, Tsukino::ECS::Entity candidateAncestor, Tsukino::ECS::Entity entity)` | candidateAncestor が entity の祖先（自分自身を含む）かどうか |
| `bool SetParent(Tsukino::ECS::Registry &registry, Tsukino::ECS::Entity child, Tsukino::ECS::Entity parent, bool keepWorldPosition=true)` | 親を設定する |
| `void SetWorldPosition(Tsukino::ECS::Registry &registry, Tsukino::ECS::Entity entity, const hlslpp::float3 &worldPosition)` | ワールド座標を指定する（親がいればローカル座標へ変換して格納） |

### Tsukino::BuiltIn::ECS::UICanvasGroupUtility — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/UI/UICanvasGroupUtility.hpp`

| シグネチャ | 説明 |
|---|---|
| `float GetAlpha(Tsukino::ECS::Registry &registry, Tsukino::ECS::Entity entity)` |  |
| `bool IsInvisible(Tsukino::ECS::Registry &registry, Tsukino::ECS::Entity entity)` |  |

### Tsukino::BuiltIn::ECS::UIClipUtility — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/UI/UIClipUtility.hpp`

| シグネチャ | 説明 |
|---|---|
| `ClipBounds ComputeBounds(const TransformComponent &transform, const UIClipComponent &clip, const UIRectComponent *rect=nullptr)` | UIClipComponent を持つエンティティの枠の範囲を求める |
| `UIRectUtility::LocalRect ResolveLocalRect(const UIClipComponent &clip, const UIRectComponent *rect) noexcept` | 枠のローカルの四角を求める |
| `bool TryGetClipBounds(Tsukino::ECS::Registry &registry, Tsukino::ECS::Entity entity, ClipBounds &outBounds)` | エンティティを切り取る枠（自身または一番近い祖先の UIClipComponent）を探す |

### Tsukino::BuiltIn::ECS::UIMotionUtility — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/UI/UIMotionUtility.hpp`

| シグネチャ | 説明 |
|---|---|
| `bool IsIdentity(const UIMotionComponent &motion)` |  |
| `Tsukino::Core::Math::matrix MakeMatrix(const UIMotionComponent &motion, const TransformComponent &transform)` |  |
| `hlslpp::float2 ResolvePivot(const UIMotionComponent &motion, const TransformComponent &transform)` |  |

### Tsukino::BuiltIn::ECS::UIRectUtility — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/UI/UIRectUtility.hpp`

| シグネチャ | 説明 |
|---|---|
| `bool Contains(const hlslpp::float4x4 &worldMatrix, const LocalRect &local, const hlslpp::float2 &point)` | 画面上の点が四角の中にあるか（拡大・回転を考慮する） |
| `LocalRect FromRect(const UIRectComponent &rect) noexcept` | UIRectComponent の四角をローカルの四角にする |
| `hlslpp::float2 LocalToWorld(const hlslpp::float4x4 &worldMatrix, const hlslpp::float2 &point)` | ローカルの点を画面上の点にする（行ベクトル規約。Z は 0 として扱う） |
| `LocalRect Resolve(Tsukino::ECS::Registry &registry, Tsukino::ECS::Entity entity, const hlslpp::float2 &fallbackSize)` | エンティティのローカルの四角を求める |
| `hlslpp::float4x4 ToMatrix(const LocalRect &local, const hlslpp::float4x4 &worldMatrix)` | 中心が原点の単位の四角を、画面上の四角へ写す行列 |
| `void WorldBounds(const hlslpp::float4x4 &worldMatrix, const LocalRect &local, hlslpp::float2 &outMin, hlslpp::float2 &outMax)` | 四角を画面に写したものを囲む、軸に沿った範囲を求める |

### Tsukino::BuiltIn::ECS::UIToggleUtility — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/UI/UIToggleUtility.hpp`

| シグネチャ | 説明 |
|---|---|
| `Tsukino::ECS::Entity GetSelected(Tsukino::ECS::Registry &registry, Tsukino::ECS::Entity group)` |  |
| `void Select(Tsukino::ECS::Registry &registry, Tsukino::ECS::Entity entity)` |  |

### Tsukino::BuiltIn::ECS::UITweenUtility — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/UI/UITweenUtility.hpp`

| シグネチャ | 説明 |
|---|---|
| `void FadeIn(Tsukino::ECS::Registry &registry, Tsukino::ECS::Entity entity, float delay=0.0f, float seconds=0.3f)` |  |
| `void FadeOut(Tsukino::ECS::Registry &registry, Tsukino::ECS::Entity entity, float delay=0.0f, float seconds=0.3f)` |  |
| `bool IsPlaying(Tsukino::ECS::Registry &registry, Tsukino::ECS::Entity entity, std::uint32_t tag)` |  |
| `void Play(Tsukino::ECS::Registry &registry, Tsukino::ECS::Entity entity, const UITween &tween)` |  |
| `void Punch(Tsukino::ECS::Registry &registry, Tsukino::ECS::Entity entity, float amount=0.25f, float seconds=0.45f)` |  |
| `void SlideIn(Tsukino::ECS::Registry &registry, Tsukino::ECS::Entity entity, const hlslpp::float2 &from, float delay=0.0f, float seconds=0.45f, Tsukino::Core::Math::Easing::EaseType ease=Tsukino::Core::Math::Easing::EaseType::OutBack)` |  |
| `void SlideOut(Tsukino::ECS::Registry &registry, Tsukino::ECS::Entity entity, const hlslpp::float2 &to, float delay=0.0f, float seconds=0.35f, Tsukino::Core::Math::Easing::EaseType ease=Tsukino::Core::Math::Easing::EaseType::InCubic)` |  |
| `void Stop(Tsukino::ECS::Registry &registry, Tsukino::ECS::Entity entity, std::uint32_t tag)` |  |

### Tsukino::BuiltIn::ECS::UIVisibilityUtility — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/UI/UIVisibilityUtility.hpp`

| シグネチャ | 説明 |
|---|---|
| `bool IsHidden(Tsukino::ECS::Registry &registry, Tsukino::ECS::Entity entity)` | エンティティが隠れているか（自身か祖先の UIVisibilityComponent::visible が false）を返す |

### Tsukino::BuiltIn::ECS — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Serialization/AmbientParticleComponentSerialization.hpp`

| シグネチャ | 説明 |
|---|---|
| `template <class Archive> void load(Archive &archive, AmbientParticleComponent &particle)` | AmbientParticleComponentのcerealデシリアライズ定義 |
| `template <class Archive> void save(Archive &archive, const AmbientParticleComponent &particle)` | AmbientParticleComponentのcerealシリアライズ定義 |

### Tsukino::BuiltIn::ECS — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Serialization/BloomComponentSerialization.hpp`

| シグネチャ | 説明 |
|---|---|
| `template <class Archive> void load(Archive &archive, BloomComponent &bloom)` | BloomComponentのcerealデシリアライズ定義 |
| `template <class Archive> void save(Archive &archive, const BloomComponent &bloom)` | BloomComponentのcerealシリアライズ定義 |

### Tsukino::BuiltIn::ECS — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Serialization/BoneFollowColliderComponentSerialization.hpp`

| シグネチャ | 説明 |
|---|---|
| `template <class Archive> void load(Archive &archive, BoneFollowColliderComponent &follow)` | BoneFollowColliderComponentのcerealデシリアライズ定義 |
| `template <class Archive> void save(Archive &archive, const BoneFollowColliderComponent &follow)` | BoneFollowColliderComponentのcerealシリアライズ定義 |

### Tsukino::BuiltIn::ECS — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Serialization/BurstParamsSerialization.hpp`

| シグネチャ | 説明 |
|---|---|
| `template <class Archive> void load(Archive &archive, BurstParams &params)` |  |
| `template <class Archive> void save(Archive &archive, const BurstParams &params)` |  |

### Tsukino::BuiltIn::ECS — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Serialization/CameraComponentSerialization.hpp`

| シグネチャ | 説明 |
|---|---|
| `template <class Archive> void load(Archive &archive, CameraComponent &camera)` | CameraComponentのcerealデシリアライズ定義 |
| `template <class Archive> void save(Archive &archive, const CameraComponent &camera)` | CameraComponentのcerealシリアライズ定義 |

### Tsukino::BuiltIn::ECS — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Serialization/CollisionComponentSerialization.hpp`

| シグネチャ | 説明 |
|---|---|
| `template <class Archive> void load(Archive &archive, CollisionComponent &collision)` | CollisionComponentのcerealデシリアライズ定義 |
| `template <class Archive> void save(Archive &archive, const CollisionComponent &collision)` | CollisionComponentのcerealシリアライズ定義 |

### Tsukino::BuiltIn::ECS — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Serialization/DebugCameraComponentSerialization.hpp`

| シグネチャ | 説明 |
|---|---|
| `template <class Archive> void load(Archive &archive, DebugCameraComponent &debugCamera)` | DebugCameraComponentのcerealデシリアライズ定義 |
| `template <class Archive> void save(Archive &archive, const DebugCameraComponent &debugCamera)` | DebugCameraComponentのcerealシリアライズ定義 |

### Tsukino::BuiltIn::ECS — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Serialization/DirectionalLightComponentSerialization.hpp`

| シグネチャ | 説明 |
|---|---|
| `template <class Archive> void load(Archive &archive, DirectionalLightComponent &light)` | DirectionalLightComponentのcerealデシリアライズ定義 |
| `template <class Archive> void save(Archive &archive, const DirectionalLightComponent &light)` | DirectionalLightComponentのcerealシリアライズ定義 |

### Tsukino::BuiltIn::ECS — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Serialization/EffectComponentSerialization.hpp`

| シグネチャ | 説明 |
|---|---|
| `template <class Archive> void load(Archive &archive, EffectComponent &effect)` | EffectComponent のロード処理 |
| `template <class Archive> void save(Archive &archive, const EffectComponent &effect)` | EffectComponent のセーブ処理 |

### Tsukino::BuiltIn::ECS — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Serialization/FogComponentSerialization.hpp`

| シグネチャ | 説明 |
|---|---|
| `template <class Archive> void load(Archive &archive, FogComponent &fog)` | FogComponentのcerealデシリアライズ定義 |
| `template <class Archive> void save(Archive &archive, const FogComponent &fog)` | FogComponentのcerealシリアライズ定義 |

### Tsukino::BuiltIn::ECS — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Serialization/FontComponentSerialization.hpp`

| シグネチャ | 説明 |
|---|---|
| `template <class Archive> void load(Archive &archive, FontComponent &font)` | FontComponentのcerealデシリアライズ定義 |
| `template <class Archive> void save(Archive &archive, const FontComponent &font)` | FontComponentのcerealシリアライズ定義 |

### Tsukino::BuiltIn::ECS — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Serialization/ModelComponentSerialization.hpp`

| シグネチャ | 説明 |
|---|---|
| `template <class Archive> void load(Archive &archive, ModelComponent &model)` | ModelComponentのcerealデシリアライズ定義 |
| `template <class Archive> void save(Archive &archive, const ModelComponent &model)` | ModelComponentのcerealシリアライズ定義 |

### Tsukino::BuiltIn::ECS — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Serialization/MotionBlurComponentSerialization.hpp`

| シグネチャ | 説明 |
|---|---|
| `template <class Archive> void load(Archive &archive, MotionBlurComponent &blur)` | MotionBlurComponentのcerealデシリアライズ定義 |
| `template <class Archive> void save(Archive &archive, const MotionBlurComponent &blur)` | MotionBlurComponentのcerealシリアライズ定義 |

### Tsukino::BuiltIn::ECS — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Serialization/OcclusionCutoutComponentSerialization.hpp`

| シグネチャ | 説明 |
|---|---|
| `template <class Archive> void load(Archive &archive, OcclusionCutoutComponent &cutout)` | OcclusionCutoutComponentのcerealデシリアライズ定義 |
| `template <class Archive> void save(Archive &archive, const OcclusionCutoutComponent &cutout)` | OcclusionCutoutComponentのcerealシリアライズ定義 |

### Tsukino::BuiltIn::ECS — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Serialization/PointLightComponentSerialization.hpp`

| シグネチャ | 説明 |
|---|---|
| `template <class Archive> void load(Archive &archive, PointLightComponent &light)` | PointLightComponentのcerealデシリアライズ定義 |
| `template <class Archive> void save(Archive &archive, const PointLightComponent &light)` | PointLightComponentのcerealシリアライズ定義 |

### Tsukino::BuiltIn::ECS — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Serialization/RigidbodyComponentSerialization.hpp`

| シグネチャ | 説明 |
|---|---|
| `template <class Archive> void load(Archive &archive, RigidbodyComponent &rb)` | RigidbodyComponentのcerealデシリアライズ定義 |
| `template <class Archive> void save(Archive &archive, const RigidbodyComponent &rb)` | RigidbodyComponentのcerealシリアライズ定義 |

### Tsukino::BuiltIn::ECS — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Serialization/SkyAtmosphereComponentSerialization.hpp`

| シグネチャ | 説明 |
|---|---|
| `template <class Archive> void load(Archive &archive, SkyAtmosphereComponent &sky)` | SkyAtmosphereComponentのcerealデシリアライズ定義 |
| `template <class Archive> void save(Archive &archive, const SkyAtmosphereComponent &sky)` | SkyAtmosphereComponentのcerealシリアライズ定義 |

### Tsukino::BuiltIn::ECS — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Serialization/SphereColorGradeComponentSerialization.hpp`

| シグネチャ | 説明 |
|---|---|
| `template <class Archive> void load(Archive &archive, SphereColorGradeComponent &grade)` | SphereColorGradeComponentのcerealデシリアライズ定義 |
| `template <class Archive> void save(Archive &archive, const SphereColorGradeComponent &grade)` | SphereColorGradeComponentのcerealシリアライズ定義 |

### Tsukino::BuiltIn::ECS — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Serialization/SpotLightComponentSerialization.hpp`

| シグネチャ | 説明 |
|---|---|
| `template <class Archive> void load(Archive &archive, SpotLightComponent &light)` | SpotLightComponentのcerealデシリアライズ定義 |
| `template <class Archive> void save(Archive &archive, const SpotLightComponent &light)` | SpotLightComponentのcerealシリアライズ定義 |

### Tsukino::BuiltIn::ECS — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Serialization/SpriteComponentSerialization.hpp`

| シグネチャ | 説明 |
|---|---|
| `template <class Archive> void load(Archive &archive, SpriteComponent &sprite)` | SpriteComponentのcerealデシリアライズ定義 |
| `template <class Archive> void save(Archive &archive, const SpriteComponent &sprite)` | SpriteComponentのcerealシリアライズ定義 |

### Tsukino::BuiltIn::ECS — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Serialization/TerrainGenerationRequestComponentSerialization.hpp`

| シグネチャ | 説明 |
|---|---|
| `template <class Archive> void load(Archive &archive, TerrainGenerationRequestComponent &req)` | TerrainGenerationRequestComponentのcerealデシリアライズ定義 |
| `template <class Archive> void save(Archive &archive, const TerrainGenerationRequestComponent &req)` | TerrainGenerationRequestComponentのcerealシリアライズ定義 |

### Tsukino::BuiltIn::ECS — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Serialization/TransformComponentSerialization.hpp`

| シグネチャ | 説明 |
|---|---|
| `template <class Archive> void load(Archive &archive, TransformComponent &transform)` | cereal用：読み込み（ロード）処理の定義 |
| `template <class Archive> void save(Archive &archive, const TransformComponent &transform)` | cereal用：保存（セーブ）処理の定義 |

### Tsukino::BuiltIn::ECS — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Serialization/UIButtonSoundComponentSerialization.hpp`

| シグネチャ | 説明 |
|---|---|
| `template <class Archive> void load(Archive &archive, UIButtonSoundComponent &sound)` |  |
| `template <class Archive> void save(Archive &archive, const UIButtonSoundComponent &sound)` |  |

### Tsukino::BuiltIn::ECS — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Serialization/UIRectComponentSerialization.hpp`

| シグネチャ | 説明 |
|---|---|
| `template <class Archive> void load(Archive &archive, UIRectComponent &rect)` | UIRectComponentのcerealデシリアライズ定義 |
| `template <class Archive> void save(Archive &archive, const UIRectComponent &rect)` | UIRectComponentのcerealシリアライズ定義 |

### Tsukino::BuiltIn::ECS — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/UI/UICanvas.hpp`

| シグネチャ | 説明 |
|---|---|
| `const UICanvas & GetUICanvas(Tsukino::ECS::Registry &registry)` | レジストリに置いた UI の対応を返す |

### Tsukino::Renderer — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Serialization/DirectionalLightComponentSerialization.hpp`

| シグネチャ | 説明 |
|---|---|
| `template <class Archive> void load(Archive &archive, ShadowSettings &shadow)` | ShadowSettingsのcerealデシリアライズ定義 |
| `template <class Archive> void save(Archive &archive, const ShadowSettings &shadow)` | ShadowSettingsのcerealシリアライズ定義 |

## 名前空間の定数・変数

### Tsukino::BuiltIn::ECS::TransformUtility — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Transform/TransformUtility.hpp`

| シグネチャ | 説明 |
|---|---|
| `int kMaxHierarchyDepth = 64` |  |

### Tsukino::BuiltIn::ECS::UICanvasGroupUtility — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/UI/UICanvasGroupUtility.hpp`

| シグネチャ | 説明 |
|---|---|
| `float kInvisibleAlpha = 0.01f` | これより小さい不透明度は「見えない」とみなします（当たり判定を外すしきい値）。 |

### Tsukino::BuiltIn::ECS::UITweenUtility::Tag — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/UI/UITweenUtility.hpp`

| シグネチャ | 説明 |
|---|---|
| `std::uint32_t kFade = 0xFFFFFF02u` |  |
| `std::uint32_t kPunch = 0xFFFFFF03u` |  |
| `std::uint32_t kSlideX = 0xFFFFFF00u` |  |
| `std::uint32_t kSlideY = 0xFFFFFF01u` |  |
