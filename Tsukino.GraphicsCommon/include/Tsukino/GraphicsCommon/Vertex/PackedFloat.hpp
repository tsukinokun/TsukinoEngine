//----------------------------------------------------------------------------
//! @file   PackedFloat.hpp
//! @brief  頂点の要素に使う、詰めて並べる float の組
//! @detail GPU へそのまま送る頂点の構造体に使います。hlslpp の float3 などと違い、
//!         SIMD のための詰め物が入らないので、メモリ上の並びが頂点レイアウトと一致します。
//----------------------------------------------------------------------------
#pragma once

// 名前空間 : Tsukino::GraphicsCommon
namespace Tsukino::GraphicsCommon {

    //------------------------------------------------------------------------
    //! float 2つの組です（UV など）。
    //------------------------------------------------------------------------
    struct Float2 {
        float x = 0.0f;    // 1つ目の成分
        float y = 0.0f;    // 2つ目の成分
    };

    //------------------------------------------------------------------------
    //! float 3つの組です（位置・法線など）。
    //------------------------------------------------------------------------
    struct Float3 {
        float x = 0.0f;    // 1つ目の成分
        float y = 0.0f;    // 2つ目の成分
        float z = 0.0f;    // 3つ目の成分
    };

    //------------------------------------------------------------------------
    //! float 4つの組です（色など）。
    //------------------------------------------------------------------------
    struct Float4 {
        float x = 0.0f;    // 1つ目の成分
        float y = 0.0f;    // 2つ目の成分
        float z = 0.0f;    // 3つ目の成分
        float w = 0.0f;    // 4つ目の成分
    };

    static_assert(sizeof(Float2) == sizeof(float) * 2, "Float2 must be tightly packed");
    static_assert(sizeof(Float3) == sizeof(float) * 3, "Float3 must be tightly packed");
    static_assert(sizeof(Float4) == sizeof(float) * 4, "Float4 must be tightly packed");

}    // namespace Tsukino::GraphicsCommon
