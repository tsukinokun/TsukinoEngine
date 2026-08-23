//--------------------------------------------------------------
//! @file   DepthMode.hpp
//! @brief  深度モードの列挙型の宣言
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {
    //--------------------------------------------------------------
    //! @enum  DepthMode
    //! @brief 深度モードの列挙型
    //--------------------------------------------------------------
    enum class DepthMode {
        None,         // 深度テストなし（2D）
        ReadWrite,    // 深度読み書き（3D）
        ReadOnly,     // 深度読み取りのみ（透明物など）
        EqualReadOnly, // 深度が等しい画素のみ通す・書き込みなし（半透明フォワードの色パス用。先に埋めたTransparentDepthと組で使う）
    };

}    // namespace Tsukino::Renderer
