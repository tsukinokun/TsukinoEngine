//----------------------------------------------------------------------------
//! @file   PhysicsWorld.cpp
//! @brief  PhysicsWorld クラスの実装
//! @detail Jolt Physics の初期化から破棄まで、およびボディ・キャラクター・
//!         形状クエリ・接触・デバッグ描画の全てをこのファイルに閉じ込めています。
//----------------------------------------------------------------------------
#include <Tsukino/Physics/PhysicsWorld.hpp>

#include <Tsukino/Physics/IPhysicsDebugDraw.hpp>

#include "JoltContactListener.hpp"
#include "JoltConversion.hpp"
#include "JoltDebugRenderer.hpp"
#include "JoltLayers.hpp"

#include <Tsukino/Core/Log.hpp>

#include <Jolt/Jolt.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Geometry/AABox.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyLock.h>
#include <Jolt/Physics/Character/CharacterVirtual.h>
#include <Jolt/Physics/Collision/CollideShape.h>
#include <Jolt/Physics/Collision/CollisionCollectorImpl.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/HeightFieldShape.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/RegisterTypes.h>

#include <string>
#include <thread>
#include <unordered_map>

// 名前空間 : Tsukino::Physics
namespace Tsukino::Physics {

    namespace {

        //--------------------------------------------------------------------
        //! Tsukino の運動タイプを Jolt の運動タイプへ変換します。
        //! @param  [in] motion 変換元
        //! @return 対応する Jolt の運動タイプ
        //--------------------------------------------------------------------
        JPH::EMotionType ToJoltMotionType(MotionType motion) {
            switch(motion) {
            case MotionType::Kinematic:
                return JPH::EMotionType::Kinematic;
            case MotionType::Dynamic:
                return JPH::EMotionType::Dynamic;
            case MotionType::Static:
            default:
                return JPH::EMotionType::Static;
            }
        }

        //--------------------------------------------------------------------
        //! Jolt の運動タイプを Tsukino の運動タイプへ変換します。
        //! @param  [in] motion 変換元
        //! @return 対応する Tsukino の運動タイプ
        //--------------------------------------------------------------------
        MotionType ToTsukinoMotionType(JPH::EMotionType motion) {
            switch(motion) {
            case JPH::EMotionType::Kinematic:
                return MotionType::Kinematic;
            case JPH::EMotionType::Dynamic:
                return MotionType::Dynamic;
            case JPH::EMotionType::Static:
            default:
                return MotionType::Static;
            }
        }

        //--------------------------------------------------------------------
        //! 運動タイプに対応するオブジェクトレイヤーを返します。
        //! @param  [in] motion 運動タイプ
        //! @return Static なら NON_MOVING、それ以外は MOVING
        //--------------------------------------------------------------------
        JPH::ObjectLayer ToObjectLayer(MotionType motion) {
            return (motion == MotionType::Static) ? Layers::NON_MOVING : Layers::MOVING;
        }

        //--------------------------------------------------------------------
        //! 許可軸マスクを Jolt の EAllowedDOFs へ変換します。
        //! @param  [in] mask 変換元
        //! @return 対応する Jolt の許可軸
        //--------------------------------------------------------------------
        JPH::EAllowedDOFs ToJoltAllowedDofs(DofMask mask) {
            JPH::EAllowedDOFs dofs = JPH::EAllowedDOFs::None;
            if((mask & DofMask::TranslationX) != DofMask::None)
                dofs = dofs | JPH::EAllowedDOFs::TranslationX;
            if((mask & DofMask::TranslationY) != DofMask::None)
                dofs = dofs | JPH::EAllowedDOFs::TranslationY;
            if((mask & DofMask::TranslationZ) != DofMask::None)
                dofs = dofs | JPH::EAllowedDOFs::TranslationZ;
            if((mask & DofMask::RotationX) != DofMask::None)
                dofs = dofs | JPH::EAllowedDOFs::RotationX;
            if((mask & DofMask::RotationY) != DofMask::None)
                dofs = dofs | JPH::EAllowedDOFs::RotationY;
            if((mask & DofMask::RotationZ) != DofMask::None)
                dofs = dofs | JPH::EAllowedDOFs::RotationZ;
            return dofs;
        }

