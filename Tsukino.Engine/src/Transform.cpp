//-------------------------------------------------------------
//! @file   Transform.cpp
//! @brief  Transformクラスの実装
//! @author 山﨑愛
//-------------------------------------------------------------
#include <Tsukino/Engine/Transform.hpp>
// 名前空間 : Tsukino::Engine
namespace Tsukino::Engine {
    //-------------------------------------------------------------
    //! @brief  ワールド変換行列を取得
    //! @return ワールド変換行列
    //-------------------------------------------------------------
    matrix Transform::GetMatrix() const {
        // Scale
        matrix S = matrix::scale(scale);

        // Rotation (quaternion → matrix)
        matrix R = matrix(rotation);

        // Translation
        matrix T = matrix::translate(position);

        // World = T * R * S
        return T * R * S;
    }

}    // namespace Tsukino::Engine
