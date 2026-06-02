//-------------------------------------------------------------
//! @file   PhysicsSystem.cpp
//! @brief  PhysicsSystemクラスの実装
//! @author 山﨑愛
//-------------------------------------------------------------

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <hlsl++.h>
#include <entt/entt.hpp>
#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>
#include <Jolt/Renderer/DebugRendererSimple.h>
#include <Tsukino/EngineIntegration/ECS/System/PhysicsSystem.hpp>
#include <Tsukino/BuiltIn/ECS/Component/CollisionComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/RigidBodyComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/ImpulseRequestComponent.hpp>

#include <Tsukino/Renderer/Renderer.hpp>
#include <Tsukino/EngineIntegration/EngineContext.hpp>
#include <Tsukino/GraphicsCommon/Vertex/DebugVertex.hpp>

// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {

    //-------------------------------------------------------------
    //! @namespace Layers
    //! @brief     オブジェクトレイヤーの定義
    //-------------------------------------------------------------
    namespace Layers {
        static constexpr JPH::ObjectLayer NON_MOVING = 0; //!< 静的オブジェクトレイヤー
        static constexpr JPH::ObjectLayer MOVING = 1;     //!< 動的オブジェクトレイヤー
        static constexpr JPH::ObjectLayer NUM_LAYERS = 2; //!< オブジェクトレイヤー数
    }

    //-------------------------------------------------------------
    //! @namespace BroadPhaseLayers
    //! @brief     ブロードフェーズレイヤーの定義
    //-------------------------------------------------------------
    namespace BroadPhaseLayers {
        static constexpr JPH::BroadPhaseLayer NON_MOVING(0); //!< 静的ブロードフェーズレイヤー
        static constexpr JPH::BroadPhaseLayer MOVING(1);     //!< 動的ブロードフェーズレイヤー
        static constexpr uint32_t NUM_LAYERS(2);             //!< ブロードフェーズレイヤー数
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
            m_objectToBroadPhase[Layers::MOVING] = BroadPhaseLayers::MOVING;
        }

        //-------------------------------------------------------------
        //! @brief  ブロードフェーズレイヤー数を取得する
        //! @return ブロードフェーズレイヤーの数
        //-------------------------------------------------------------
        uint32_t GetNumBroadPhaseLayers() const override {
            return BroadPhaseLayers::NUM_LAYERS;
        }

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
            switch ((JPH::BroadPhaseLayer::Type)inLayer) {
                case (JPH::BroadPhaseLayer::Type)BroadPhaseLayers::NON_MOVING: return "NON_MOVING";
                case (JPH::BroadPhaseLayer::Type)BroadPhaseLayers::MOVING: return "MOVING";
                default: JPH_ASSERT(false); return "INVALID";
            }
        }