        //--------------------------------------------------------------------
        //! Jolt のグローバルな初期化を一度だけ行います。
        //! @note   アロケータ・ファクトリ・型登録はプロセスに1組しか持てないため、
        //!         PhysicsWorld を複数生成しても初回のみ実行されます
        //--------------------------------------------------------------------
        void EnsureJoltInitialized() {
            static bool isJoltInitialized = false;
            if(isJoltInitialized)
                return;

            JPH::RegisterDefaultAllocator();
            JPH::Factory::sInstance = new JPH::Factory();
            JPH::RegisterTypes();
            isJoltInitialized = true;
        }

    }    // namespace

    //------------------------------------------------------------------------
    //! @struct PhysicsWorld::Impl
    //! 物理ワールドの実装を隠蔽する構造体
    //------------------------------------------------------------------------
    struct PhysicsWorld::Impl {
        JPH::TempAllocatorImpl*           tempAllocator = nullptr;    //!< 物理計算用一時アロケータ
        JPH::JobSystemThreadPool*         jobSystem     = nullptr;    //!< 物理シミュレーション用ジョブシステム
        BPLayerInterfaceImpl              bpLayerInterface;           //!< ブロードフェーズインターフェース
        ObjectVsBroadPhaseLayerFilterImpl objVsBpFilter;              //!< オブジェクト対ブロードフェーズ層フィルタ
        ObjectLayerPairFilterImpl         objPairFilter;              //!< オブジェクト層間フィルタ

        JPH::PhysicsSystem*  physicsSystem   = nullptr;    //!< Jolt 物理システム本体
        ContactListenerImpl* contactListener = nullptr;    //!< 衝突イベントリスナー
        JoltDebugRenderer*   debugRenderer   = nullptr;    //!< デバッグ描画インターフェース

        //! ハイトフィールド用キャッシュ（Shape を直接保持して使い回す）
        std::unordered_map<uint64_t, JPH::Ref<JPH::HeightFieldShape>> heightfieldCache;

        CharacterContactListenerImpl                                  characterContactListener;    //!< Character 用接触リスナー
        std::unordered_map<uint32_t, JPH::Ref<JPH::CharacterVirtual>> characters;                  //!< 生存中のキャラクター
        uint32_t                                                      nextCharacterId = 0;         //!< 次に払い出すキャラクター識別子

        //--------------------------------------------------------------------
        //! 形状の指定から Jolt の Shape を生成します。
        //! @param  [in] desc     形状の指定
        //! @param  [in] cacheKey ハイトフィールド形状の使い回しに使うキー
        //! @return 生成された Shape。生成できなかった場合は空の参照
        //--------------------------------------------------------------------
        JPH::RefConst<JPH::Shape> CreateShape(const ShapeDesc& desc, uint64_t cacheKey) {
            switch(desc.type) {
            case ShapeType::Box:
                return new JPH::BoxShape(ToJoltVec3(desc.extent));

            case ShapeType::Sphere:
                return new JPH::SphereShape(desc.extent.x);

            case ShapeType::Capsule:
                return new JPH::CapsuleShape(desc.extent.y, desc.extent.x);

            case ShapeType::Heightfield:
                return CreateHeightfieldShape(desc, cacheKey);

            default:
                return {};
            }
        }

