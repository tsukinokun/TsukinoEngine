//--------------------------------------------------------------
//! @file   ConstantBuffer.hpp
//! @brief  VS用定数バッファ構造体（行列）
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
#include <Tsukino/Core/Math/Matrix.hpp>
// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {
    //--------------------------------------------------------------
    //! @struct CBufferTransform
    //--------------------------------------------------------------
    struct CBufferTransform {
        Tsukino::Core::Math::matrix mvp;
    };
}    // namespace Tsukino::Renderer
