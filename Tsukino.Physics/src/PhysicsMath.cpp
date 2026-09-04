//----------------------------------------------------------------------------
//! @file   PhysicsMath.cpp
//! @brief  PhysicsMath.hpp の実装
//! @detail 物理エンジン（Jolt）のクォータニオン演算をそのまま使い、姿勢の
//!         合成結果が物理側の内部処理と一致するようにしています。
//----------------------------------------------------------------------------
#include <Tsukino/Physics/PhysicsMath.hpp>

#include "JoltConversion.hpp"

#include <Jolt/Jolt.h>
#include <Jolt/Math/Quat.h>
#include <Jolt/Math/Vec3.h>

// 名前空間 : Tsukino::Physics
namespace Tsukino::Physics {

    //------------------------------------------------------------------------
    //! 親の姿勢へローカルオフセットを合成した姿勢を求めます。
    //------------------------------------------------------------------------
    void ComposeTransform(const hlslpp::float3&     position,
                          const hlslpp::quaternion& rotation,
                          const hlslpp::float3&     offsetPosition,
                          const hlslpp::quaternion& offsetRotation,
                          hlslpp::float3&           outPosition,
                          hlslpp::quaternion&       outRotation) {
        //--------------------------------------------------------------------
        // ToJoltQuat() が正規化を行う。
        // 下の rot * localOffset（Quat::operator*(Vec3)）が IsNormalized() を
        // 要求するため、ここを通さずに合成してはならない
        //--------------------------------------------------------------------
        const JPH::Quat rot       = ToJoltQuat(rotation);
        const JPH::Quat offsetRot = ToJoltQuat(offsetRotation);
        const JPH::Vec3 offsetPos = ToJoltVec3(offsetPosition);

        outPosition = ToFloat3(ToJoltVec3(position) + (rot * offsetPos));
        outRotation = ToQuaternion(rot * offsetRot);
    }

}    // namespace Tsukino::Physics
