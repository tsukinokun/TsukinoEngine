//--------------------------------------------------------------
//! @file   HlslppSerialization.hpp
//! @brief  hlsl++および自作数学クラスのcerealシリアライズ定義
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once

#include <Tsukino/Core/Math/Matrix.hpp>
#include <hlsl++.h>
#include <cereal/cereal.hpp>

// 名前空間 : cereal
namespace cereal {

    //----------------------------------------------------------
    //! @name   hlsl++ ベクトル系のシリアライズ
    //----------------------------------------------------------
    //@{

    // --- hlslpp::float2 ---
    template <class Archive>
    void save(Archive& ar, const hlslpp::float2& vec) {
        ar(cereal::make_nvp("x", static_cast<float>(vec.x)), cereal::make_nvp("y", static_cast<float>(vec.y)));
    }

    template <class Archive>
    void load(Archive& ar, hlslpp::float2& vec) {
        float x = 0.0f;
        float y = 0.0f;
        ar(cereal::make_nvp("x", x), cereal::make_nvp("y", y));
        vec.x = x;
        vec.y = y;
    }

    // --- hlslpp::float3 ---
    template <class Archive>
    void save(Archive& ar, const hlslpp::float3& vec) {
        ar(cereal::make_nvp("x", static_cast<float>(vec.x)),
           cereal::make_nvp("y", static_cast<float>(vec.y)),
           cereal::make_nvp("z", static_cast<float>(vec.z)));
    }

    template <class Archive>
    void load(Archive& ar, hlslpp::float3& vec) {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
        ar(cereal::make_nvp("x", x), cereal::make_nvp("y", y), cereal::make_nvp("z", z));
        vec.x = x;
        vec.y = y;
        vec.z = z;
    }

    // --- hlslpp::float4 ---
    template <class Archive>
    void save(Archive& ar, const hlslpp::float4& vec) {
        ar(cereal::make_nvp("x", static_cast<float>(vec.x)),
           cereal::make_nvp("y", static_cast<float>(vec.y)),
           cereal::make_nvp("z", static_cast<float>(vec.z)),
           cereal::make_nvp("w", static_cast<float>(vec.w)));
    }

    template <class Archive>
    void load(Archive& ar, hlslpp::float4& vec) {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
        float w = 0.0f;
        ar(cereal::make_nvp("x", x), cereal::make_nvp("y", y), cereal::make_nvp("z", z), cereal::make_nvp("w", w));
        vec.x = x;
        vec.y = y;
        vec.z = z;
        vec.w = w;
    }

    //@}

    //----------------------------------------------------------
    //! @name   hlsl++ 四元数（クォータニオン）のシリアライズ
    //----------------------------------------------------------
    //@{

    template <class Archive>
    void save(Archive& ar, const hlslpp::quaternion& q) {
        ar(cereal::make_nvp("x", static_cast<float>(q.x)),
           cereal::make_nvp("y", static_cast<float>(q.y)),
           cereal::make_nvp("z", static_cast<float>(q.z)),
           cereal::make_nvp("w", static_cast<float>(q.w)));
    }

    template <class Archive>
    void load(Archive& ar, hlslpp::quaternion& q) {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
        float w = 0.0f;
        ar(cereal::make_nvp("x", x), cereal::make_nvp("y", y), cereal::make_nvp("z", z), cereal::make_nvp("w", w));
        q.x = x;
        q.y = y;
        q.z = z;
        q.w = w;
    }

    //@}

    //----------------------------------------------------------
    //! @name   hlsl++ 標準行列のシリアライズ
    //----------------------------------------------------------
    //@{

    template <class Archive>
    void save(Archive& ar, const hlslpp::float4x4& mat) {
        ar(cereal::make_nvp("row0", mat[0]), cereal::make_nvp("row1", mat[1]), cereal::make_nvp("row2", mat[2]), cereal::make_nvp("row3", mat[3]));
    }

    template <class Archive>
    void load(Archive& ar, hlslpp::float4x4& mat) {
        hlslpp::float4 r0, r1, r2, r3;
        ar(cereal::make_nvp("row0", r0), cereal::make_nvp("row1", r1), cereal::make_nvp("row2", r2), cereal::make_nvp("row3", r3));
        mat[0] = r0;
        mat[1] = r1;
        mat[2] = r2;
        mat[3] = r3;
    }

    //@}

    //----------------------------------------------------------
    //! @name   自作 matrix クラスのシリアライズ
    //----------------------------------------------------------
    //@{

    template <class Archive>
    void save(Archive& ar, const Tsukino::Core::Math::matrix& mat) {
        ar(cereal::make_nvp("row0", mat[0]), cereal::make_nvp("row1", mat[1]), cereal::make_nvp("row2", mat[2]), cereal::make_nvp("row3", mat[3]));
    }

    template <class Archive>
    void load(Archive& ar, Tsukino::Core::Math::matrix& mat) {
        hlslpp::float4 r0, r1, r2, r3;
        ar(cereal::make_nvp("row0", r0), cereal::make_nvp("row1", r1), cereal::make_nvp("row2", r2), cereal::make_nvp("row3", r3));
        mat[0] = r0;
        mat[1] = r1;
        mat[2] = r2;
        mat[3] = r3;
    }

    //@}

}    // namespace cereal