        //--------------------------------------------------------------------
        //! ハイトフィールド形状を生成、またはキャッシュから取り出します。
        //! @param  [in] desc     形状の指定
        //! @param  [in] cacheKey 使い回しに使うキー
        //! @return 生成された Shape。生成できなかった場合は空の参照
        //--------------------------------------------------------------------
        JPH::RefConst<JPH::Shape> CreateHeightfieldShape(const ShapeDesc& desc, uint64_t cacheKey) {
            if(desc.heightSamples == nullptr || desc.heightSize == 0)
                return {};

            auto cacheIt = heightfieldCache.find(cacheKey);
            if(cacheIt != heightfieldCache.end())
                return JPH::RefConst<JPH::Shape>(cacheIt->second.GetPtr());

            //----------------------------------------------------------------
            // HeightFieldShapeSettings の Offset と Scale は必ず対で合わせる。
            // heightOffset は地形の左下隅(Min)、heightScale はグリッド1つあたりの
            // 間隔(サイズ)を指すという取り決めになっている
            //----------------------------------------------------------------
            JPH::HeightFieldShapeSettings hfSettings(
                desc.heightSamples, ToJoltVec3(desc.heightOffset), ToJoltVec3(desc.heightScale), desc.heightSize);

            // このフラグが「地形の法線」を正しく計算させる
            hfSettings.mBlockSize = 2;    // デフォルトの2にする

            JPH::Shape::ShapeResult hfResult = hfSettings.Create();
            if(!hfResult.IsValid()) {
                Tsukino::Core::Log::Error(std::string("Failed to create heightfield shape: ") + hfResult.GetError().c_str());
                return {};
            }

            JPH::RefConst<JPH::Shape> shape = hfResult.Get();
            heightfieldCache[cacheKey]      = (JPH::HeightFieldShape*)shape.GetPtr();

            JPH::AABox localBounds = shape->GetLocalBounds();
            Tsukino::Core::Log::Info("HeightField local bounds min=(" + std::to_string(localBounds.mMin.GetX()) + ","
                                     + std::to_string(localBounds.mMin.GetZ()) + ") max=(" + std::to_string(localBounds.mMax.GetX()) + ","
                                     + std::to_string(localBounds.mMax.GetZ()) + ")");
            Tsukino::Core::Log::Info("Requested offset=(" + std::to_string(float(desc.heightOffset.x)) + ","
                                     + std::to_string(float(desc.heightOffset.z)) + ") scale=(" + std::to_string(float(desc.heightScale.x)) + ","
                                     + std::to_string(float(desc.heightScale.z)) + ") size=" + std::to_string(desc.heightSize));

            return shape;
        }
    };

    //------------------------------------------------------------------------
    //! コンストラクタ
    //------------------------------------------------------------------------
    PhysicsWorld::PhysicsWorld() {
        m_impl = new Impl();

        EnsureJoltInitialized();

        m_impl->tempAllocator = new JPH::TempAllocatorImpl(10 * 1024 * 1024);
        m_impl->jobSystem = new JPH::JobSystemThreadPool(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, std::thread::hardware_concurrency() - 1);

        m_impl->physicsSystem                 = new JPH::PhysicsSystem();
        const uint32_t cMaxBodies             = 1024;
        const uint32_t cNumBodyMutexes        = 0;
        const uint32_t cMaxBodyPairs          = 1024;
        const uint32_t cMaxContactConstraints = 1024;

        m_impl->physicsSystem->Init(
            cMaxBodies, cNumBodyMutexes, cMaxBodyPairs, cMaxContactConstraints, m_impl->bpLayerInterface, m_impl->objVsBpFilter, m_impl->objPairFilter);

        m_impl->contactListener = new ContactListenerImpl();
        m_impl->physicsSystem->SetContactListener(m_impl->contactListener);
        m_impl->characterContactListener.physicsSystem = m_impl->physicsSystem;

        m_impl->debugRenderer = new JoltDebugRenderer();
    }

    //------------------------------------------------------------------------
    //! デストラクタ
    //------------------------------------------------------------------------
    PhysicsWorld::~PhysicsWorld() {
        if(!m_impl)
            return;

        //--------------------------------------------------------------------
        // CharacterVirtual は PhysicsSystem を参照しているため、先に手放す
        //--------------------------------------------------------------------
        m_impl->characters.clear();
        m_impl->heightfieldCache.clear();

        delete m_impl->debugRenderer;
        delete m_impl->contactListener;
        delete m_impl->physicsSystem;
        delete m_impl->jobSystem;
        delete m_impl->tempAllocator;
        delete m_impl;
        m_impl = nullptr;
    }

