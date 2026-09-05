//-------------------------------------------------------------
//! @file   PhysicsSystem.cpp
//! @brief  PhysicsSystemクラスの実装
//! @author 山﨑愛
//-------------------------------------------------------------

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Tsukino/EngineIntegration/ECS/System/PhysicsSystem.hpp>

#include <Tsukino/BuiltIn/ECS/Component/CollisionComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/RigidbodyComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/ImpulseRequestComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/CharacterControllerComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Event/CollisionEnterEvent.hpp>

#include <Tsukino/Physics/PhysicsMath.hpp>
#include <Tsukino/Physics/PhysicsWorld.hpp>

#include <Tsukino/Renderer/Renderer.hpp>
#include <Tsukino/EngineIntegration/EngineContext.hpp>
#include <Tsukino/EngineIntegration/Physics/RendererPhysicsDebugDraw.hpp>

#include <Tsukino/Core/ECS/Event/EventBus.hpp>
#include <Tsukino/Core/Log.hpp>
#include <Tsukino/Core/DebugTools/DebugFeatures.hpp>

#include <hlsl++.h>
#include <entt/entt.hpp>

#include <windows.h>

#include <string>
#include <vector>

// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {

    namespace {

        namespace Phys = Tsukino::Physics;

        //-------------------------------------------------------------
        //! @brief  コライダーの形状種別を物理側の形状種別へ変換する
        //! @param  type [in] 変換元
        //! @return 対応する物理側の形状種別
        //-------------------------------------------------------------
        Phys::ShapeType ToPhysicsShapeType(ColliderType type) {
            switch(type) {
            case ColliderType::Sphere:
                return Phys::ShapeType::Sphere;
            case ColliderType::Capsule:
                return Phys::ShapeType::Capsule;
            case ColliderType::Heightfield:
                return Phys::ShapeType::Heightfield;
            case ColliderType::Box:
            default:
                return Phys::ShapeType::Box;
            }
        }

        //-------------------------------------------------------------
        //! @brief  Rigidbody の種別を物理側の運動タイプへ変換する
        //! @param  type [in] 変換元
        //! @return 対応する運動タイプ
        //-------------------------------------------------------------
        Phys::MotionType ToPhysicsMotionType(RigidbodyType type) {
            switch(type) {
            case RigidbodyType::Kinematic:
                return Phys::MotionType::Kinematic;
            case RigidbodyType::Dynamic:
                return Phys::MotionType::Dynamic;
            case RigidbodyType::Static:
            default:
                return Phys::MotionType::Static;
            }
        }

        //-------------------------------------------------------------
        //! @brief  Rigidbody の凍結フラグから許可軸マスクを組み立てる
        //! @param  rb [in] 参照する Rigidbody
        //! @return 許可する移動・回転軸
        //-------------------------------------------------------------
        Phys::DofMask MakeDofMask(const RigidbodyComponent& rb) {
            Phys::DofMask dofs = Phys::DofMask::All;
            if(rb.freezePositionX)
                dofs &= ~Phys::DofMask::TranslationX;
            if(rb.freezePositionY)
                dofs &= ~Phys::DofMask::TranslationY;
            if(rb.freezePositionZ)
                dofs &= ~Phys::DofMask::TranslationZ;
            if(rb.freezeRotationX)
                dofs &= ~Phys::DofMask::RotationX;
            if(rb.freezeRotationY)
                dofs &= ~Phys::DofMask::RotationY;
            if(rb.freezeRotationZ)
                dofs &= ~Phys::DofMask::RotationZ;
            return dofs;
        }

        //-------------------------------------------------------------
        //! @brief  接地判定用ボックスの線をデバッグ描画へ積む
        //! @param  sink       [in,out] 描画の出力先
        //! @param  center     [in]     ボックス中心のワールド座標
        //! @param  halfExtent [in]     ボックスの各軸の半分サイズ
        //! @param  color      [in]     線の色（RGBA、各成分 0.0〜1.0）
        //-------------------------------------------------------------
        void DrawWireBox(Phys::IPhysicsDebugDraw& sink, const hlslpp::float3& center, const hlslpp::float3& halfExtent, const hlslpp::float4& color) {
            const float cx = center.x;
            const float cy = center.y;
            const float cz = center.z;
            const float ex = halfExtent.x;
            const float ey = halfExtent.y;
            const float ez = halfExtent.z;

            // 上面
            sink.DrawLine({cx - ex, cy + ey, cz - ez}, {cx + ex, cy + ey, cz - ez}, color);
            sink.DrawLine({cx + ex, cy + ey, cz - ez}, {cx + ex, cy + ey, cz + ez}, color);
            sink.DrawLine({cx + ex, cy + ey, cz + ez}, {cx - ex, cy + ey, cz + ez}, color);
            sink.DrawLine({cx - ex, cy + ey, cz + ez}, {cx - ex, cy + ey, cz - ez}, color);
            // 下面
            sink.DrawLine({cx - ex, cy - ey, cz - ez}, {cx + ex, cy - ey, cz - ez}, color);
            sink.DrawLine({cx + ex, cy - ey, cz - ez}, {cx + ex, cy - ey, cz + ez}, color);
            sink.DrawLine({cx + ex, cy - ey, cz + ez}, {cx - ex, cy - ey, cz + ez}, color);
            sink.DrawLine({cx - ex, cy - ey, cz + ez}, {cx - ex, cy - ey, cz - ez}, color);
            // 縦辺
            sink.DrawLine({cx - ex, cy + ey, cz - ez}, {cx - ex, cy - ey, cz - ez}, color);
            sink.DrawLine({cx + ex, cy + ey, cz - ez}, {cx + ex, cy - ey, cz - ez}, color);
            sink.DrawLine({cx + ex, cy + ey, cz + ez}, {cx + ex, cy - ey, cz + ez}, color);
            sink.DrawLine({cx - ex, cy + ey, cz + ez}, {cx - ex, cy - ey, cz + ez}, color);
        }

    }    // namespace

    //-------------------------------------------------------------
    //! @brief コンストラクタ
    //-------------------------------------------------------------
    PhysicsSystem::PhysicsSystem(Tsukino::ECS::EventBus& eventBus) {
        m_world = std::make_unique<Tsukino::Physics::PhysicsWorld>();

        //-------------------------------------------------------------
        // イベントバスは物理ワールドではなくシステム側が持つ。
        // 発行はワーカースレッドではなくメインスレッド（Update の末尾）で行うため。
        //-------------------------------------------------------------
        m_eventBus = &eventBus;
    }

    //-------------------------------------------------------------
    // 物理コリジョンのデバッグワイヤーフレーム描画を有効/無効にする
    // （デバッグ描画を含まないビルドでは値を保持するだけで効果は無い）
    //-------------------------------------------------------------
    void PhysicsSystem::SetDebugDrawEnabled(bool enabled) {
        m_isDebugDrawEnabled = enabled;
    }

    //-------------------------------------------------------------
    // デストラクタ
    //-------------------------------------------------------------
    PhysicsSystem::~PhysicsSystem() {
        //-------------------------------------------------------------
        // 先にレジストリのシグナル購読を解除する。
        // System は Registry より先に破棄されるため、解除しないと
        // レジストリ側の後始末で破棄済みの this が呼ばれてしまう。
        //-------------------------------------------------------------
        if(m_connectedRegistry) {
            m_connectedRegistry->OnDestroy<CollisionComponent>().disconnect(this);
            m_connectedRegistry->OnDestroy<CharacterControllerComponent>().disconnect(this);
            m_connectedRegistry = nullptr;
        }
    }

    //-------------------------------------------------------------
    //! @brief  指定のカプセル形状と現在重なっている全エンティティを取得する
    //-------------------------------------------------------------
    std::vector<entt::entity> PhysicsSystem::OverlapCapsule(const hlslpp::float3&     center,
                                                            const hlslpp::quaternion& rotation,
                                                            float                     radius,
                                                            float                     halfHeight) {
        std::vector<entt::entity> result;
        if(!m_world)
            return result;

        //-------------------------------------------------------------
        // ボディのユーザーデータにはエンティティを入れてあるので読み替える
        //-------------------------------------------------------------
        const std::vector<uint64_t> hits = m_world->OverlapCapsule(center, rotation, radius, halfHeight);
        result.reserve(hits.size());
        for(uint64_t userData : hits) {
            result.push_back(static_cast<entt::entity>(userData));
        }
        return result;
    }

    //-------------------------------------------------------------
    // レジストリの破棄シグナルへ購読する
    //-------------------------------------------------------------
    void PhysicsSystem::ConnectRegistrySignals(Tsukino::ECS::Registry& registry) {
        if(m_connectedRegistry == &registry)
            return;    // 購読済み

        registry.OnDestroy<CollisionComponent>().connect<&PhysicsSystem::OnCollisionComponentDestroyed>(*this);
        registry.OnDestroy<CharacterControllerComponent>().connect<&PhysicsSystem::OnCharacterControllerDestroyed>(*this);

        m_connectedRegistry = &registry;
    }

    //-------------------------------------------------------------
    // CollisionComponent 破棄時に物理ワールドの Body を回収する
    //-------------------------------------------------------------
    void PhysicsSystem::OnCollisionComponentDestroyed(entt::registry& registry, entt::entity entity) {
        if(!m_world)
            return;

        //-------------------------------------------------------------
        // EnTT は「取り外す直前」に呼ぶので、この時点ではまだ読める
        //-------------------------------------------------------------
        if(const CollisionComponent* col = registry.try_get<CollisionComponent>(entity)) {
            if(col->isInitialized && col->bodyID.IsValid()) {
                m_world->DestroyBody(col->bodyID);
            }
        }

        //-------------------------------------------------------------
        // エンティティをキーにした付随データも一緒に片付ける。
        // ここを漏らすとフレームごとに増え続けるだけのマップになる。
        //-------------------------------------------------------------
        m_prevPositions.erase(entity);
        m_world->ForgetShapeCache(static_cast<uint64_t>(entity));
    }

    //-------------------------------------------------------------
    // CharacterControllerComponent 破棄時にキャラクターを回収する
    //-------------------------------------------------------------
    void PhysicsSystem::OnCharacterControllerDestroyed(entt::registry& registry, entt::entity entity) {
        auto it = m_characters.find(entity);
        if(it == m_characters.end())
            return;

        if(m_world) {
            m_world->DestroyCharacter(it->second);
        }
        m_characters.erase(it);
    }

    //-------------------------------------------------------------
    // システムの更新処理
    //-------------------------------------------------------------
    void PhysicsSystem::Update(Tsukino::ECS::Registry& registry, float deltaTime) {
        //-------------------------------------------------------------
        // レジストリはコンストラクタ時点では手に入らないため、
        // 初回 Update で一度だけ破棄シグナルへ購読する
        //-------------------------------------------------------------
        ConnectRegistrySignals(registry);

        auto view = registry.View<CollisionComponent>();

        // 1. 生成処理
        for(auto entity : view) {
            auto& col = registry.GetComponent<CollisionComponent>(entity);
            if(col.isInitialized)
                continue;

            Tsukino::Physics::BodyDesc desc;
            desc.shape.type   = ToPhysicsShapeType(col.type);
            desc.shape.extent = col.extent;
            if(col.type == ColliderType::Heightfield) {
                desc.shape.heightSamples = col.heightfieldSamples.empty() ? nullptr : col.heightfieldSamples.data();
                desc.shape.heightSize    = col.heightfieldSize;
                desc.shape.heightOffset  = col.heightfieldOffset;
                desc.shape.heightScale   = col.heightfieldScale;
            }

            //-------------------------------------------------------------
            // Transform とオフセットから物理用の初期姿勢を計算する
            //-------------------------------------------------------------
            hlslpp::float3     basePosition = {0.0f, 0.0f, 0.0f};
            hlslpp::quaternion baseRotation = hlslpp::quaternion::identity();
            if(registry.HasComponent<TransformComponent>(entity)) {
                auto& tf     = registry.GetComponent<TransformComponent>(entity);
                basePosition = tf.position;
                baseRotation = tf.rotation;
            }
            Tsukino::Physics::ComposeTransform(
                basePosition, baseRotation, col.offsetPosition, col.offsetRotation, desc.position, desc.rotation);

            desc.isSensor = col.isSensor;
            desc.userData = static_cast<uint64_t>(entity);

            //-------------------------------------------------------------
            // Rigidbody を持たないエンティティは Static のまま、質量まわりも
            // 物理側の既定に任せる（従来の挙動を維持している）
            //-------------------------------------------------------------
            if(registry.HasComponent<RigidbodyComponent>(entity)) {
                auto& rb                    = registry.GetComponent<RigidbodyComponent>(entity);
                desc.motion                 = ToPhysicsMotionType(rb.type);
                desc.friction               = rb.friction;
                desc.restitution            = rb.restitution;
                desc.gravityFactor          = rb.gravityFactor;
                desc.mass                   = rb.mass;
                desc.allowedDofs            = MakeDofMask(rb);
                desc.overrideMassProperties = true;
            }

            const Tsukino::Physics::BodyHandle handle = m_world->CreateBody(desc, static_cast<uint64_t>(entity));
            if(handle.IsValid()) {
                col.bodyID        = handle;
                col.isInitialized = true;
            }
        }

        float stepTime = deltaTime > 0.0f ? deltaTime : 1.0f / 60.0f;

        // CharacterVirtualの生成処理
        auto charView = registry.View<CharacterControllerComponent, TransformComponent>();
        charView.each([&](auto entity, auto& cc, auto& tf) {
            if(cc.isInitialized)
                return;

            Tsukino::Physics::CharacterDesc desc;
            desc.radius       = cc.radius;
            desc.halfHeight   = cc.halfHeight;
            desc.maxSlopeDeg  = cc.maxSlopeDeg;
            desc.mass         = cc.mass;
            desc.centerOffset = cc.centerOffset;
            desc.position     = tf.position;
            desc.rotation     = tf.rotation;
            desc.userData     = static_cast<uint64_t>(entity);

            m_characters[entity] = m_world->CreateCharacter(desc);
            cc.isInitialized     = true;

            Tsukino::Core::Log::Info("CharacterVirtual created for entity id=" + std::to_string((uint32_t)entity));
        });

        // 破棄されたキャラクターの回収は OnCharacterControllerDestroyed() が行う。
        // 以前はここで毎フレーム characters を全走査していたが、
        // EnTT の破棄シグナルはコンポーネント削除・エンティティ破棄の
        // どちらでも必ず発火するため、この走査は不要になった。

        // 2. Kinematic ボディの姿勢同期
        for(auto entity : view) {
            auto& col = registry.GetComponent<CollisionComponent>(entity);
            if(!registry.HasComponent<RigidbodyComponent>(entity))
                continue;
            auto& rb = registry.GetComponent<RigidbodyComponent>(entity);
            if(col.isInitialized && rb.type == RigidbodyType::Kinematic && registry.HasComponent<TransformComponent>(entity)) {
                auto& tf = registry.GetComponent<TransformComponent>(entity);

                //-------------------------------------------------------------
                // 回転の正規化は ComposeTransform() が内包している。
                // slerp 等の補間で tf.rotation を毎フレーム自己更新していると
                // 誤差が蓄積して非正規化し、物理側のアサートに引っかかるため。
                // 特にキャラクターコントローラーと併用しているエンティティ
                // （プレイヤー等）はずれが溜まりやすい
                //-------------------------------------------------------------
                hlslpp::float3     bodyPosition;
                hlslpp::quaternion bodyRotation;
                Tsukino::Physics::ComposeTransform(
                    tf.position, tf.rotation, col.offsetPosition, col.offsetRotation, bodyPosition, bodyRotation);

                m_world->SetPositionAndRotation(col.bodyID, bodyPosition, bodyRotation);

                auto it = m_prevPositions.find(entity);
                if(it != m_prevPositions.end()) {
                    const hlslpp::float3 velocity = (hlslpp::float3(tf.position) - it->second) / stepTime;
                    m_world->SetLinearVelocity(col.bodyID, velocity);
                }
                m_prevPositions[entity] = tf.position;
            }
        }

        std::vector<entt::entity> entitiesToRemoveImpulse;

        auto requestView = registry.View<ImpulseRequestComponent, CollisionComponent>();
        requestView.each([&](auto entity, auto& ir, auto& col) {
            if(col.isInitialized) {
                m_world->AddImpulse(col.bodyID, ir.impulse);

                // 回転（トルク）の付与
                const hlslpp::float3 angularImpulse = ir.angularImpulse;
                if(angularImpulse.x != 0.0f || angularImpulse.y != 0.0f || angularImpulse.z != 0.0f) {
                    m_world->AddAngularImpulse(col.bodyID, angularImpulse);
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

            const Tsukino::Physics::MotionType targetType = ToPhysicsMotionType(rb.type);
            if(m_world->GetMotionType(col.bodyID) != targetType) {
                m_world->SetMotionType(col.bodyID, targetType);
            }
        }

        //-------------------------------------------------------------
        // Freezeフラグ（許可軸）の変更を反映
        //-------------------------------------------------------------
        for(auto entity : view) {
            if(!registry.HasComponent<RigidbodyComponent>(entity))
                continue;
            auto& rb  = registry.GetComponent<RigidbodyComponent>(entity);
            auto& col = registry.GetComponent<CollisionComponent>(entity);
            if(!col.isInitialized)
                continue;

            if(!rb.isFreezeDirty)
                continue;

            m_world->SetAllowedDofs(col.bodyID, MakeDofMask(rb), rb.mass);

            rb.isFreezeDirty = false;
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
                m_world->AddForce(col.bodyID, rb.force);
            }
            if(hasTorque) {
                m_world->AddTorque(col.bodyID, rb.torque);
            }
        }

        // 3. 物理シミュレーション実行
        m_world->Step(stepTime);

        // 4. Dynamic同期
        for(auto entity : view) {
            auto& col = registry.GetComponent<CollisionComponent>(entity);
            if(col.isInitialized && registry.HasComponent<RigidbodyComponent>(entity)) {
                auto& rb = registry.GetComponent<RigidbodyComponent>(entity);
                if(rb.type == RigidbodyType::Dynamic && registry.HasComponent<TransformComponent>(entity)) {
                    auto& tf = registry.GetComponent<TransformComponent>(entity);

                    const Tsukino::Physics::BodyState state = m_world->GetBodyState(col.bodyID);

                    tf.position = state.position;
                    tf.rotation = state.rotation;
                    tf.dirty    = true;

                    // rb.linearVelocity/angularVelocityは静止判定など他システムが実測値として
                    // 参照するため、物理側の実速度をここで書き戻しておく（従来は書き戻されておらず、
                    // 常に初期値の0のまま扱われてしまっていた）
                    rb.linearVelocity  = state.linearVelocity;
                    rb.angularVelocity = state.angularVelocity;

                    const hlslpp::float3 checkCenter(tf.position.x, tf.position.y - rb.groundCheckDistance, tf.position.z);
                    const hlslpp::float3 checkExtent(rb.groundCheckRadius, 0.05f, rb.groundCheckRadius);
                    rb.isGrounded = m_world->OverlapBox(checkCenter, checkExtent, col.bodyID);
                }
            }
        }

        // キャラクターの同期（移動・ジャンプ対応）
        const hlslpp::float3 gravity = m_world->GetGravity();

        charView.each([&](auto entity, auto& cc, auto& tf) {
            auto it = m_characters.find(entity);
            if(it == m_characters.end())
                return;

            const Tsukino::Physics::CharacterHandle handle = it->second;

            bool wasGrounded = m_world->IsCharacterSupported(handle);

            // 縦方向速度の更新
            float vertY = cc.verticalVelocity.y;
            if(wasGrounded && vertY < 0.0f) {
                vertY = -0.1f;    // 地面に張り付かせる程度の小さい下向き速度（斜面を滑り落ちないように）
            }
            if(cc.jumpRequested && wasGrounded) {
                vertY = cc.jumpSpeed;
            }
            cc.jumpRequested = false;    // 消費して1フレームでリセット

            vertY += gravity.y * cc.gravityFactor * stepTime;

            //-------------------------------------------------------------
            // 水平（moveInput）＋垂直を合成した速度と、PlayerSystem等が今フレーム
            // 書き込んだ向きを渡す。向きを渡さないと更新後の姿勢が常に identity に
            // なり、下の tf.rotation 書き戻しで回転が毎フレーム上書きされてしまう
            //-------------------------------------------------------------
            Tsukino::Physics::CharacterInput input;
            input.linearVelocity = hlslpp::float3(cc.moveInput.x, vertY, cc.moveInput.z);
            input.rotation       = tf.rotation;

            Tsukino::Physics::CharacterOutput output;
            if(!m_world->StepCharacter(handle, input, output, stepTime))
                return;

            cc.isGrounded         = output.isGrounded;
            cc.verticalVelocity.y = output.verticalVelocity;    // 更新後の実際の縦速度を書き戻す

            tf.position = output.position;
            tf.rotation = output.rotation;
            tf.dirty    = true;
        });

        // デバッグ描画
#ifdef TSUKINO_DEBUG_COLLISION_DRAW
        bool f5IsDown = (::GetAsyncKeyState(VK_F5) & 0x8000) != 0;
        if(f5IsDown && !m_f5WasDown) {
            m_isDebugDrawEnabled = !m_isDebugDrawEnabled;
        }
        m_f5WasDown = f5IsDown;

        if(m_isDebugDrawEnabled) {
            auto* ctx = registry.GetContext<Tsukino::EngineIntegration::EngineContext*>();
            if(ctx && ctx->renderer) {
                Tsukino::EngineIntegration::RendererPhysicsDebugDraw sink(ctx->renderer);

                // ECSのviewから全ボディを描画
                for(auto entity : view) {
                    auto& col = registry.GetComponent<CollisionComponent>(entity);
                    if(!col.isInitialized)
                        continue;

                    m_world->DebugDrawBody(sink, col.bodyID);
                }

                // キャラクター（CollisionComponentを持たないため上のループでは描画されない）のカプセルを描画
                m_world->DebugDrawCharacters(sink);

                // isGrounded判定Box描画
                for(auto entity : view) {
                    if(!registry.HasComponent<RigidbodyComponent>(entity))
                        continue;
                    if(!registry.HasComponent<TransformComponent>(entity))
                        continue;
                    auto& rb = registry.GetComponent<RigidbodyComponent>(entity);
                    if(rb.type != RigidbodyType::Dynamic)
                        continue;

                    auto& tf = registry.GetComponent<TransformComponent>(entity);

                    const hlslpp::float3 center(tf.position.x, tf.position.y - rb.groundCheckDistance, tf.position.z);
                    const hlslpp::float3 halfExtent(rb.groundCheckRadius, 0.05f, rb.groundCheckRadius);
                    const hlslpp::float4 color = rb.isGrounded ? hlslpp::float4(0.0f, 1.0f, 0.0f, 1.0f) : hlslpp::float4(1.0f, 0.0f, 0.0f, 1.0f);

                    DrawWireBox(sink, center, halfExtent, color);
                }

                Tsukino::Renderer::DrawCommand cmd{};
                cmd.customDraw = [renderer = ctx->renderer](ID3D11DeviceContext* context) { renderer->FlushDebugDraw(); };
                ctx->renderer->PushDrawCommand(cmd);
            }
        }
#endif    // TSUKINO_DEBUG_COLLISION_DRAW

        //-------------------------------------------------------------
        // 接触の後処理（メインスレッド）
        //
        // 物理エンジンは接触コールバックをジョブスレッドから並行に呼ぶ。
        // EventBus も EnTT のレジストリもスレッドセーフではないので、
        // コールバック側では接触を積むだけにしてあり、
        // イベント発行とレジストリ操作はここまで遅らせている。
        //-------------------------------------------------------------
        m_world->DrainContacts(m_drainedContacts);

        //-------------------------------------------------------------
        // Kinematic ボディの速度を接触法線で反射させる
        // （Dynamic は物理エンジン側が解決するため対象外）
        //-------------------------------------------------------------
        auto applyReflection = [&registry](entt::entity e, const hlslpp::float3& n) {
            if(!registry.HasComponent<RigidbodyComponent>(e))
                return;

            auto& rb = registry.GetComponent<RigidbodyComponent>(e);
            if(rb.type != RigidbodyType::Kinematic)
                return;

            hlslpp::float3 V   = rb.linearVelocity;
            float          dot = hlslpp::dot(V, n);
            if(dot < 0.0f)
                rb.linearVelocity = V - 2.0f * dot * n;
        };

        for(const auto& contact : m_drainedContacts) {
            const entt::entity entityA = static_cast<entt::entity>(contact.userDataA);
            const entt::entity entityB = static_cast<entt::entity>(contact.userDataB);

            //-------------------------------------------------------------
            // 接触してから今ここへ来るまでの間にエンティティが破棄されている
            // ことがあるため、触る前に生存を確認する
            //-------------------------------------------------------------
            if(!registry.IsValid(entityA) || !registry.IsValid(entityB))
                continue;

            const hlslpp::float3 reverseNormal = hlslpp::float3(-contact.normal.x, -contact.normal.y, -contact.normal.z);

            // イベント発行（衝突の事実を双方向に通知）
            if(m_eventBus) {
                // Aから見たBへのイベント
                m_eventBus->Publish(CollisionEnterEvent{entityA, entityB, contact.normal});
                // Bから見たAへのイベント
                m_eventBus->Publish(CollisionEnterEvent{entityB, entityA, reverseNormal});
            }

            applyReflection(entityA, contact.normal);
            applyReflection(entityB, reverseNormal);
        }

        m_drainedContacts.clear();
    }

}    // namespace Tsukino::BuiltIn::ECS
