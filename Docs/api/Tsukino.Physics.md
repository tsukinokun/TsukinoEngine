# Tsukino.Physics の公開 API

**このファイルは自動生成です。直接編集しないでください。**

型の在処だけ知りたいときは `../api-index.md` を見る。

## 全公開型の索引

メンバ名のみ。詳細が要るときはヘッダを開く。

- **Tsukino::Physics::BPLayerInterfaceImpl** — `Tsukino.Physics/src/JoltLayers.hpp`
  - BPLayerInterfaceImpl(), GetNumBroadPhaseLayers(), GetBroadPhaseLayer()
- **Tsukino::Physics::BodyDesc** — `Tsukino.Physics/include/Tsukino/Physics/PhysicsTypes.hpp`
  - shape, position, rotation, motion, isSensor, mass, friction, restitution, gravityFactor, allowedDofs, overrideMassProperties, userData
- **Tsukino::Physics::BodyHandle** — `Tsukino.Physics/include/Tsukino/Physics/BodyHandle.hpp`
  - value, IsValid(), operator==(), operator!=()
- **Tsukino::Physics::BodyState** — `Tsukino.Physics/include/Tsukino/Physics/PhysicsTypes.hpp`
  - position, rotation, linearVelocity, angularVelocity
- **Tsukino::Physics::CharacterContactListenerImpl** — `Tsukino.Physics/src/JoltContactListener.hpp`
  - physicsSystem, OnContactAdded()
- **Tsukino::Physics::CharacterDesc** — `Tsukino.Physics/include/Tsukino/Physics/PhysicsTypes.hpp`
  - radius, halfHeight, maxSlopeDeg, mass, centerOffset, position, rotation, userData
- **Tsukino::Physics::CharacterHandle** — `Tsukino.Physics/include/Tsukino/Physics/BodyHandle.hpp`
  - value, IsValid(), operator==(), operator!=()
- **Tsukino::Physics::CharacterInput** — `Tsukino.Physics/include/Tsukino/Physics/PhysicsTypes.hpp`
  - linearVelocity, rotation
- **Tsukino::Physics::CharacterOutput** — `Tsukino.Physics/include/Tsukino/Physics/PhysicsTypes.hpp`
  - position, rotation, verticalVelocity, isGrounded
- **Tsukino::Physics::ContactListenerImpl** — `Tsukino.Physics/src/JoltContactListener.hpp`
  - OnContactAdded(), DrainContacts()
- **Tsukino::Physics::ContactRecord** — `Tsukino.Physics/include/Tsukino/Physics/PhysicsTypes.hpp`
  - userDataA, userDataB, normal
- **Tsukino::Physics::IPhysicsDebugDraw** — `Tsukino.Physics/include/Tsukino/Physics/IPhysicsDebugDraw.hpp`
  - ~IPhysicsDebugDraw(), DrawLine(), DrawTriangle()
- **Tsukino::Physics::JoltDebugRenderer** — `Tsukino.Physics/src/JoltDebugRenderer.hpp`
  - JoltDebugRenderer(), SetSink(), DrawLine(), DrawTriangle(), DrawText3D()
- **Tsukino::Physics::ObjectLayerPairFilterImpl** — `Tsukino.Physics/src/JoltLayers.hpp`
  - ShouldCollide()
- **Tsukino::Physics::ObjectVsBroadPhaseLayerFilterImpl** — `Tsukino.Physics/src/JoltLayers.hpp`
  - ShouldCollide()
- **Tsukino::Physics::PhysicsWorld** — `Tsukino.Physics/include/Tsukino/Physics/PhysicsWorld.hpp`
  - PhysicsWorld(), ~PhysicsWorld(), PhysicsWorld(), operator=(), CreateBody(), DestroyBody(), ForgetShapeCache(), SetPositionAndRotation(), SetLinearVelocity(), AddImpulse(), AddAngularImpulse(), AddForce(), AddTorque(), SetMotionType(), SetAllowedDofs(), GetMotionType(), GetBodyState(), GetGravity(), Step(), OverlapCapsule(), OverlapBox(), CreateCharacter(), DestroyCharacter(), IsCharacterSupported(), StepCharacter(), DrainContacts(), DebugDrawBody(), DebugDrawCharacters()
- **Tsukino::Physics::PhysicsWorld::Impl** — `Tsukino.Physics/src/PhysicsWorld.cpp`
  - tempAllocator, jobSystem, bpLayerInterface, objVsBpFilter, objPairFilter, physicsSystem, contactListener, debugRenderer, heightfieldCache, characterContactListener, characters, nextCharacterId, CreateShape(), CreateHeightfieldShape()
- **Tsukino::Physics::ShapeDesc** — `Tsukino.Physics/include/Tsukino/Physics/PhysicsTypes.hpp`
  - type, extent, heightSamples, heightSize, heightOffset, heightScale
- **Tsukino::Physics::SpringBoneChain** — `Tsukino.Physics/include/Tsukino/Physics/SpringBone/SpringBoneData.hpp`
  - name, anchorNodeIndex, settings, nodes, colliders, previousAnchorPosition, anchorInitialized
- **Tsukino::Physics::SpringBoneNode** — `Tsukino.Physics/include/Tsukino/Physics/SpringBone/SpringBoneData.hpp`
  - nodeIndex, parentIndexInChain, restLength, currentPosition, previousPosition, correctedRotation, initialized
- **Tsukino::Physics::SpringBoneSettings** — `Tsukino.Physics/include/Tsukino/Physics/SpringBone/SpringBoneData.hpp`
  - stiffness, drag, inertia, gravityScale, gravityDir, boneRadius, angleLimitDeg, collisionIterations, serialize()
- **Tsukino::Physics::SpringColliderSphere** — `Tsukino.Physics/include/Tsukino/Physics/SpringBone/SpringBoneData.hpp`
  - attachNodeIndex, localOffset, radius, serialize()
- **Tsukino::Physics::WorldPose** — `Tsukino.Physics/include/Tsukino/Physics/SpringBone/SpringBoneData.hpp`
  - position, rotation
