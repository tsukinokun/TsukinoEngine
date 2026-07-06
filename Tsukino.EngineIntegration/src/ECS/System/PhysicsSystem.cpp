//-------------------------------------------------------------
//! @file   PhysicsSystem.cpp
//! @brief  PhysicsSystemクラスの実装
//! @author 山﨑愛
//-------------------------------------------------------------

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Tsukino/EngineIntegration/ECS/System/PhysicsSystem.hpp>

#include <Tsukino/BuiltIn/ECS/Component/CollisionComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/RigidBodyComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/ImpulseRequestComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Event/CollisionEnterEvent.hpp>

#include <Tsukino/Renderer/Renderer.hpp>
#include <Tsukino/EngineIntegration/EngineContext.hpp>
#include <Tsukino/GraphicsCommon/Vertex/DebugVertex.hpp>

#include <Tsukino/Core/ECS/Event/EventBus.hpp>
#include <Tsukino/Core/Log.hpp>

#include <hlsl++.h>
#include <entt/entt.hpp>
#include <Jolt/Jolt.h>
#include <Jolt/Physics/Collision/CollideShape.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/CollisionCollectorImpl.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Geometry/AABox.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>
#include <Jolt/Renderer/DebugRendererSimple.h>
#include <Jolt/Physics/Collision/Shape/HeightFieldShape.h>

#include <windows.h>
// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {

    //-------------------------------------------------------------
    //! @namespace Layers
    //! @brief     オブジェクトレイヤーの定義
    //-------------------------------------------------------------
    namespace Layers {
        static constexpr JPH::ObjectLayer NON_MOVING = 0;    //!< 静的オブジェクトレイヤー
        static constexpr JPH::ObjectLayer MOVING     = 1;    //!< 動的オブジェクトレイヤー
        static constexpr JPH::ObjectLayer NUM_LAYERS = 2;    //!< オブジェクトレイヤー数
    }

    //-------------------------------------------------------------
    //! @namespace BroadPhaseLayers
    //! @brief     ブロードフェーズレイヤーの定義
    //-------------------------------------------------------------
    namespace BroadPhaseLayers {
        static constexpr JPH::BroadPhaseLayer NON_MOVING(0);    //!< 静的ブロードフェーズレイヤー
        static constexpr JPH::BroadPhaseLayer MOVING(1);        //!< 動的ブロードフェーズレイヤー
        static constexpr uint32_t             NUM_LAYERS(2);    //!< ブロードフェーズレイヤー数
    }

    //-------------------------------------------------------------
    //! @class  BPLayerInterfaceImpl
    //! @brief  オブジェクトレイヤーとブロードフェーズレイヤーの対応を定義する連携インターフェース
    //-------------------------------------------------------------
    class BPLayerInterfaceImpl final : public JPH::BroadPhaseLayerInterface {
    public:
        //-------------------------------------------------------------
        //! @brief コンストラクタ
        //-------------------------------------------------------------
        BPLayerInterfaceImpl() {
            m_objectToBroadPhase[Layers::NON_MOVING] = BroadPhaseLayers::NON_MOVING;
            m_objectToBroadPhase[Layers::MOVING]     = BroadPhaseLayers::MOVING;
        }

        //-------------------------------------------------------------
        //! @brief  ブロードフェーズレイヤー数を取得する
        //! @return ブロードフェーズレイヤーの数
        //-------------------------------------------------------------
        uint32_t GetNumBroadPhaseLayers() const override { return BroadPhaseLayers::NUM_LAYERS; }

        //-------------------------------------------------------------
        //! @brief  指定したオブジェクトレイヤーに対応するブロードフェーズレイヤーを取得する
        //! @param  inLayer [in] オブジェクトレイヤー
        //! @return 対応するブロードフェーズレイヤー
        //-------------------------------------------------------------
        JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const override {
            JPH_ASSERT(inLayer < Layers::NUM_LAYERS);
            return m_objectToBroadPhase[inLayer];
        }

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
        const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer inLayer) const override {
            switch((JPH::BroadPhaseLayer::Type)inLayer) {
            case(JPH::BroadPhaseLayer::Type)BroadPhaseLayers::NON_MOVING:
                return "NON_MOVING";
            case(JPH::BroadPhaseLayer::Type)BroadPhaseLayers::MOVING:
                return "MOVING";
            default:
                JPH_ASSERT(false);
                return "INVALID";
            }
        }
