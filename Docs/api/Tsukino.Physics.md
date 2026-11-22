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
  - PhysicsWorld(), ~PhysicsWorld(), PhysicsWorld(), operator=(), CreateBody(), DestroyBody(), ForgetShapeCache(), SetPositionAndRotation(), MoveKinematic(), SetLinearVelocity(), AddImpulse(), AddAngularImpulse(), AddForce(), AddTorque(), SetMotionType(), SetAllowedDofs(), GetMotionType(), GetBodyState(), GetGravity(), SetUnitsPerMeter(), SetContactTolerances(), Step(), OverlapCapsule(), OverlapBox(), CreateCharacter(), DestroyCharacter(), IsCharacterSupported(), StepCharacter(), DrainContacts(), DebugDrawBody(), DebugDrawCharacters()
- **Tsukino::Physics::PhysicsWorld::Impl** — `Tsukino.Physics/src/PhysicsWorld.cpp`
  - tempAllocator, jobSystem, bpLayerInterface, objVsBpFilter, objPairFilter, physicsSystem, contactListener, heightfieldCache, characterContactListener, characters, nextCharacterId, CreateShape(), CreateHeightfieldShape()
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

## 列挙型

- **Tsukino::Physics::DofMask** — `Tsukino.Physics/include/Tsukino/Physics/PhysicsTypes.hpp` — 移動・回転の許可軸を表すビットマスク
  - `None = 0` — 全軸を凍結
  - `TranslationX = 1 << 0` — X 方向の移動を許可
  - `TranslationY = 1 << 1` — Y 方向の移動を許可
  - `TranslationZ = 1 << 2` — Z 方向の移動を許可
  - `RotationX = 1 << 3` — X 軸まわりの回転を許可
  - `RotationY = 1 << 4` — Y 軸まわりの回転を許可
  - `RotationZ = 1 << 5` — Z 軸まわりの回転を許可
  - `All = 0x3f` — 全軸を許可
- **Tsukino::Physics::MotionType** — `Tsukino.Physics/include/Tsukino/Physics/PhysicsTypes.hpp` — ボディの運動タイプ
  - `Static` — 動かない。他から押されもしない
  - `Kinematic` — 位置を外部から与える。物理演算では動かない
  - `Dynamic` — 物理演算で動く
- **Tsukino::Physics::ShapeType** — `Tsukino.Physics/include/Tsukino/Physics/PhysicsTypes.hpp` — 衝突判定に使用する形状の種類
  - `Box` — 直方体
  - `Sphere` — 球
  - `Capsule` — カプセル
  - `Heightfield` — ハイトフィールド（地形）

## 名前空間の関数

### Tsukino::Physics::SpringBonePhysics — `Tsukino.Physics/include/Tsukino/Physics/SpringBone/SpringBonePhysics.hpp`

| シグネチャ | 説明 |
|---|---|
| `SpringBoneChain BuildChainFromHierarchy(const std::string &name, u32 anchorNodeIndex, const std::vector< GraphicsCommon::NodeData > &nodes, const std::unordered_set< std::string > &excludeNodeNames, u32 maxDepth, const SpringBoneSettings &settings)` | ノード階層をDFSで辿り、揺れ物チェーンを構築する |
| `SpringBoneChain BuildChainFromRoot(const std::string &name, u32 rootNodeIndex, const std::vector< GraphicsCommon::NodeData > &nodes, const std::unordered_set< std::string > &excludeNodeNames, u32 maxDepth, const SpringBoneSettings &settings)` | 特定の1本のボーン(rootNodeIndex)を起点にチェーンを構築する。 その親を「動かないアンカー」として自動的に使うが、 アンカーの他の子（兄弟ボーン）は一切巻き込まない。 胸のように「特定の1本だけを、共通の親を基準に揺らしたい」 ケース向け（BuildChainFromHierarchyだと親の子を全部拾ってしまう）。 |
| `void InitializeChain(SpringBoneChain &chain, const std::vector< WorldPose > &animatedPoses)` | 現在の姿勢でシミュレーション状態を初期化する （SpringBoneComponent登録直後、物理を破綻させないために Update()の前に一度呼ぶ） |
| `void UpdateChain(SpringBoneChain &chain, float deltaTime, const std::vector< WorldPose > &animatedPoses)` | 1チェーン分の物理を1フレーム進める |

### Tsukino::Physics — `Tsukino.Physics/include/Tsukino/Physics/PhysicsMath.hpp`

| シグネチャ | 説明 |
|---|---|
| `void ComposeTransform(const hlslpp::float3 &position, const hlslpp::quaternion &rotation, const hlslpp::float3 &offsetPosition, const hlslpp::quaternion &offsetRotation, hlslpp::float3 &outPosition, hlslpp::quaternion &outRotation)` | 親の姿勢へローカルオフセットを合成した姿勢を求めます。 |

### Tsukino::Physics — `Tsukino.Physics/include/Tsukino/Physics/PhysicsTypes.hpp`

| シグネチャ | 説明 |
|---|---|
| `DofMask operator&(DofMask lhs, DofMask rhs)` |  |
| `DofMask & operator&=(DofMask &lhs, DofMask rhs)` |  |
| `DofMask operator\|(DofMask lhs, DofMask rhs)` |  |
| `DofMask operator~(DofMask mask)` |  |

### Tsukino::Physics — `Tsukino.Physics/include/Tsukino/Physics/SpringBone/SpringBoneMath.hpp`

| シグネチャ | 説明 |
|---|---|
| `hlslpp::float3 NLerpDirection(const hlslpp::float3 &from, const hlslpp::float3 &to, float t)` | 方向ベクトルの簡易球面補間（角度制限のクランプ用途） |
| `hlslpp::quaternion QuatFromToRotation(const hlslpp::float3 &from, const hlslpp::float3 &to)` | 正規化済み2ベクトル間の最短回転クォータニオンを求める |

## 名前空間の定数・変数

### Tsukino::Physics — `Tsukino.Physics/include/Tsukino/Physics/BodyHandle.hpp`

| シグネチャ | 説明 |
|---|---|
| `uint32_t cInvalidHandleValue = 0xffffffffu` | 無効なハンドルを表す値（Jolt の cInvalidBodyID と同一） |
