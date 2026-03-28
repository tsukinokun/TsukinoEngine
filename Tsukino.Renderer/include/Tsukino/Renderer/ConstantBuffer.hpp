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
    //! @struct CBufferScene
    //! @brief  スロット0 (b0) 用：フレーム内で全オブジェクト共通のデータ
    //--------------------------------------------------------------
    struct CBufferScene {
        Tsukino::Core::Math::matrix view;
        Tsukino::Core::Math::matrix projection;
        Tsukino::Core::Math::matrix viewProj;
    };

    //--------------------------------------------------------------
    //! @struct CBufferTransform
    //! @brief  スロット1 (b1) 用：オブジェクトごとの固有データ
    //--------------------------------------------------------------
    struct CBufferTransform {
        Tsukino::Core::Math::matrix mvp;
    };
}    // namespace Tsukino::Renderer