#endif // JPH_EXTERNAL_PROFILE || JPH_PROFILE_ENABLED

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
            switch (inLayer1) {
                case Layers::NON_MOVING: return inLayer2 == BroadPhaseLayers::MOVING;
                case Layers::MOVING: return true;
                default: return false;
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
            switch (inObject1) {
                case Layers::NON_MOVING: return inObject2 == Layers::MOVING;
                case Layers::MOVING: return true;
                default: return false;
            }
        }
    };

    //-------------------------------------------------------------
    //! @class  MyContactListener
    //! @brief  Jolt Physicsからの衝突イベントを受け取るリスナー
    //-------------------------------------------------------------
    class MyContactListener : public JPH::ContactListener {
    public:
        using Registry = Tsukino::ECS::Registry;
        Registry* registry = nullptr; //!< コールバック実行用のレジストリ参照

        //-------------------------------------------------------------
        //! @brief  衝突が追加された（当たった）際に呼ばれるコールバック
        //! @param  inBody1    [in] 衝突したボディ1
        //! @param  inBody2    [in] 衝突したボディ2
        //! @param  inManifold [in] マニフォールド情報（接触点等）
        //! @param  ioSettings [in/out] コンタクト設定（摩擦係数等のオーバーライド可能）
        //-------------------------------------------------------------
        void OnContactAdded(const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings) override {
            if(!registry)
                return;

            // 衝突時の法線（Body1から見たBody2への法線）
            // Joltの法線は「Body1から衝突点への方向」を指す
            JPH::Vec3 normal = inManifold.mWorldSpaceNormal;

            auto handleReflection = [&](const JPH::Body& self, const JPH::Body& other, JPH::Vec3 n) {
                uint64_t selfId = self.GetUserData();
                if(selfId == 0)
                    return;

                entt::entity entity = (entt::entity)selfId;

                // 1. RigidbodyComponentがあれば反射処理を行う
                if(registry->HasComponent<RigidbodyComponent>(entity)) {
                    auto& rb = registry->GetComponent<RigidbodyComponent>(entity);

                    // Kinematic（ボール等）のみ反射を計算
                    if(rb.type == RigidbodyType::Kinematic) {
                        hlslpp::float3 V = rb.linearVelocity;
                        hlslpp::float3 N = {n.GetX(), n.GetY(), n.GetZ()};

                        // 内積を計算 (速度ベクトルと法線が向き合っているか)
                        float dot = hlslpp::dot(V, N);

                        // dot < 0 の場合、物体は壁に向かっている
                        if(dot < 0.0f) {
                            // 反射の公式: V' = V - 2 * (V・N) * N
                            rb.linearVelocity = V - 2.0f * dot * N;
                        }
                    }
                }

                // 2. 既存の通知処理（CollisionComponent）
                if(registry->HasComponent<CollisionComponent>(entity)) {
                    auto& col = registry->GetComponent<CollisionComponent>(entity);
                    if(col.onCollisionEnter) {
                        col.onCollisionEnter((entt::entity)other.GetUserData());
                    }
                }
            };

            // Body1に対する処理 (法線はそのままでOK)
            handleReflection(inBody1, inBody2, normal);

            // Body2に対する処理 (法線は逆向きにする必要がある)
            handleReflection(inBody2, inBody1, -normal);
        }
    };

    //-------------------------------------------------------------
    //! @class  JoltDebugRendererImpl
    //! @brief  Jolt Physics のデバッグ描画を Tsukino::Renderer に中継するクラス
    //-------------------------------------------------------------
    class JoltDebugRendererImpl final : public JPH::DebugRendererSimple {
    public:
        JoltDebugRendererImpl() {
            JPH::DebugRendererSimple::Initialize();
        }

        void DrawLine(JPH::RVec3Arg inFrom, JPH::RVec3Arg inTo, JPH::ColorArg inColor) override {
            if (!m_renderer) return;
            Tsukino::GraphicsCommon::DebugVertex v1 {
                { inFrom.GetX(), inFrom.GetY(), inFrom.GetZ() },
                { inColor.r / 255.0f, inColor.g / 255.0f, inColor.b / 255.0f, inColor.a / 255.0f }
            };
            Tsukino::GraphicsCommon::DebugVertex v2 {
                { inTo.GetX(), inTo.GetY(), inTo.GetZ() },
                { inColor.r / 255.0f, inColor.g / 255.0f, inColor.b / 255.0f, inColor.a / 255.0f }
            };
            m_renderer->DrawDebugLine(v1, v2);
        }

        void DrawTriangle(JPH::RVec3Arg inV1, JPH::RVec3Arg inV2, JPH::RVec3Arg inV3, JPH::ColorArg inColor, ECastShadow inCastShadow) override {
            if (!m_renderer) return;
            Tsukino::GraphicsCommon::DebugVertex v1 { { inV1.GetX(), inV1.GetY(), inV1.GetZ() }, { inColor.r / 255.0f, inColor.g / 255.0f, inColor.b / 255.0f, inColor.a / 255.0f } };
            Tsukino::GraphicsCommon::DebugVertex v2 { { inV2.GetX(), inV2.GetY(), inV2.GetZ() }, { inColor.r / 255.0f, inColor.g / 255.0f, inColor.b / 255.0f, inColor.a / 255.0f } };
            Tsukino::GraphicsCommon::DebugVertex v3 { { inV3.GetX(), inV3.GetY(), inV3.GetZ() }, { inColor.r / 255.0f, inColor.g / 255.0f, inColor.b / 255.0f, inColor.a / 255.0f } };
            m_renderer->DrawDebugTriangle(v1, v2, v3);
        }

        virtual void DrawText3D(JPH::RVec3Arg inPosition, const std::string_view &inString, JPH::ColorArg inColor, float inHeight) override {}

        void SetEngineRenderer(Tsukino::Renderer::Renderer* renderer) {
            m_renderer = renderer;
        }

    private:
        Tsukino::Renderer::Renderer* m_renderer = nullptr;
    };

    //-------------------------------------------------------------
    //! @struct PhysicsSystem::Impl
    //! @brief  システム実装の隠蔽用構造体
    //-------------------------------------------------------------
    struct PhysicsSystem::Impl {
        JPH::TempAllocatorImpl* tempAllocator = nullptr;   //!< 物理計算用一時アロケータ
        JPH::JobSystemThreadPool* jobSystem = nullptr;     //!< 物理シミュレーション用ジョブシステム
        BPLayerInterfaceImpl bpLayerInterface;             //!< ブロードフェーズインターフェース
        ObjectVsBroadPhaseLayerFilterImpl objVsBpFilter;   //!< オブジェクト対ブロードフェーズ層フィルタ
        ObjectLayerPairFilterImpl objPairFilter;           //!< オブジェクト層間フィルタ
        JPH::PhysicsSystem* physicsSystem = nullptr;       //!< Jolt物理システム本体
        MyContactListener* contactListener = nullptr;      //!< 衝突イベントリスナー
        JoltDebugRendererImpl* debugRenderer = nullptr;    //!< デバッグ描画インターフェース
        bool isDebugDrawEnabled = false;                   //!< デバッグ描画が有効か
        bool f5WasDown = false;                            //!< 直前フレームでF5キーが押されていたか
    };

    //-------------------------------------------------------------
    // デフォルトコンストラクタ
    //-------------------------------------------------------------
    PhysicsSystem::PhysicsSystem() {
        m_impl = new Impl();
        
        static bool isJoltInitialized = false;
        if (!isJoltInitialized) {
            JPH::RegisterDefaultAllocator();
            JPH::Factory::sInstance = new JPH::Factory();
            JPH::RegisterTypes();
            isJoltInitialized = true;
        }

        m_impl->tempAllocator = new JPH::TempAllocatorImpl(10 * 1024 * 1024);
        m_impl->jobSystem = new JPH::JobSystemThreadPool(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, std::thread::hardware_concurrency() - 1);

        m_impl->physicsSystem = new JPH::PhysicsSystem();
        const uint32_t cMaxBodies = 1024;
        const uint32_t cNumBodyMutexes = 0;
        const uint32_t cMaxBodyPairs = 1024;
        const uint32_t cMaxContactConstraints = 1024;

        m_impl->physicsSystem->Init(cMaxBodies, cNumBodyMutexes, cMaxBodyPairs, cMaxContactConstraints,
            m_impl->bpLayerInterface, m_impl->objVsBpFilter, m_impl->objPairFilter);

        m_impl->contactListener = new MyContactListener();
        m_impl->physicsSystem->SetContactListener(m_impl->contactListener);

        m_impl->debugRenderer = new JoltDebugRendererImpl();
    }

    //-------------------------------------------------------------
    // デストラクタ
    //-------------------------------------------------------------
    PhysicsSystem::~PhysicsSystem() {
        if (m_impl) {
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
        auto                view          = registry.View<CollisionComponent>();
        JPH::BodyInterface& bodyInterface = m_impl->physicsSystem->GetBodyInterface();

        for(auto entity : view) {
            auto& col = registry.GetComponent<CollisionComponent>(entity);

            // ボディの新規生成
            if(!col.isInitialized) {
                JPH::RefConst<JPH::Shape> shape;
                // (Shape生成ロジックは現状維持)
                if(col.type == ColliderType::Box)
                    shape = new JPH::BoxShape(JPH::Vec3(col.extent.x, col.extent.y, col.extent.z));
                else if(col.type == ColliderType::Sphere)
                    shape = new JPH::SphereShape(col.extent.x);
                else if(col.type == ColliderType::Capsule)
                    shape = new JPH::CapsuleShape(col.extent.y, col.extent.x);

                if(shape) {
                    // デフォルトはStatic設定
                    JPH::EMotionType motionType = JPH::EMotionType::Static;
                    JPH::ObjectLayer layer      = Layers::NON_MOVING;

                    // Rigidbodyがある場合はその設定を反映
                    if(registry.HasComponent<RigidbodyComponent>(entity)) {
                        auto& rb = registry.GetComponent<RigidbodyComponent>(entity);
                        switch(rb.type) {
                        case RigidbodyType::Dynamic:
                            motionType = JPH::EMotionType::Dynamic;
                            layer      = Layers::MOVING;
                            break;
                        case RigidbodyType::Kinematic:
                            motionType = JPH::EMotionType::Kinematic;
                            layer      = Layers::MOVING;
                            break;
                        case RigidbodyType::Static:
                            motionType = JPH::EMotionType::Static;
                            layer      = Layers::NON_MOVING;
                            break;
                        }
                    }

                    // Transformの取得
                    JPH::RVec3 position(0, 0, 0);
                    JPH::Quat  rotation = JPH::Quat::sIdentity();
                    if(registry.HasComponent<TransformComponent>(entity)) {
                        auto& transform = registry.GetComponent<TransformComponent>(entity);
                        position        = JPH::RVec3(transform.position.x, transform.position.y, transform.position.z);
                        rotation        = JPH::Quat(transform.rotation.x, transform.rotation.y, transform.rotation.z, transform.rotation.w);
                    }

                    JPH::BodyCreationSettings settings(shape, position, rotation, motionType, layer);
                    settings.mIsSensor = col.isSensor;

                    // Rigidbodyのパラメータ適用
                    if(registry.HasComponent<RigidbodyComponent>(entity)) {
                        auto& rb              = registry.GetComponent<RigidbodyComponent>(entity);
                        settings.mFriction    = rb.friction;
                        settings.mRestitution = rb.restitution;
                        settings.mGravityFactor  = rb.gravityFactor;    // 重力の影響度 (0.0f で無重力)
                        settings.mLinearDamping  = 0.0f;                // 空気抵抗による減衰を無効化
                        settings.mAngularDamping = 0.0f;                // 回転の減衰を無効化
                        settings.mOverrideMassProperties       = JPH::EOverrideMassProperties::CalculateInertia;
                        settings.mMassPropertiesOverride.mMass = rb.mass;
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

            // Kinematicの同期 (Transform -> Jolt) ---
            // 物理シミュレーションの前に、プログラム側で動いた座標をJoltに伝える
            if(col.isInitialized && registry.HasComponent<RigidbodyComponent>(entity)) {
                auto& rb = registry.GetComponent<RigidbodyComponent>(entity);
                if(rb.type == RigidbodyType::Kinematic && registry.HasComponent<TransformComponent>(entity)) {
                    auto&      tf = registry.GetComponent<TransformComponent>(entity);
                    JPH::RVec3 p(tf.position.x, tf.position.y, tf.position.z);
                    JPH::Quat  r(tf.rotation.x, tf.rotation.y, tf.rotation.z, tf.rotation.w);
                    // Kinematic移動としてJolt側に通知（これにより周囲のDynamicが押し出される）
                    bodyInterface.SetPositionAndRotation(col.bodyID, p, r, JPH::EActivation::Activate);
                }
            }
        }

        // 外部システムから投げられた ImpulseRequestComponent を処理する
        auto impulseView = registry.View<CollisionComponent, ImpulseRequestComponent>();
        
        // 削除対象を一時的に保持するリスト
        std::vector<entt::entity> entitiesToRemove;

        impulseView.each([&](auto entity, const auto& col, const auto& req) {
            // 物理エンジンへの反映
            bodyInterface.AddImpulse(col.bodyID, JPH::Vec3(req.impulse.x, req.impulse.y, req.impulse.z));

            // 削除対象を貯める
            entitiesToRemove.push_back(entity);
        });

        // 反映し終えたリクエストを削除
        for(auto entity : entitiesToRemove) {
            registry.RemoveComponent<ImpulseRequestComponent>(entity);
        }

        //  物理シミュレーション実行 ---
        float stepTime = deltaTime > 0.0f ? deltaTime : 1.0f / 60.0f;
        m_impl->physicsSystem->Update(stepTime, 1, m_impl->tempAllocator, m_impl->jobSystem);

        //  Dynamicの同期 (Jolt -> Transform) ---
        for(auto entity : view) {
            auto& col = registry.GetComponent<CollisionComponent>(entity);
            if(col.isInitialized && registry.HasComponent<RigidbodyComponent>(entity)) {
                auto& rb = registry.GetComponent<RigidbodyComponent>(entity);
                // Dynamicのみ、物理演算の結果をTransformに書き戻す
                if(rb.type == RigidbodyType::Dynamic && registry.HasComponent<TransformComponent>(entity)) {
                    auto& tf = registry.GetComponent<TransformComponent>(entity);
                    if(bodyInterface.IsActive(col.bodyID)) {
                        JPH::RVec3 pos = bodyInterface.GetPosition(col.bodyID);
                        JPH::Quat  rot = bodyInterface.GetRotation(col.bodyID);
                        tf.position    = hlslpp::float3(pos.GetX(), pos.GetY(), pos.GetZ());
                        tf.rotation    = hlslpp::quaternion(rot.GetX(), rot.GetY(), rot.GetZ(), rot.GetW());
                        tf.dirty       = true;
                    }
                }
            }
        }

        bool f5IsDown = (::GetAsyncKeyState(VK_F5) & 0x8000) != 0;
        if (f5IsDown && !m_impl->f5WasDown) {
            m_impl->isDebugDrawEnabled = !m_impl->isDebugDrawEnabled;
        }
        m_impl->f5WasDown = f5IsDown;

        if (m_impl->isDebugDrawEnabled) {
            auto* ctx = registry.GetContext<Tsukino::EngineIntegration::EngineContext*>();
            if (ctx && ctx->renderer) {
                m_impl->debugRenderer->SetEngineRenderer(ctx->renderer);
                JPH::BodyManager::DrawSettings drawSettings;
                drawSettings.mDrawShape = true;
                drawSettings.mDrawBoundingBox = false;

                m_impl->physicsSystem->DrawBodies(drawSettings, m_impl->debugRenderer);
                m_impl->physicsSystem->DrawConstraints(m_impl->debugRenderer);

                // 発行されたデバッグ頂点を描画するコマンドをキューに送る
                Tsukino::Renderer::DrawCommand cmd{};
                cmd.pass = Tsukino::Renderer::RenderPass::World;
                cmd.customDraw = [renderer = ctx->renderer](ID3D11DeviceContext* context) {
                    renderer->FlushDebugDraw();
                };
                ctx->renderer->PushDrawCommand(cmd);
            }
        }
    }

}    // namespace Tsukino::BuiltIn::ECS