    //------------------------------------------------------------------------
    //! ボディを生成してワールドへ追加します。
    //------------------------------------------------------------------------
    BodyHandle PhysicsWorld::CreateBody(const BodyDesc& desc, uint64_t shapeCacheKey) {
        if(!m_impl || !m_impl->physicsSystem)
            return BodyHandle{};

        JPH::RefConst<JPH::Shape> shape = m_impl->CreateShape(desc.shape, shapeCacheKey);
        if(!shape)
            return BodyHandle{};

        JPH::BodyCreationSettings settings(
            shape, ToJoltRVec3(desc.position), ToJoltQuat(desc.rotation), ToJoltMotionType(desc.motion), ToObjectLayer(desc.motion));

        settings.mIsSensor      = desc.isSensor;
        settings.mFriction      = desc.friction;
        settings.mRestitution   = desc.restitution;
        settings.mGravityFactor = desc.gravityFactor;

        //--------------------------------------------------------------------
        // 質量と許可軸は、呼び出し側が明示したときだけ上書きする。
        // 指定が無いボディは Jolt の既定（形状の密度から算出した質量・全軸許可）に任せる
        //--------------------------------------------------------------------
        if(desc.overrideMassProperties) {
            settings.mOverrideMassProperties       = JPH::EOverrideMassProperties::CalculateInertia;
            settings.mMassPropertiesOverride.mMass = desc.mass;
            settings.mAllowedDOFs                  = ToJoltAllowedDofs(desc.allowedDofs);
        }

        JPH::BodyInterface& bodyInterface = m_impl->physicsSystem->GetBodyInterface();

        JPH::Body* body = bodyInterface.CreateBody(settings);
        if(!body)
            return BodyHandle{};

        body->SetUserData(desc.userData);

        const JPH::BodyID bodyID = body->GetID();
        bodyInterface.AddBody(bodyID, JPH::EActivation::Activate);

        return BodyHandle{bodyID.GetIndexAndSequenceNumber()};
    }

    //------------------------------------------------------------------------
    //! ボディをワールドから取り除いて破棄します。
    //------------------------------------------------------------------------
    void PhysicsWorld::DestroyBody(BodyHandle handle) {
        if(!m_impl || !m_impl->physicsSystem || !handle.IsValid())
            return;

        const JPH::BodyID   bodyID(handle.value);
        JPH::BodyInterface& bodyInterface = m_impl->physicsSystem->GetBodyInterface();

        // AddBody していた場合のみ RemoveBody する
        if(bodyInterface.IsAdded(bodyID)) {
            bodyInterface.RemoveBody(bodyID);
        }
        bodyInterface.DestroyBody(bodyID);
    }

    //------------------------------------------------------------------------
    //! キャッシュ済みのハイトフィールド形状を解放します。
    //------------------------------------------------------------------------
    void PhysicsWorld::ForgetShapeCache(uint64_t shapeCacheKey) {
        if(!m_impl)
            return;

        m_impl->heightfieldCache.erase(shapeCacheKey);
    }

    //------------------------------------------------------------------------
    //! ボディの位置と向きを直接設定します。
    //------------------------------------------------------------------------
    void PhysicsWorld::SetPositionAndRotation(BodyHandle handle, const hlslpp::float3& position, const hlslpp::quaternion& rotation) {
        if(!m_impl || !m_impl->physicsSystem || !handle.IsValid())
            return;

        m_impl->physicsSystem->GetBodyInterface().SetPositionAndRotation(
            JPH::BodyID(handle.value), ToJoltRVec3(position), ToJoltQuat(rotation), JPH::EActivation::Activate);
    }

    //------------------------------------------------------------------------
    //! ボディの並進速度を設定します。
    //------------------------------------------------------------------------
    void PhysicsWorld::SetLinearVelocity(BodyHandle handle, const hlslpp::float3& velocity) {
        if(!m_impl || !m_impl->physicsSystem || !handle.IsValid())
            return;

        m_impl->physicsSystem->GetBodyInterface().SetLinearVelocity(JPH::BodyID(handle.value), ToJoltVec3(velocity));
    }

    //------------------------------------------------------------------------
    //! ボディへ撃力を加えます。
    //------------------------------------------------------------------------
    void PhysicsWorld::AddImpulse(BodyHandle handle, const hlslpp::float3& impulse) {
        if(!m_impl || !m_impl->physicsSystem || !handle.IsValid())
            return;

        m_impl->physicsSystem->GetBodyInterface().AddImpulse(JPH::BodyID(handle.value), ToJoltVec3(impulse));
    }

