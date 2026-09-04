//----------------------------------------------------------------------------
//! @file   JoltConversion.hpp
//! @brief  hlslpp と Jolt の間の型変換
//! @detail Tsukino.Physics の内部専用ヘッダです。公開ヘッダからは参照しません。
//!         クォータニオンの変換には正規化を内包させてあります。呼び出し側が
//!         slerp やボーン行列の分解で組み立てた回転はわずかに非正規化している
//!         ことがあり、Jolt 側の JPH_ASSERT(IsNormalized()) に引っかかるためです。
//----------------------------------------------------------------------------
#pragma once
#include <Jolt/Jolt.h>
#include <Jolt/Core/Color.h>
#include <Jolt/Math/Quat.h>
#include <Jolt/Math/Vec3.h>

#include <hlsl++.h>

// 名前空間 : Tsukino::Physics
namespace Tsukino::Physics {

    //------------------------------------------------------------------------
    //! hlslpp のベクトルを Jolt のベクトルへ変換します。
    //! @param  [in] v 変換元
    //! @return 変換後のベクトル
    //------------------------------------------------------------------------
    inline JPH::Vec3 ToJoltVec3(const hlslpp::float3& v) {
        return JPH::Vec3(v.x, v.y, v.z);
    }

    //------------------------------------------------------------------------
    //! hlslpp のベクトルを Jolt のワールド座標ベクトルへ変換します。
    //! @param  [in] v 変換元
    //! @return 変換後のベクトル
    //------------------------------------------------------------------------
    inline JPH::RVec3 ToJoltRVec3(const hlslpp::float3& v) {
        return JPH::RVec3(v.x, v.y, v.z);
    }

    //------------------------------------------------------------------------
    //! hlslpp のクォータニオンを Jolt のクォータニオンへ変換します。
    //! @param  [in] q 変換元
    //! @return 正規化済みのクォータニオン
    //! @note   正規化はここで一元的に行います。Jolt の Quat::operator*(Vec3) や
    //!         Mat44::sRotation は JPH_ASSERT(IsNormalized()) を持っており、
    //!         誤差の溜まった回転をそのまま渡すとアサートで停止するためです
    //------------------------------------------------------------------------
    inline JPH::Quat ToJoltQuat(const hlslpp::quaternion& q) {
        return JPH::Quat(q.x, q.y, q.z, q.w).Normalized();
    }

    //------------------------------------------------------------------------
    //! Jolt のベクトルを hlslpp のベクトルへ変換します。
    //! @param  [in] v 変換元
    //! @return 変換後のベクトル
    //------------------------------------------------------------------------
    inline hlslpp::float3 ToFloat3(JPH::Vec3Arg v) {
        return hlslpp::float3(v.GetX(), v.GetY(), v.GetZ());
    }

    //------------------------------------------------------------------------
    //! Jolt のクォータニオンを hlslpp のクォータニオンへ変換します。
    //! @param  [in] q 変換元
    //! @return 変換後のクォータニオン
    //------------------------------------------------------------------------
    inline hlslpp::quaternion ToQuaternion(JPH::QuatArg q) {
        return hlslpp::quaternion(q.GetX(), q.GetY(), q.GetZ(), q.GetW());
    }

    //------------------------------------------------------------------------
    //! Jolt の色を 0.0〜1.0 の RGBA へ変換します。
    //! @param  [in] c 変換元
    //! @return 変換後の色
    //------------------------------------------------------------------------
    inline hlslpp::float4 ToFloat4(JPH::ColorArg c) {
        return hlslpp::float4(c.r / 255.0f, c.g / 255.0f, c.b / 255.0f, c.a / 255.0f);
    }

}    // namespace Tsukino::Physics
