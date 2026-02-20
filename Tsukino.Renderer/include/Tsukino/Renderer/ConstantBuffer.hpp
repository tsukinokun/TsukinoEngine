//--------------------------------------------------------------
//! @file   ConstantBuffer.hpp
//! @brief  VS用定数バッファ構造体（行列）
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
#include <DirectXMath.h>
// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {
    //--------------------------------------------------------------
    //! @struct CBufferTransform
    //--------------------------------------------------------------
    struct CBufferTransform {
        DirectX::XMMATRIX mvp;    // 16 バイトアラインメントされた 4×4 行列型（SIMD 最適化対応）
    };
}    // namespace Tsukino::Renderer
