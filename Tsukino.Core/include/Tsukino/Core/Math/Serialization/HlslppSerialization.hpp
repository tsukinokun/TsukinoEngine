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

    template <class Archive>
    void serialize(Archive& ar, hlslpp::float2& vec) {
        ar(cereal::make_nvp("x", vec.x), cereal::make_nvp("y", vec.y));
    }

    template <class Archive>
    void serialize(Archive& ar, hlslpp::float3& vec) {
        ar(cereal::make_nvp("x", vec.x), cereal::make_nvp("y", vec.y), cereal::make_nvp("z", vec.z));
    }

    template <class Archive>
    void serialize(Archive& ar, hlslpp::float4& vec) {
        ar(cereal::make_nvp("x", vec.x), cereal::make_nvp("y", vec.y), cereal::make_nvp("z", vec.z), cereal::make_nvp("w", vec.w));
    }

    //@}

    //----------------------------------------------------------
    //! @name   hlsl++ 四元数（クォータニオン）のシリアライズ
    //----------------------------------------------------------
    //@{

    template <class Archive>
    void serialize(Archive& ar, hlslpp::quaternion& q) {
        ar(cereal::make_nvp("x", q.x), cereal::make_nvp("y", q.y), cereal::make_nvp("z", q.z), cereal::make_nvp("w", q.w));
    }

    //@}

    //----------------------------------------------------------
    //! @name   hlsl++ 標準行列のシリアライズ
    //----------------------------------------------------------
    //@{

    template <class Archive>
    void serialize(Archive& ar, hlslpp::float4x4& mat) {
        // hlsl++のfloat4x4はoperator[]で各行(float4)にアクセス可能
        ar(cereal::make_nvp("row0", mat[0]), cereal::make_nvp("row1", mat[1]), cereal::make_nvp("row2", mat[2]), cereal::make_nvp("row3", mat[3]));
    }

    //@}

    //----------------------------------------------------------
    //! @name   自作 matrix クラスのシリアライズ
    //----------------------------------------------------------
    //@{

    template <class Archive>
    void serialize(Archive& ar, Tsukino::Core::Math::matrix& mat) {
        // 基底クラスである hlslpp::float4x4 のデータ構造をそのまま展開
        // 追加のメンバ変数がないため、同様に各行をシリアライズすればOKです
        ar(cereal::make_nvp("row0", mat[0]), cereal::make_nvp("row1", mat[1]), cereal::make_nvp("row2", mat[2]), cereal::make_nvp("row3", mat[3]));
    }

    //@}

}    // namespace cereal