#endif    // JPH_EXTERNAL_PROFILE || JPH_PROFILE_ENABLED

    private:
        JPH::BroadPhaseLayer m_objectToBroadPhase[Layers::NUM_LAYERS];
    };

    //-------------------------------------------------------------
    //! @class  ObjectVsBroadPhaseLayerFilterImpl
    //! @brief  オブジェクト同士のブロードフェーズレベルでの衝突判定可否を定義するフィルタ
    //-------------------------------------------------------------
    class ObjectVsBroadPhaseLayerFilterImpl : public JPH::ObjectVsBroadPhaseLayerFilter {
    public:
        //-------------------------------------------------------------
        //! @brief  衝突すべきかどうかを判定する
        //! @param  inLayer1 [in] オブジェクトのレイヤー
        //! @param  inLayer2 [in] 相手側のブロードフェーズレイヤー
        //! @return 衝突する場合はtrue
        //-------------------------------------------------------------
        bool ShouldCollide(JPH::ObjectLayer inLayer1, JPH::BroadPhaseLayer inLayer2) const override {
            switch(inLayer1) {
            case Layers::NON_MOVING:
                return inLayer2 == BroadPhaseLayers::MOVING;
            case Layers::MOVING:
                return true;
            default:
                return false;
            }
        }
    };

    //-------------------------------------------------------------
    //! @class  ObjectLayerPairFilterImpl
    //! @brief  オブジェクトレイヤー同士での衝突判定可否を定義するフィルタ
    //-------------------------------------------------------------
    class ObjectLayerPairFilterImpl : public JPH::ObjectLayerPairFilter {
    public:
        //-------------------------------------------------------------
        //! @brief  衝突すべきかどうかを判定する
        //! @param  inObject1 [in] 判定元オブジェクトのレイヤー
        //! @param  inObject2 [in] 対象のオブジェクトレイヤー
        //! @return 衝突する場合はtrue
        //-------------------------------------------------------------
        bool ShouldCollide(JPH::ObjectLayer inObject1, JPH::ObjectLayer inObject2) const override {
            switch(inObject1) {
            case Layers::NON_MOVING:
                return inObject2 == Layers::MOVING;
            case Layers::MOVING:
                return true;
            default:
                return false;
            }
        }
    };

    //-------------------------------------------------------------
    //! @class  MyContactListener
    //! @brief  Jolt Physicsからの衝突イベントを受け取るリスナー
    //-------------------------------------------------------------
    class MyContactListener : public JPH::ContactListener {
    public:
        using Registry                   = Tsukino::ECS::Registry;
        Registry*               registry = nullptr;    //!< コールバック実行用のレジストリ参照
        Tsukino::ECS::EventBus* eventBus = nullptr;
        //-------------------------------------------------------------
        //! @brief  衝突が追加された（当たった）際に呼ばれるコールバック
        //! @param  inBody1    [in] 衝突したボディ1
        //! @param  inBody2    [in] 衝突したボディ2
        //! @param  inManifold [in] マニフォールド情報（接触点等）
        //! @param  ioSettings [in/out] コンタクト設定（摩擦係数等のオーバーライド可能）
        //-------------------------------------------------------------
        void OnContactAdded(const JPH::Body&            inBody1,
                            const JPH::Body&            inBody2,
                            const JPH::ContactManifold& inManifold,
                            JPH::ContactSettings&       ioSettings) override {
            JPH::Vec3 normal = inManifold.mWorldSpaceNormal;
            //-------------------------------------------------------------
            // 衝突時の法線（Body1から見たBody2への法線）
            // Joltの法線は「Body1から衝突点への方向」を指す
            //-------------------------------------------------------------
            uint64_t id1 = inBody1.GetUserData();
            uint64_t id2 = inBody2.GetUserData();

            // イベント発行（衝突の事実を双方向に通知）
            if(eventBus) {
                // Aから見たBへのイベント
                eventBus->Publish(CollisionEnterEvent{
                    (entt::entity)id1, (entt::entity)id2, {normal.GetX(), normal.GetY(), normal.GetZ()}
                });
                // Bから見たAへのイベント
                eventBus->Publish(CollisionEnterEvent{
                    (entt::entity)id2, (entt::entity)id1, {-normal.GetX(), -normal.GetY(), -normal.GetZ()}
                });
            }

            // 2. 反射処理（Bodyそれぞれの計算）
            auto applyReflection = [&](const JPH::Body& b, JPH::Vec3 n) {
                entt::entity e = (entt::entity)b.GetUserData();
                if(!registry->HasComponent<RigidbodyComponent>(e))
                    return;

                auto& rb = registry->GetComponent<RigidbodyComponent>(e);
                if(rb.type == RigidbodyType::Kinematic) {
                    hlslpp::float3 V   = rb.linearVelocity;
                    hlslpp::float3 N   = {n.GetX(), n.GetY(), n.GetZ()};
                    float          dot = hlslpp::dot(V, N);
                    if(dot < 0.0f) {
                        rb.linearVelocity = V - 2.0f * dot * N;
                    }
                }
            };

            applyReflection(inBody1, normal);
            applyReflection(inBody2, -normal);
        }
    };

    //-------------------------------------------------------------
    //! @class  JoltDebugRendererImpl
    //! @brief  Jolt Physics のデバッグ描画を Tsukino::Renderer に中継するクラス
    //-------------------------------------------------------------
    class JoltDebugRendererImpl final : public JPH::DebugRendererSimple {
    public:
        JoltDebugRendererImpl() { JPH::DebugRendererSimple::Initialize(); }

        void DrawLine(JPH::RVec3Arg inFrom, JPH::RVec3Arg inTo, JPH::ColorArg inColor) override {
            if(!m_renderer)
                return;
            Tsukino::GraphicsCommon::DebugVertex v1{
                {inFrom.GetX(), inFrom.GetY(), inFrom.GetZ()},
                {inColor.r / 255.0f, inColor.g / 255.0f, inColor.b / 255.0f, inColor.a / 255.0f}
            };
            Tsukino::GraphicsCommon::DebugVertex v2{
                {inTo.GetX(), inTo.GetY(), inTo.GetZ()},
                {inColor.r / 255.0f, inColor.g / 255.0f, inColor.b / 255.0f, inColor.a / 255.0f}
            };
            m_renderer->DrawDebugLine(v1, v2);
        }

        void DrawTriangle(JPH::RVec3Arg inV1, JPH::RVec3Arg inV2, JPH::RVec3Arg inV3, JPH::ColorArg inColor, ECastShadow inCastShadow) override {
            if(!m_renderer)
                return;
            Tsukino::GraphicsCommon::DebugVertex v1{
                {inV1.GetX(), inV1.GetY(), inV1.GetZ()},
                {inColor.r / 255.0f, inColor.g / 255.0f, inColor.b / 255.0f, inColor.a / 255.0f}
            };
            Tsukino::GraphicsCommon::DebugVertex v2{
                {inV2.GetX(), inV2.GetY(), inV2.GetZ()},
                {inColor.r / 255.0f, inColor.g / 255.0f, inColor.b / 255.0f, inColor.a / 255.0f}
            };
            Tsukino::GraphicsCommon::DebugVertex v3{
                {inV3.GetX(), inV3.GetY(), inV3.GetZ()},
                {inColor.r / 255.0f, inColor.g / 255.0f, inColor.b / 255.0f, inColor.a / 255.0f}
            };
            m_renderer->DrawDebugTriangle(v1, v2, v3);
        }

        virtual void DrawText3D(JPH::RVec3Arg inPosition, const std::string_view& inString, JPH::ColorArg inColor, float inHeight) override {}

        void SetEngineRenderer(Tsukino::Renderer::Renderer* renderer) { m_renderer = renderer; }

    private:
        Tsukino::Renderer::Renderer* m_renderer = nullptr;
    };

    //-------------------------------------------------------------
    //! @struct PhysicsSystem::Impl
    //! @brief  システム実装の隠蔽用構造体
    //-------------------------------------------------------------
    struct PhysicsSystem::Impl {
        JPH::TempAllocatorImpl*                      tempAllocator = nullptr;         //!< 物理計算用一時アロケータ
        JPH::JobSystemThreadPool*                    jobSystem     = nullptr;         //!< 物理シミュレーション用ジョブシステム
        BPLayerInterfaceImpl                         bpLayerInterface;                //!< ブロードフェーズインターフェース
        ObjectVsBroadPhaseLayerFilterImpl            objVsBpFilter;                   //!< オブジェクト対ブロードフェーズ層フィルタ
        ObjectLayerPairFilterImpl                    objPairFilter;                   //!< オブジェクト層間フィルタ
        JPH::PhysicsSystem*                          physicsSystem      = nullptr;    //!< Jolt物理システム本体
        MyContactListener*                           contactListener    = nullptr;    //!< 衝突イベントリスナー
        JoltDebugRendererImpl*                       debugRenderer      = nullptr;    //!< デバッグ描画インターフェース
        bool                                         isDebugDrawEnabled = false;      //!< デバッグ描画が有効か
        bool                                         f5WasDown          = false;      //!< 直前フレームでF5キーが押されていたか
        std::unordered_map<entt::entity, JPH::RVec3> prevPositions;
        // ハイトマップ用キャッシュ（Shape を直接保持して使い回す）
        std::unordered_map<uint64_t, JPH::Ref<JPH::HeightFieldShape>> heightfieldCache;
    };

    //-------------------------------------------------------------
    //! @brief コンストラクタ
    //-------------------------------------------------------------
    PhysicsSystem::PhysicsSystem(Tsukino::ECS::EventBus& eventBus) {
        m_impl = new Impl();

        static bool isJoltInitialized = false;
        if(!isJoltInitialized) {
            JPH::RegisterDefaultAllocator();
            JPH::Factory::sInstance = new JPH::Factory();
            JPH::RegisterTypes();
            isJoltInitialized = true;
        }

        m_impl->tempAllocator = new JPH::TempAllocatorImpl(10 * 1024 * 1024);
        m_impl->jobSystem     = new JPH::JobSystemThreadPool(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, std::thread::hardware_concurrency() - 1);

        m_impl->physicsSystem                 = new JPH::PhysicsSystem();
        const uint32_t cMaxBodies             = 1024;
        const uint32_t cNumBodyMutexes        = 0;
        const uint32_t cMaxBodyPairs          = 1024;
        const uint32_t cMaxContactConstraints = 1024;

        m_impl->physicsSystem->Init(
            cMaxBodies, cNumBodyMutexes, cMaxBodyPairs, cMaxContactConstraints, m_impl->bpLayerInterface, m_impl->objVsBpFilter, m_impl->objPairFilter);

        m_impl->contactListener = new MyContactListener();
        // ContactListenerにイベントバスの参照を渡す
        m_impl->contactListener->eventBus = &eventBus;
        m_impl->physicsSystem->SetContactListener(m_impl->contactListener);

        m_impl->debugRenderer = new JoltDebugRendererImpl();
    }

    //-------------------------------------------------------------
    // デストラクタ
    //-------------------------------------------------------------
    PhysicsSystem::~PhysicsSystem() {
        if(m_impl) {
            delete m_impl->debugRenderer;
            delete m_impl->contactListener;
            delete m_impl->physicsSystem;
            delete m_impl->jobSystem;
            delete m_impl->tempAllocator;
            delete m_impl;
        }
    }

    //-------------------------------------------------------------
    // システムの更新処理
    //-------------------------------------------------------------
    void PhysicsSystem::Update(Tsukino::ECS::Registry& registry, float deltaTime) {
        m_impl->contactListener->registry = &registry;
        JPH::BodyInterface& bodyInterface = m_impl->physicsSystem->GetBodyInterface();

        auto view = registry.View<CollisionComponent>();

        // 1. 生成処理
        for(auto entity : view) {
            auto& col = registry.GetComponent<CollisionComponent>(entity);
            if(!col.isInitialized) {
                JPH::RefConst<JPH::Shape> shape;
                if(col.type == ColliderType::Box)
                    shape = new JPH::BoxShape(JPH::Vec3(col.extent.x, col.extent.y, col.extent.z));
                else if(col.type == ColliderType::Sphere)
                    shape = new JPH::SphereShape(col.extent.x);
                else if(col.type == ColliderType::Capsule)
                    shape = new JPH::CapsuleShape(col.extent.y, col.extent.x);
                else if(col.type == ColliderType::Heightfield) {
                    if(!col.heightfieldSamples.empty() && col.heightfieldSize > 0) {
                        uint64_t cacheKey = (uint64_t)entity;

                        auto cacheIt = m_impl->heightfieldCache.find(cacheKey);
                        if(cacheIt != m_impl->heightfieldCache.end()) {
                            shape = cacheIt->second;
                        } else {
                            // 修正：HeightFieldShapeSettings の第3引数(Scale)と第2引数(Offset)を確実に一致させる
                            // col.heightfieldOffset は地形の左下隅(Min)を指すようにし、
                            // col.heightfieldScale はグリッド1つあたりの間隔(サイズ)を指すようにする

                            JPH::HeightFieldShapeSettings hfSettings(col.heightfieldSamples.data(),
                                                                     JPH::Vec3(col.heightfieldOffset.x, col.heightfieldOffset.y, col.heightfieldOffset.z),
                                                                     JPH::Vec3(col.heightfieldScale.x, col.heightfieldScale.y, col.heightfieldScale.z),
                                                                     col.heightfieldSize);

                            // このフラグが「地形の法線」を正しく計算させます
                            hfSettings.mBlockSize = 2;    // デフォルトの2にする

                            JPH::Shape::ShapeResult hfResult = hfSettings.Create();
                            if(hfResult.IsValid()) {
                                shape                              = hfResult.Get();
                                m_impl->heightfieldCache[cacheKey] = (JPH::HeightFieldShape*)shape.GetPtr();
                            
                                JPH::AABox localBounds = shape->GetLocalBounds();
                                Tsukino::Core::Log::Info("HeightField local bounds min=(" + std::to_string(localBounds.mMin.GetX()) + ","
                                                         + std::to_string(localBounds.mMin.GetZ()) + ") max=(" + std::to_string(localBounds.mMax.GetX()) + ","
                                                         + std::to_string(localBounds.mMax.GetZ()) + ")");
                                Tsukino::Core::Log::Info("Requested offset=(" + std::to_string(col.heightfieldOffset.x) + ","
                                                         + std::to_string(col.heightfieldOffset.z) + ") scale=(" + std::to_string(col.heightfieldScale.x) + ","
                                                         + std::to_string(col.heightfieldScale.z) + ") size=" + std::to_string(col.heightfieldSize));
                            } else {
                                OutputDebugStringA(hfResult.GetError().c_str());
                            }
                        }
                    }
                }

                if(shape) {
                    JPH::EMotionType motionType = JPH::EMotionType::Static;
                    JPH::ObjectLayer layer      = Layers::NON_MOVING;

                    if(registry.HasComponent<RigidbodyComponent>(entity)) {
                        auto& rb = registry.GetComponent<RigidbodyComponent>(entity);
                        if(rb.type == RigidbodyType::Dynamic) {
                            motionType = JPH::EMotionType::Dynamic;
                            layer      = Layers::MOVING;
                        } else if(rb.type == RigidbodyType::Kinematic) {
                            motionType = JPH::EMotionType::Kinematic;
                            layer      = Layers::MOVING;
                        }
                    }

                    // Transformとオフセットから物理用の初期位置を計算
                    JPH::RVec3 pos(0, 0, 0);
                    JPH::Quat  rot = JPH::Quat::sIdentity();
                    if(registry.HasComponent<TransformComponent>(entity)) {
                        auto& tf = registry.GetComponent<TransformComponent>(entity);
                        pos      = JPH::RVec3(tf.position.x, tf.position.y, tf.position.z);
                        rot      = JPH::Quat(tf.rotation.x, tf.rotation.y, tf.rotation.z, tf.rotation.w);
                    }

                    JPH::Quat offsetRot(col.offsetRotation.x, col.offsetRotation.y, col.offsetRotation.z, col.offsetRotation.w);
                    JPH::Vec3 localOffset(col.offsetPosition.x, col.offsetPosition.y, col.offsetPosition.z);

                    JPH::RVec3 finalPos = pos + (rot * localOffset);
                    JPH::Quat  finalRot = rot * offsetRot;

                    JPH::BodyCreationSettings settings(shape, finalPos, finalRot, motionType, layer);
                    settings.mIsSensor = col.isSensor;

                    if(registry.HasComponent<RigidbodyComponent>(entity)) {
                        auto& rb                               = registry.GetComponent<RigidbodyComponent>(entity);
                        settings.mFriction                     = rb.friction;
                        settings.mRestitution                  = rb.restitution;
                        settings.mGravityFactor                = rb.gravityFactor;
                        settings.mOverrideMassProperties       = JPH::EOverrideMassProperties::CalculateInertia;
                        settings.mMassPropertiesOverride.mMass = rb.mass;

                        JPH::EAllowedDOFs dofs = JPH::EAllowedDOFs::All;
                        if(rb.freezePositionX)
                            dofs &= ~JPH::EAllowedDOFs::TranslationX;
                        if(rb.freezePositionY)
                            dofs &= ~JPH::EAllowedDOFs::TranslationY;
                        if(rb.freezePositionZ)
                            dofs &= ~JPH::EAllowedDOFs::TranslationZ;
                        if(rb.freezeRotationX)
                            dofs &= ~JPH::EAllowedDOFs::RotationX;
                        if(rb.freezeRotationY)
                            dofs &= ~JPH::EAllowedDOFs::RotationY;
                        if(rb.freezeRotationZ)
                            dofs &= ~JPH::EAllowedDOFs::RotationZ;

                        settings.mAllowedDOFs = dofs;
                    }

                    JPH::Body* body = bodyInterface.CreateBody(settings);
                    if(body) {
                        body->SetUserData((uint64_t)entity);
                        col.bodyID = body->GetID();
                        bodyInterface.AddBody(col.bodyID, JPH::EActivation::Activate);
                        col.isInitialized = true;
                    }
                }
            }
        }

        float stepTime = deltaTime > 0.0f ? deltaTime : 1.0f / 60.0f;

        for(auto entity : view) {
            auto& col = registry.GetComponent<CollisionComponent>(entity);
            if(!registry.HasComponent<RigidbodyComponent>(entity))
                continue;
            auto& rb = registry.GetComponent<RigidbodyComponent>(entity);
            if(col.isInitialized && rb.type == RigidbodyType::Kinematic && registry.HasComponent<TransformComponent>(entity)) {
                auto&      tf = registry.GetComponent<TransformComponent>(entity);
                JPH::RVec3 pos(tf.position.x, tf.position.y, tf.position.z);
                JPH::Quat  rot(tf.rotation.x, tf.rotation.y, tf.rotation.z, tf.rotation.w);
                JPH::Quat  offsetRot(col.offsetRotation.x, col.offsetRotation.y, col.offsetRotation.z, col.offsetRotation.w);

                bodyInterface.SetPositionAndRotation(col.bodyID,
                                                     pos + (rot * JPH::Vec3(col.offsetPosition.x, col.offsetPosition.y, col.offsetPosition.z)),
                                                     rot * offsetRot,
                                                     JPH::EActivation::Activate);

                auto it = m_impl->prevPositions.find(entity);
                if(it != m_impl->prevPositions.end()) {
                    JPH::Vec3 velocity = (pos - it->second) / stepTime;
                    bodyInterface.SetLinearVelocity(col.bodyID, velocity);
                }
                m_impl->prevPositions[entity] = pos;
            }
        }

        std::vector<entt::entity> entitiesToRemoveImpulse;

        auto requestView = registry.View<ImpulseRequestComponent, CollisionComponent>();
        requestView.each([&](auto entity, auto& ir, auto& col) {
            if(col.isInitialized) {
                JPH::BodyInterface& bi = m_impl->physicsSystem->GetBodyInterface();
                bi.AddImpulse(col.bodyID, JPH::Vec3(ir.impulse.x, ir.impulse.y, ir.impulse.z));
            
                // 回転（トルク）の付与
                JPH::Vec3 angImpulse(ir.angularImpulse.x, ir.angularImpulse.y, ir.angularImpulse.z);
                if(!angImpulse.IsNearZero()) {
                    bi.AddAngularImpulse(col.bodyID, angImpulse);
                }
            }
            entitiesToRemoveImpulse.push_back(entity);
        });
        // 一括で削除（遅延削除によって、イテレータ走査中にリムーブしない）
        for(auto entity : entitiesToRemoveImpulse) {
            registry.RemoveComponent<ImpulseRequestComponent>(entity);
        }

        // MotionTypeの変更
        for(auto entity : view) {
            if(!registry.HasComponent<RigidbodyComponent>(entity))
                continue;
            auto& rb  = registry.GetComponent<RigidbodyComponent>(entity);
            auto& col = registry.GetComponent<CollisionComponent>(entity);
            if(!col.isInitialized)
                continue;

            if(!rb.isTypeDirty)
                continue;

            JPH::EMotionType currentJoltType = bodyInterface.GetMotionType(col.bodyID);
            JPH::EMotionType targetType;

            switch(rb.type) {
            case RigidbodyType::Static:
                targetType = JPH::EMotionType::Static;
                break;
            case RigidbodyType::Kinematic:
                targetType = JPH::EMotionType::Kinematic;
                break;
            case RigidbodyType::Dynamic:
                targetType = JPH::EMotionType::Dynamic;
                break;
            default:
                continue;
            }

            if(currentJoltType != targetType) {
                bodyInterface.SetMotionType(col.bodyID, targetType, JPH::EActivation::Activate);
            }
        }

        //-------------------------------------------------------------
        // Rigidbodyのforce/torqueを反映
        //-------------------------------------------------------------
        for(auto entity : view) {
            auto& col = registry.GetComponent<CollisionComponent>(entity);
            if(!col.isInitialized || !registry.HasComponent<RigidbodyComponent>(entity))
                continue;

            auto& rb = registry.GetComponent<RigidbodyComponent>(entity);
            if(rb.type != RigidbodyType::Dynamic)
                continue;

            bool hasForce  = !(rb.force.x == 0.0f && rb.force.y == 0.0f && rb.force.z == 0.0f);
            bool hasTorque = !(rb.torque.x == 0.0f && rb.torque.y == 0.0f && rb.torque.z == 0.0f);

            if(hasForce) {
                bodyInterface.AddForce(col.bodyID, JPH::Vec3(rb.force.x, rb.force.y, rb.force.z));
            }
            if(hasTorque) {
                bodyInterface.AddTorque(col.bodyID, JPH::Vec3(rb.torque.x, rb.torque.y, rb.torque.z));
            }
        }

        // 3. 物理シミュレーション実行
        m_impl->physicsSystem->Update(stepTime, 1, m_impl->tempAllocator, m_impl->jobSystem);

        // 4. Dynamic同期
        for(auto entity : view) {
            auto& col = registry.GetComponent<CollisionComponent>(entity);
            if(col.isInitialized && registry.HasComponent<RigidbodyComponent>(entity)) {
                auto& rb = registry.GetComponent<RigidbodyComponent>(entity);
                if(rb.type == RigidbodyType::Dynamic && registry.HasComponent<TransformComponent>(entity)) {
                    auto&      tf      = registry.GetComponent<TransformComponent>(entity);
                    JPH::RVec3 bodyPos = bodyInterface.GetPosition(col.bodyID);
                    JPH::Quat  bodyRot = bodyInterface.GetRotation(col.bodyID);

                    tf.position = hlslpp::float3(bodyPos.GetX(), bodyPos.GetY(), bodyPos.GetZ());
                    tf.rotation = hlslpp::quaternion(bodyRot.GetX(), bodyRot.GetY(), bodyRot.GetZ(), bodyRot.GetW());
                    tf.dirty    = true;

                    JPH::BoxShape                                             checkShape(JPH::Vec3(rb.groundCheckRadius, 0.05f, rb.groundCheckRadius));
                    JPH::RVec3                                                checkPos(tf.position.x, tf.position.y - rb.groundCheckDistance, tf.position.z);
                    JPH::IgnoreSingleBodyFilter                               bodyFilter(col.bodyID);
                    JPH::AllHitCollisionCollector<JPH::CollideShapeCollector> collector;
                    m_impl->physicsSystem->GetNarrowPhaseQuery().CollideShape(&checkShape,
                                                                              JPH::Vec3::sReplicate(1.0f),
                                                                              JPH::RMat44::sTranslation(checkPos),
                                                                              JPH::CollideShapeSettings(),
                                                                              JPH::RVec3::sZero(),
                                                                              collector,
                                                                              {},
                                                                              {},
                                                                              bodyFilter);
                    rb.isGrounded = collector.HadHit();
                }
            }
        }

        // デバッグ描画
        bool f5IsDown = (::GetAsyncKeyState(VK_F5) & 0x8000) != 0;
        if(f5IsDown && !m_impl->f5WasDown) {
            m_impl->isDebugDrawEnabled = !m_impl->isDebugDrawEnabled;
        }
        m_impl->f5WasDown = f5IsDown;

        if(m_impl->isDebugDrawEnabled) {
            auto* ctx = registry.GetContext<Tsukino::EngineIntegration::EngineContext*>();
            if(ctx && ctx->renderer) {
                m_impl->debugRenderer->SetEngineRenderer(ctx->renderer);

                // DrawBodiesの代わりに自前でECSのviewから全ボディを描画
                for(auto entity : view) {
                    auto& col = registry.GetComponent<CollisionComponent>(entity);
                    if(!col.isInitialized)
                        continue;

                    JPH::BodyLockRead lock(m_impl->physicsSystem->GetBodyLockInterface(), col.bodyID);
                    if(!lock.Succeeded())
                        continue;

                    const JPH::Body& body      = lock.GetBody();
                    JPH::RMat44      transform = body.GetCenterOfMassTransform();

                    JPH::BodyManager::DrawSettings ds;
                    ds.mDrawShape          = true;
                    ds.mDrawShapeWireframe = true;

                    body.GetShape()->Draw(m_impl->debugRenderer, transform, JPH::Vec3::sReplicate(1.0f), JPH::Color::sGreen, false, false);
                }

                // isGrounded判定Box描画
                for(auto entity : view) {
                    if(!registry.HasComponent<RigidbodyComponent>(entity))
                        continue;
                    if(!registry.HasComponent<TransformComponent>(entity))
                        continue;
                    auto& rb = registry.GetComponent<RigidbodyComponent>(entity);
                    if(rb.type != RigidbodyType::Dynamic)
                        continue;

                    auto&         tf    = registry.GetComponent<TransformComponent>(entity);
                    float         r     = rb.groundCheckRadius;
                    float         thick = 0.05f;
                    float         cx    = tf.position.x;
                    float         cy    = tf.position.y - rb.groundCheckDistance;
                    float         cz    = tf.position.z;
                    JPH::ColorArg color = rb.isGrounded ? JPH::Color::sGreen : JPH::Color::sRed;

                    // 上面
                    m_impl->debugRenderer->DrawLine({cx - r, cy + thick, cz - r}, {cx + r, cy + thick, cz - r}, color);
                    m_impl->debugRenderer->DrawLine({cx + r, cy + thick, cz - r}, {cx + r, cy + thick, cz + r}, color);
                    m_impl->debugRenderer->DrawLine({cx + r, cy + thick, cz + r}, {cx - r, cy + thick, cz + r}, color);
                    m_impl->debugRenderer->DrawLine({cx - r, cy + thick, cz + r}, {cx - r, cy + thick, cz - r}, color);
                    // 下面
                    m_impl->debugRenderer->DrawLine({cx - r, cy - thick, cz - r}, {cx + r, cy - thick, cz - r}, color);
                    m_impl->debugRenderer->DrawLine({cx + r, cy - thick, cz - r}, {cx + r, cy - thick, cz + r}, color);
                    m_impl->debugRenderer->DrawLine({cx + r, cy - thick, cz + r}, {cx - r, cy - thick, cz + r}, color);
                    m_impl->debugRenderer->DrawLine({cx - r, cy - thick, cz + r}, {cx - r, cy - thick, cz - r}, color);
                    // 縦辺
                    m_impl->debugRenderer->DrawLine({cx - r, cy + thick, cz - r}, {cx - r, cy - thick, cz - r}, color);
                    m_impl->debugRenderer->DrawLine({cx + r, cy + thick, cz - r}, {cx + r, cy - thick, cz - r}, color);
                    m_impl->debugRenderer->DrawLine({cx + r, cy + thick, cz + r}, {cx + r, cy - thick, cz + r}, color);
                    m_impl->debugRenderer->DrawLine({cx - r, cy + thick, cz + r}, {cx - r, cy - thick, cz + r}, color);
                }

                Tsukino::Renderer::DrawCommand cmd{};
                cmd.customDraw = [renderer = ctx->renderer](ID3D11DeviceContext* context) { renderer->FlushDebugDraw(); };
                ctx->renderer->PushDrawCommand(cmd);
            }
        }
    }

}    // namespace Tsukino::BuiltIn::ECS