    //------------------------------------------------------------------------
    //! ボディへ角撃力を加えます。
    //------------------------------------------------------------------------
    void PhysicsWorld::AddAngularImpulse(BodyHandle handle, const hlslpp::float3& angularImpulse) {
        if(!m_impl || !m_impl->physicsSystem || !handle.IsValid())
            return;

        m_impl->physicsSystem->GetBodyInterface().AddAngularImpulse(JPH::BodyID(handle.value), ToJoltVec3(angularImpulse));
    }

    //------------------------------------------------------------------------
    //! ボディへ力を加えます。
    //------------------------------------------------------------------------
    void PhysicsWorld::AddForce(BodyHandle handle, const hlslpp::float3& force) {
        if(!m_impl || !m_impl->physicsSystem || !handle.IsValid())
            return;

        m_impl->physicsSystem->GetBodyInterface().AddForce(JPH::BodyID(handle.value), ToJoltVec3(force));
    }

    //------------------------------------------------------------------------
    //! ボディへトルクを加えます。
    //------------------------------------------------------------------------
    void PhysicsWorld::AddTorque(BodyHandle handle, const hlslpp::float3& torque) {
        if(!m_impl || !m_impl->physicsSystem || !handle.IsValid())
            return;

        m_impl->physicsSystem->GetBodyInterface().AddTorque(JPH::BodyID(handle.value), ToJoltVec3(torque));
    }

    //------------------------------------------------------------------------
    //! ボディの運動タイプを変更します。
    //------------------------------------------------------------------------
    void PhysicsWorld::SetMotionType(BodyHandle handle, MotionType motion) {
        if(!m_impl || !m_impl->physicsSystem || !handle.IsValid())
            return;

        m_impl->physicsSystem->GetBodyInterface().SetMotionType(JPH::BodyID(handle.value), ToJoltMotionType(motion), JPH::EActivation::Activate);
    }

    //------------------------------------------------------------------------
    //! 移動・回転の許可軸を変更します。
    //------------------------------------------------------------------------
    void PhysicsWorld::SetAllowedDofs(BodyHandle handle, DofMask dofs, float mass) {
        if(!m_impl || !m_impl->physicsSystem || !handle.IsValid())
            return;

        const JPH::EAllowedDOFs joltDofs = ToJoltAllowedDofs(dofs);

        // BodyLockWrite で MotionProperties を直接書き換える
        JPH::BodyLockWrite lock(m_impl->physicsSystem->GetBodyLockInterface(), JPH::BodyID(handle.value));
        if(!lock.Succeeded())
            return;

        JPH::Body& body = lock.GetBody();

        // Static / Kinematic は mAllowedDOFs が実質意味を持たないため何もしない
        if(body.GetMotionType() != JPH::EMotionType::Dynamic)
            return;

        JPH::MotionProperties* motionProperties = body.GetMotionProperties();

        //--------------------------------------------------------------------
        // 現在の質量を保ったまま DOF のみ変更したいので、既存の質量から
        // MassProperties を組み直す
        //--------------------------------------------------------------------
        JPH::MassProperties massProps;
        massProps.mMass    = mass;
        massProps.mInertia = motionProperties->GetLocalSpaceInverseInertia().Inversed3x3();
        motionProperties->SetMassProperties(joltDofs, massProps);

        //--------------------------------------------------------------------
        // 凍結した軸の残存速度をゼロにしておく（急な巻き戻り防止）
        //--------------------------------------------------------------------
        JPH::Vec3 linearVelocity  = motionProperties->GetLinearVelocity();
        JPH::Vec3 angularVelocity = motionProperties->GetAngularVelocity();
        if((dofs & DofMask::TranslationX) == DofMask::None)
            linearVelocity.SetX(0.0f);
        if((dofs & DofMask::TranslationY) == DofMask::None)
            linearVelocity.SetY(0.0f);
        if((dofs & DofMask::TranslationZ) == DofMask::None)
            linearVelocity.SetZ(0.0f);
        if((dofs & DofMask::RotationX) == DofMask::None)
            angularVelocity.SetX(0.0f);
        if((dofs & DofMask::RotationY) == DofMask::None)
            angularVelocity.SetY(0.0f);
        if((dofs & DofMask::RotationZ) == DofMask::None)
            angularVelocity.SetZ(0.0f);
        motionProperties->SetLinearVelocity(linearVelocity);
        motionProperties->SetAngularVelocity(angularVelocity);
    }

