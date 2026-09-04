//----------------------------------------------------------------------------
//! @file   JoltLayers.hpp
//! @brief  Jolt のレイヤー定義と衝突フィルタ
//! @detail Tsukino.Physics の内部専用ヘッダです。オブジェクトレイヤーと
//!         ブロードフェーズレイヤーの対応、およびレイヤー間で衝突させるか
//!         どうかの判定を定義します。
//----------------------------------------------------------------------------
#pragma once
#include <Jolt/Jolt.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>

#include <cstdint>

// 名前空間 : Tsukino::Physics
namespace Tsukino::Physics {

    //------------------------------------------------------------------------
    //! @namespace Layers
    //! オブジェクトレイヤーの定義
    //------------------------------------------------------------------------
    namespace Layers {
        static constexpr JPH::ObjectLayer NON_MOVING = 0;    //!< 静的オブジェクトレイヤー
        static constexpr JPH::ObjectLayer MOVING     = 1;    //!< 動的オブジェクトレイヤー
        static constexpr JPH::ObjectLayer NUM_LAYERS = 2;    //!< オブジェクトレイヤー数
    }

    //------------------------------------------------------------------------
    //! @namespace BroadPhaseLayers
    //! ブロードフェーズレイヤーの定義
    //------------------------------------------------------------------------
    namespace BroadPhaseLayers {
        static constexpr JPH::BroadPhaseLayer NON_MOVING(0);    //!< 静的ブロードフェーズレイヤー
        static constexpr JPH::BroadPhaseLayer MOVING(1);        //!< 動的ブロードフェーズレイヤー
        static constexpr uint32_t             NUM_LAYERS(2);    //!< ブロードフェーズレイヤー数
    }

    //------------------------------------------------------------------------
    //! オブジェクトレイヤーとブロードフェーズレイヤーの対応を定義する連携インターフェース
    //------------------------------------------------------------------------
    class BPLayerInterfaceImpl final : public JPH::BroadPhaseLayerInterface {
    public:
        //! コンストラクタ
        BPLayerInterfaceImpl() {
            m_objectToBroadPhase[Layers::NON_MOVING] = BroadPhaseLayers::NON_MOVING;
            m_objectToBroadPhase[Layers::MOVING]     = BroadPhaseLayers::MOVING;
        }

        //! ブロードフェーズレイヤー数を取得します。
        //! @return ブロードフェーズレイヤーの数
        uint32_t GetNumBroadPhaseLayers() const override { return BroadPhaseLayers::NUM_LAYERS; }

        //! 指定したオブジェクトレイヤーに対応するブロードフェーズレイヤーを取得します。
        //! @param  [in] inLayer オブジェクトレイヤー
        //! @return 対応するブロードフェーズレイヤー
        JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const override {
            JPH_ASSERT(inLayer < Layers::NUM_LAYERS);
            return m_objectToBroadPhase[inLayer];
        }

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
        //! プロファイラ表示用のブロードフェーズレイヤー名を取得します。
        //! @param  [in] inLayer ブロードフェーズレイヤー
        //! @return レイヤー名
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

    //------------------------------------------------------------------------
    //! オブジェクト同士のブロードフェーズレベルでの衝突判定可否を定義するフィルタ
    //------------------------------------------------------------------------
    class ObjectVsBroadPhaseLayerFilterImpl : public JPH::ObjectVsBroadPhaseLayerFilter {
    public:
        //! 衝突すべきかどうかを判定します。
        //! @param  [in] inLayer1 オブジェクトのレイヤー
        //! @param  [in] inLayer2 相手側のブロードフェーズレイヤー
        //! @return 衝突する場合は true
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

    //------------------------------------------------------------------------
    //! オブジェクトレイヤー同士での衝突判定可否を定義するフィルタ
    //------------------------------------------------------------------------
    class ObjectLayerPairFilterImpl : public JPH::ObjectLayerPairFilter {
    public:
        //! 衝突すべきかどうかを判定します。
        //! @param  [in] inObject1 判定元オブジェクトのレイヤー
        //! @param  [in] inObject2 対象のオブジェクトレイヤー
        //! @return 衝突する場合は true
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

}    // namespace Tsukino::Physics
