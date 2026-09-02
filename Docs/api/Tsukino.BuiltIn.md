# Tsukino.BuiltIn の公開 API

**このファイルは自動生成です。直接編集しないでください。**

型の在処だけ知りたいときは `../api-index.md` を見る。

## 全公開型の索引

メンバ名のみ。詳細が要るときはヘッダを開く。

- **Tsukino::BuiltIn::BuiltInAssets** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/BuiltInAssets.hpp`
  - shaders, fonts, Initialize()
- **Tsukino::BuiltIn::BuiltInFonts** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/BuiltInFonts.hpp`
  - defaultFont, Initialize()
- **Tsukino::BuiltIn::BuiltInShaders** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/BuiltInShaders.hpp`
  - spriteVS, spritePS, spriteWorldVS, modelVS, modelPS, staticModelVS, debugVS, debugPS, shadowVS, shadowStaticVS, shadowPS, skyVS, skyPS, tonemapVS, tonemapPS, waterPS, gbufferPS, lightingPS, motionBlurPS, fogPS, Initialize()
- **Tsukino::BuiltIn::ECS::AnimationControllerComponent** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/AnimationControllerComponent.hpp`
  - blend_alpha, is_transitioning, next, outgoing
- **Tsukino::BuiltIn::ECS::AnimationControllerComponent::NextAnimation** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/AnimationControllerComponent.hpp`
  - clip, animation_index, fade_time, immediate, is_looping, clip_start_time, clip_end_time, in_place
- **Tsukino::BuiltIn::ECS::AnimationControllerComponent::OutgoingAnimation** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/AnimationControllerComponent.hpp`
  - clip, animation_index, elapsed_time, is_looping, clip_start_time, clip_end_time, in_place
- **Tsukino::BuiltIn::ECS::AnimationPlayerComponent** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/AnimationPlayerComponent.hpp`
  - current_clip_id, animation_index, elapsed_time, playback_speed, is_looping, is_playing, is_finished, clip_start_time, clip_end_time, in_place, root_motion_node_name, root_motion_node_index, root_motion_resolved, root_motion_lock_active, root_motion_lock_x, root_motion_lock_z
- **Tsukino::BuiltIn::ECS::BoxCollider2DComponent** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/BoxCollider2DComponent.hpp`
  - offset, size
- **Tsukino::BuiltIn::ECS::CameraComponent** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/CameraComponent.hpp`
  - projectionType, orthoSize, fov, aspectRatio, nearZ, farZ, useLookAt, lookAtTarget, viewMatrix, projectionMatrix, viewProjMatrix, invViewProjMatrix, isPrimary, dirty
- **Tsukino::BuiltIn::ECS::CharacterControllerComponent** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/CharacterControllerComponent.hpp`
  - radius, halfHeight, maxSlopeDeg, mass, gravityFactor, centerOffset, isInitialized, moveInput, jumpRequested, jumpSpeed, verticalVelocity, isGrounded
- **Tsukino::BuiltIn::ECS::CollisionComponent** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/CollisionComponent.hpp`
  - bodyID, type, extent, offsetPosition, offsetRotation, heightfieldSamples, heightfieldSize, heightfieldOffset, heightfieldScale, isSensor, isInitialized, onCollisionEnter, IsValid()
- **Tsukino::BuiltIn::ECS::CollisionEnterEvent** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Event/CollisionEnterEvent.hpp`
  - self, other, normal
- **Tsukino::BuiltIn::ECS::DebugCameraComponent** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/DebugCameraComponent.hpp`
  - moveSpeed, sprintSpeed, mouseSens, yaw, pitch, isActive
- **Tsukino::BuiltIn::ECS::DebugCameraTag** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/DebugCameraTag.hpp`
  - dummy
- **Tsukino::BuiltIn::ECS::DirectionalLightComponent** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/DirectionalLightComponent.hpp`
  - direction, color, intensity, castShadow
- **Tsukino::BuiltIn::ECS::DraggableComponent** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/DraggableComponent.hpp`
  - isDragging, dragOffset
- **Tsukino::BuiltIn::ECS::EffectComponent** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/EffectComponent.hpp`
  - effectAsset, effectPath, handle, playSpeed, looping, stopped, active, scale, followRotation
- **Tsukino::BuiltIn::ECS::FogComponent** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/FogComponent.hpp`
  - enabled, color, density, startDistance, maxOpacity, heightFogEnabled, height, heightFalloff, heightDensity, sunColor, sunScatterPower, noiseEnabled, noiseScale, noiseIntensity, windDirection, windSpeed