    //------------------------------------------------------------------------
    //! ボディの運動タイプを取得します。
    //------------------------------------------------------------------------
    MotionType PhysicsWorld::GetMotionType(BodyHandle handle) const {
        if(!m_impl || !m_impl->physicsSystem || !handle.IsValid())
            return MotionType::Static;

        return ToTsukinoMotionType(m_impl->physicsSystem->GetBodyInterface().GetMotionType(JPH::BodyID(handle.value)));
    }

    //------------------------------------------------------------------------
    //! ボディの位置・向き・速度をまとめて取得します。
    //------------------------------------------------------------------------
    BodyState PhysicsWorld::GetBodyState(BodyHandle handle) const {
        BodyState state;
        if(!m_impl || !m_impl->physicsSystem || !handle.IsValid())
            return state;

        const JPH::BodyID   bodyID(handle.value);
        JPH::BodyInterface& bodyInterface = m_impl->physicsSystem->GetBodyInterface();

        state.position        = ToFloat3(bodyInterface.GetPosition(bodyID));
        state.rotation        = ToQuaternion(bodyInterface.GetRotation(bodyID));
        state.linearVelocity  = ToFloat3(bodyInterface.GetLinearVelocity(bodyID));
        state.angularVelocity = ToFloat3(bodyInterface.GetAngularVelocity(bodyID));
        return state;
    }

    //------------------------------------------------------------------------
    //! ワールドの重力加速度を取得します。
    //------------------------------------------------------------------------
    hlslpp::float3 PhysicsWorld::GetGravity() const {
        if(!m_impl || !m_impl->physicsSystem)
            return hlslpp::float3(0.0f, 0.0f, 0.0f);

        return ToFloat3(m_impl->physicsSystem->GetGravity());
    }

    //------------------------------------------------------------------------
    //! 物理シミュレーションを1ステップ進めます。
    //------------------------------------------------------------------------
    void PhysicsWorld::Step(float deltaTime) {
        if(!m_impl || !m_impl->physicsSystem)
            return;

        m_impl->physicsSystem->Update(deltaTime, 1, m_impl->tempAllocator, m_impl->jobSystem);
    }

    //------------------------------------------------------------------------
    //! 指定のカプセル形状と現在重なっている全ボディのユーザーデータを取得します。
    //------------------------------------------------------------------------
    std::vector<uint64_t> PhysicsWorld::OverlapCapsule(const hlslpp::float3&     center,
                                                       const hlslpp::quaternion& rotation,
                                                       float                     radius,
                                                       float                     halfHeight) const {
        std::vector<uint64_t> result;
        if(!m_impl || !m_impl->physicsSystem)
            return result;

        JPH::CapsuleShape capsuleShape(halfHeight, radius);
        JPH::RMat44       transform = JPH::RMat44::sRotationTranslation(ToJoltQuat(rotation), ToJoltRVec3(center));

        JPH::AllHitCollisionCollector<JPH::CollideShapeCollector> collector;
        m_impl->physicsSystem->GetNarrowPhaseQuery().CollideShape(
            &capsuleShape, JPH::Vec3::sReplicate(1.0f), transform, JPH::CollideShapeSettings(), JPH::RVec3::sZero(), collector);

        JPH::BodyInterface& bodyInterface = m_impl->physicsSystem->GetBodyInterface();
        result.reserve(collector.mHits.size());
        for(const auto& hit : collector.mHits) {
            result.push_back(bodyInterface.GetUserData(hit.mBodyID2));
        }
        return result;
    }

