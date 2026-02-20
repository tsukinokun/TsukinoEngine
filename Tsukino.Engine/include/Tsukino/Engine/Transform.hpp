//-------------------------------------------------------------
//! @file   Transform.hpp
//! @brief  Transformクラスの宣言
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once
#include <hlsl++.h>
#include <Tsukino/Core/Math/Matrix.hpp>
// 名前空間 : Tsukino::Engine
namespace Tsukino::Engine {
    //-------------------------------------------------------------
    //! @class  Transform
    //! @brief  位置、回転、スケールを管理するクラス
    //-------------------------------------------------------------
    class Transform {
    public:
        hlslpp::float3     position = {0.0f, 0.0f, 0.0f};                // world space
        hlslpp::quaternion rotation = hlslpp::quaternion::identity();    // radians
        hlslpp::float3     scale    = {1.0f, 1.0f, 1.0f};                // local space

        //-------------------------------------------------------------
        // ワールド変換行列を取得
        //! @return ワールド変換行列
        //-------------------------------------------------------------
        matrix GetMatrix() const;
    };

}    // namespace Tsukino::Engine
