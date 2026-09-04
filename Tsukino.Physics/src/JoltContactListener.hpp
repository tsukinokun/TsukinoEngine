//----------------------------------------------------------------------------
//! @file   JoltContactListener.hpp
//! @brief  Jolt の接触コールバックを受け取るリスナー
//! @detail Tsukino.Physics の内部専用ヘッダです。
//!         Jolt は接触コールバックを **物理ジョブスレッドから並行に** 呼び出します。
//!         上位のイベントバスも ECS のレジストリもスレッドセーフではないため、
//!         ここでは接触の事実だけをミューテックス保護されたバッファへ積み、
//!         実際の通知は PhysicsWorld::DrainContacts() を呼んだメインスレッド側で行います。
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/Physics/PhysicsTypes.hpp>

#include "JoltConversion.hpp"

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/Body.h>
#include <Jolt/Physics/Character/CharacterVirtual.h>
#include <Jolt/Physics/Collision/ContactListener.h>
#include <Jolt/Physics/PhysicsSystem.h>

#include <mutex>
#include <vector>

// 名前空間 : Tsukino::Physics
namespace Tsukino::Physics {

    //------------------------------------------------------------------------
    //! ボディ同士の衝突イベントを受け取るリスナー
    //------------------------------------------------------------------------
    class ContactListenerImpl : public JPH::ContactListener {
    public:
        //! 衝突が追加された（当たった）際に呼ばれるコールバックです。
        //! @param  [in]     inBody1    衝突したボディ1
        //! @param  [in]     inBody2    衝突したボディ2
        //! @param  [in]     inManifold マニフォールド情報（接触点等）
        //! @param  [in,out] ioSettings コンタクト設定（摩擦係数等のオーバーライド可能）
        //! @note   複数のジョブスレッドから並行に呼ばれる。積むだけに留めること
        void OnContactAdded(const JPH::Body&            inBody1,
                            const JPH::Body&            inBody2,
                            const JPH::ContactManifold& inManifold,
                            JPH::ContactSettings&       ioSettings) override {
            //----------------------------------------------------------------
            // 衝突時の法線（ボディ1から見たボディ2への法線）
            // Jolt の法線は「ボディ1から衝突点への方向」を指す
            //----------------------------------------------------------------
            const ContactRecord contact{inBody1.GetUserData(), inBody2.GetUserData(), ToFloat3(inManifold.mWorldSpaceNormal)};

            std::lock_guard<std::mutex> lock(m_pendingMutex);
            m_pending.push_back(contact);
        }

        //! 溜まった接触を取り出して空にします。
        //! @param  [out] out 取り出し先。呼び出し前の内容は破棄されます
        //! @note   メインスレッドから、Jolt の Update 完了後に呼ぶこと
        void DrainContacts(std::vector<ContactRecord>& out) {
            std::lock_guard<std::mutex> lock(m_pendingMutex);
            out.swap(m_pending);
            m_pending.clear();
        }

    private:
        std::mutex                 m_pendingMutex;    //!< m_pending を保護する
        std::vector<ContactRecord> m_pending;         //!< 今フレームに発生した接触
    };

    //------------------------------------------------------------------------
    //! CharacterVirtual の接触イベントを受け取るリスナー（他 Dynamic ボディを押す処理）
    //------------------------------------------------------------------------
    class CharacterContactListenerImpl : public JPH::CharacterContactListener {
    public:
        JPH::PhysicsSystem* physicsSystem = nullptr;    //!< 押し出し計算に使う物理システム参照

        //! CharacterVirtual が他ボディに接触した際に呼ばれるコールバックです。
        //! @param  [in]     inCharacter       接触したキャラクター本体
        //! @param  [in]     inBodyID2         接触相手のボディID
        //! @param  [in]     inSubShapeID2     接触相手のサブシェイプID
        //! @param  [in]     inContactPosition 接触位置（ワールド座標）
        //! @param  [in]     inContactNormal   接触法線
        //! @param  [in,out] ioSettings        接触設定（押し出し無効化等のオーバーライド可能）
        void OnContactAdded(const JPH::CharacterVirtual*   inCharacter,
                            const JPH::BodyID&             inBodyID2,
                            const JPH::SubShapeID&         inSubShapeID2,
                            JPH::RVec3Arg                  inContactPosition,
                            JPH::Vec3Arg                   inContactNormal,
                            JPH::CharacterContactSettings& ioSettings) override {
            if(!physicsSystem)
                return;

            JPH::BodyInterface& bodyInterface = physicsSystem->GetBodyInterface();
            if(bodyInterface.GetMotionType(inBodyID2) != JPH::EMotionType::Dynamic)
                return;

            // キャラクターの移動方向に押し出す（Step 7で本格実装予定。現状は仮のロジック）
            JPH::Vec3 characterVelocity = inCharacter->GetLinearVelocity();
            float     pushDot           = characterVelocity.Dot(-inContactNormal);
            if(pushDot > 0.0f) {
                bodyInterface.AddImpulse(inBodyID2, -inContactNormal * pushDot * inCharacter->GetMass() * 0.1f, inContactPosition);
            }
        }
    };

}    // namespace Tsukino::Physics