- **Tsukino::BuiltIn::ECS::FontComponent** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/FontComponent.hpp`
  - text, fontHandle, color, origin, horizontalAlign, verticalAlign, outlineColor, outlineWidth, sortOrder
- **Tsukino::BuiltIn::ECS::HighlightComponent** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/HighlightComponent.hpp`
  - active, rimColor, rimIntensity, rimPower, glow
- **Tsukino::BuiltIn::ECS::ImpulseRequestComponent** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/ImpulseRequestComponent.hpp`
  - impulse, angularImpulse
- **Tsukino::BuiltIn::ECS::ModelComponent** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/ModelComponent.hpp`
  - modelHandle, visible, opacity
- **Tsukino::BuiltIn::ECS::MotionBlurComponent** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/MotionBlurComponent.hpp`
  - enabled, strength, maxBlurRadius, sampleCount, targetFps
- **Tsukino::BuiltIn::ECS::MotionVectorComponent** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/MotionVectorComponent.hpp`
  - MAX_BONES, prevWorld, prevBones, prevBoneCount, valid
- **Tsukino::BuiltIn::ECS::NodeWorldMatrixComponent** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/NodeWorldMatrixComponent.hpp`
  - matrices
- **Tsukino::BuiltIn::ECS::NodeWorldPoseComponent** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/NodeWorldPoseComponent.hpp`
  - poses
- **Tsukino::BuiltIn::ECS::PointLightComponent** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/PointLightComponent.hpp`
  - color, intensity, range, enabled
- **Tsukino::BuiltIn::ECS::RigidbodyComponent** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/RigidbodyComponent.hpp`
  - type, isTypeDirty, mass, friction, restitution, gravityFactor, linearVelocity, angularVelocity, force, torque, isInitialized, isGrounded, groundCheckDistance, groundCheckRadius, freezePositionX, freezePositionY, freezePositionZ, freezeRotationX, freezeRotationY, freezeRotationZ, isFreezeDirty
- **Tsukino::BuiltIn::ECS::RootMotionComponent** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/RootMotionComponent.hpp`
  - delta_position, delta_rotation
- **Tsukino::BuiltIn::ECS::SkeletonOutputComponent** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/SkeletonOutputComponent.hpp`
  - MAX_BONES, local_matrices, bone_count
- **Tsukino::BuiltIn::ECS::SkyAtmosphereComponent** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/SkyAtmosphereComponent.hpp`
  - rayleighScattering, mieScattering, mieAnisotropy, atmosphereHeight, planetRadius, sunIntensity, sunDiskSize, groundColor
- **Tsukino::BuiltIn::ECS::SpotLightComponent** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/SpotLightComponent.hpp`
  - color, intensity, range, innerConeDeg, outerConeDeg, enabled
- **Tsukino::BuiltIn::ECS::SpringBoneComponent** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/SpringBoneComponent.hpp`
  - chainDefs, chains, resolved, enabled
- **Tsukino::BuiltIn::ECS::SpringBoneComponent::ChainDef** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/SpringBoneComponent.hpp`
  - name, anchorNodeName, rootNodeName, excludeNodeNames, maxDepth, settings, colliders
- **Tsukino::BuiltIn::ECS::SpringBoneComponent::ColliderDef** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/SpringBoneComponent.hpp`
  - attachNodeName, localOffset, radius
- **Tsukino::BuiltIn::ECS::SpriteComponent** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/SpriteComponent.hpp`
  - textureHandle, blendMode, space, tintColor, uvRect, sortOrder
- **Tsukino::BuiltIn::ECS::TerrainGenerationRequestComponent** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/TerrainGenerationRequestComponent.hpp`
  - amplitude, noiseFrequency, seed, noiseType, collisionModelHandle
- **Tsukino::BuiltIn::ECS::TransformComponent** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp`
  - position, rotation, scale, localMatrix, worldMatrix, parent, dirty
- **Tsukino::BuiltIn::ECS::WorldAnchorComponent** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/WorldAnchorComponent.hpp`
  - target, useFixedWorldPosition, fixedWorldPosition, worldOffset, screenOffset, visible