    //------------------------------------------------------------------------
    //! 指定の直方体と重なっているボディがあるかどうかを調べます。
    //------------------------------------------------------------------------
    bool PhysicsWorld::OverlapBox(const hlslpp::float3& center, const hlslpp::float3& halfExtent, BodyHandle ignore) const {
        if(!m_impl || !m_impl->physicsSystem)
            return false;

        JPH::BoxShape                                            checkShape(ToJoltVec3(halfExtent));
        JPH::IgnoreSingleBodyFilter                              bodyFilter(JPH::BodyID(ignore.value));
        JPH::AllHitCollisionCollector<JPH::CollideShapeCollector> collector;

        m_impl->physicsSystem->GetNarrowPhaseQuery().CollideShape(&checkShape,
                                                                  JPH::Vec3::sReplicate(1.0f),
                                                                  JPH::RMat44::sTranslation(ToJoltRVec3(center)),
                                                                  JPH::CollideShapeSettings(),
                                                                  JPH::RVec3::sZero(),
                                                                  collector,
                                                                  {},
                                                                  {},
                                                                  bodyFilter);

        return collector.HadHit();
    }

    //------------------------------------------------------------------------
    //! キャラクターコントローラーを生成します。
    //------------------------------------------------------------------------
    CharacterHandle PhysicsWorld::CreateCharacter(const CharacterDesc& desc) {
        if(!m_impl || !m_impl->physicsSystem)
            return CharacterHandle{};

        JPH::RefConst<JPH::Shape> capsuleShape = new JPH::CapsuleShape(desc.halfHeight, desc.radius);

        //--------------------------------------------------------------------
        // centerOffset が設定されている場合、カプセル中心を基準位置からずらす
        //--------------------------------------------------------------------
        JPH::RefConst<JPH::Shape> shape = capsuleShape;
        if(desc.centerOffset.x != 0.0f || desc.centerOffset.y != 0.0f || desc.centerOffset.z != 0.0f) {
            shape = new JPH::RotatedTranslatedShape(ToJoltVec3(desc.centerOffset), JPH::Quat::sIdentity(), capsuleShape);
        }

        JPH::CharacterVirtualSettings settings;
        settings.mShape         = shape;
        settings.mMaxSlopeAngle = JPH::DegreesToRadians(desc.maxSlopeDeg);
        settings.mMass          = desc.mass;

        //--------------------------------------------------------------------
        // 接地判定に使う平面。カプセル底面付近を接地面とみなす
        // （Jolt基準: SignedDistance(p)=p.y+constant が0以下の点だけが支持候補になる。
        //   centerOffset によりキャラクターの原点はカプセル底面＝足元になっているため、
        //   「足元から radius だけ上まで」を許容範囲とするには constant=-radius が正しい。
        //   +centerOffset.y を足していた以前の実装は符号が逆で、足元(y≈0)の接触点が
        //   常に許容範囲外になり、isGrounded が恒久的に false のままになっていた）
        //--------------------------------------------------------------------
        settings.mSupportingVolume = JPH::Plane(JPH::Vec3::sAxisY(), -desc.radius);

        JPH::Ref<JPH::CharacterVirtual> character = new JPH::CharacterVirtual(
            &settings, ToJoltRVec3(desc.position), ToJoltQuat(desc.rotation), desc.userData, m_impl->physicsSystem);

        character->SetListener(&m_impl->characterContactListener);

        const uint32_t id       = m_impl->nextCharacterId++;
        m_impl->characters[id]  = character;

        return CharacterHandle{id};
    }

    //------------------------------------------------------------------------
    //! キャラクターコントローラーを破棄します。
    //------------------------------------------------------------------------
    void PhysicsWorld::DestroyCharacter(CharacterHandle handle) {
        if(!m_impl || !handle.IsValid())
            return;

        m_impl->characters.erase(handle.value);
    }

    //------------------------------------------------------------------------
    //! キャラクターが接地しているかどうかを返します。
    //------------------------------------------------------------------------
    bool PhysicsWorld::IsCharacterSupported(CharacterHandle handle) const {
        if(!m_impl || !handle.IsValid())
            return false;

        auto it = m_impl->characters.find(handle.value);
        if(it == m_impl->characters.end())
            return false;

        return it->second->IsSupported();
    }

