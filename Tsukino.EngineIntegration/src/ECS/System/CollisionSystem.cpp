//-------------------------------------------------------------
//! @file   CollisionSystem.cpp
//! @brief  CollisionSystemクラスの実装
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
#include <Tsukino/EngineIntegration/ECS/System/CollisionSystem.hpp>
#include <Tsukino/BuiltIn/ECS/Component/CollisionComponent.hpp>
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
            if (!registry) return;

            auto callEnterEvent = [&](const JPH::Body& b1, const JPH::Body& b2) {
                uint64_t entityId1 = b1.GetUserData();
                uint64_t entityId2 = b2.GetUserData();

                if (entityId1 != 0 && registry->HasComponent<CollisionComponent>((entt::entity)entityId1)) {
                    auto& comp = registry->GetComponent<CollisionComponent>((entt::entity)entityId1);
                    if (comp.onCollisionEnter) {
                        comp.onCollisionEnter((entt::entity)entityId2);
                    }
                }
            };
            callEnterEvent(inBody1, inBody2);
            callEnterEvent(inBody2, inBody1);
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
    //! @struct CollisionSystem::Impl
    //! @brief  システム実装の隠蔽用構造体
    //-------------------------------------------------------------
    struct CollisionSystem::Impl {
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
    CollisionSystem::CollisionSystem() {
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
    CollisionSystem::~CollisionSystem() {
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
    void CollisionSystem::Update(Tsukino::ECS::Registry& registry, float deltaTime) {
        m_impl->contactListener->registry = &registry;

        // Initialize newly added collision components
        auto view = registry.View<CollisionComponent>();
        JPH::BodyInterface& bodyInterface = m_impl->physicsSystem->GetBodyInterface();

        for (auto entity : view) {
            auto& comp = registry.GetComponent<CollisionComponent>(entity);
            if (!comp.isInitialized) {
                JPH::RefConst<JPH::Shape> shape;

                if (comp.type == ColliderType::Box) {
                    shape = new JPH::BoxShape(JPH::Vec3(comp.extent.x, comp.extent.y, comp.extent.z));
                } else if (comp.type == ColliderType::Sphere) {
                    shape = new JPH::SphereShape(comp.extent.x);
                } else if (comp.type == ColliderType::Capsule) {
                    shape = new JPH::CapsuleShape(comp.extent.y, comp.extent.x);
                }

                if (shape) {
                    JPH::BodyCreationSettings settings(shape, JPH::RVec3(0, 0, 0), JPH::Quat::sIdentity(), 
                        JPH::EMotionType::Dynamic, Layers::MOVING);
                    settings.mIsSensor = comp.isSensor;

                    JPH::Body* body = bodyInterface.CreateBody(settings);
                    if (body) {
                        body->SetUserData((uint64_t)entity);
                        comp.bodyID = body->GetID();
                        bodyInterface.AddBody(comp.bodyID, JPH::EActivation::Activate);
                        comp.isInitialized = true;
                    }
                }
            }
        }

        // Step the world
        const int cCollisionSteps = 1;
        float stepTime = deltaTime > 0.0f ? deltaTime : 1.0f / 60.0f;
        m_impl->physicsSystem->Update(stepTime, cCollisionSteps, m_impl->tempAllocator, m_impl->jobSystem);

        // F5 key logic for debug drawing
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