    //------------------------------------------------------------------------
    //! キャラクターを1ステップ進めます。
    //------------------------------------------------------------------------
    bool PhysicsWorld::StepCharacter(CharacterHandle handle, const CharacterInput& input, CharacterOutput& output, float deltaTime) {
        if(!m_impl || !m_impl->physicsSystem || !handle.IsValid())
            return false;

        auto it = m_impl->characters.find(handle.value);
        if(it == m_impl->characters.end())
            return false;

        JPH::CharacterVirtual* character = it->second;

        character->SetLinearVelocity(ToJoltVec3(input.linearVelocity));

        //--------------------------------------------------------------------
        // 呼び出し側が今フレーム書き込んだ向きをキャラクターへ反映する。
        // 反映しないと ExtendedUpdate 後の GetRotation() が常に identity のままとなり、
        // 呼び出し側の回転が毎フレーム上書きされてしまう
        //--------------------------------------------------------------------
        character->SetRotation(ToJoltQuat(input.rotation));

        const JPH::Vec3 gravity = m_impl->physicsSystem->GetGravity();

        JPH::CharacterVirtual::ExtendedUpdateSettings updateSettings;
        character->ExtendedUpdate(deltaTime,
                                  gravity,
                                  updateSettings,
                                  m_impl->physicsSystem->GetDefaultBroadPhaseLayerFilter(Layers::MOVING),
                                  m_impl->physicsSystem->GetDefaultLayerFilter(Layers::MOVING),
                                  {},    // BodyFilter
                                  {},    // ShapeFilter
                                  *m_impl->tempAllocator);

        output.isGrounded       = character->IsSupported();
        output.verticalVelocity = character->GetLinearVelocity().GetY();    // ExtendedUpdate 後の実際の縦速度
        output.position         = ToFloat3(character->GetPosition());
        output.rotation         = ToQuaternion(character->GetRotation());
        return true;
    }

    //------------------------------------------------------------------------
    //! 直前の Step() で溜まった接触を取り出して空にします。
    //------------------------------------------------------------------------
    void PhysicsWorld::DrainContacts(std::vector<ContactRecord>& out) {
        if(!m_impl || !m_impl->contactListener) {
            out.clear();
            return;
        }

        m_impl->contactListener->DrainContacts(out);
    }

    //------------------------------------------------------------------------
    //! ボディの形状をワイヤーフレームで描画します。
    //------------------------------------------------------------------------
    void PhysicsWorld::DebugDrawBody(IPhysicsDebugDraw& sink, BodyHandle handle) const {
        if(!m_impl || !m_impl->physicsSystem || !m_impl->debugRenderer || !handle.IsValid())
            return;

        JPH::BodyLockRead lock(m_impl->physicsSystem->GetBodyLockInterface(), JPH::BodyID(handle.value));
        if(!lock.Succeeded())
            return;

        const JPH::Body& body = lock.GetBody();

        m_impl->debugRenderer->SetSink(&sink);
        body.GetShape()->Draw(m_impl->debugRenderer, body.GetCenterOfMassTransform(), JPH::Vec3::sReplicate(1.0f), JPH::Color::sGreen, false, false);
        m_impl->debugRenderer->SetSink(nullptr);
    }

    //------------------------------------------------------------------------
    //! 生存している全キャラクターの形状をワイヤーフレームで描画します。
    //------------------------------------------------------------------------
    void PhysicsWorld::DebugDrawCharacters(IPhysicsDebugDraw& sink) const {
        if(!m_impl || !m_impl->debugRenderer)
            return;

        m_impl->debugRenderer->SetSink(&sink);

        for(auto& [id, character] : m_impl->characters) {
            if(!character)
                continue;

            JPH::ColorArg color = character->IsSupported() ? JPH::Color::sGreen : JPH::Color::sYellow;

            //----------------------------------------------------------------
            // Shape::Draw() は Center of Mass 基準の変換を期待するため、
            // GetWorldTransform()（position 基準。centerOffset があるとカプセル中心とはズレる）
            // ではなく GetCenterOfMassTransform() を使う
            // （centerOffset=0 の場合は同じ結果になる）
            //----------------------------------------------------------------
            character->GetShape()->Draw(m_impl->debugRenderer, character->GetCenterOfMassTransform(), JPH::Vec3::sReplicate(1.0f), color, false, false);
        }

        m_impl->debugRenderer->SetSink(nullptr);
    }

}    // namespace Tsukino::Physics
