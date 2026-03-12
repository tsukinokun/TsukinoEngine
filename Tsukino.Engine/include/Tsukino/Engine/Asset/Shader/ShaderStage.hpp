//--------------------------------------------------------------
//! @file   ShaderStage.hpp
//! @brief  シェーダーステージの定義
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
// 名前空間 Tsukino::Shader
namespace Tsukino::Shader {
    //--------------------------------------------------------------
    //! @enum  ShaderStage
    //! @brief シェーダーステージ種別
    //! @note  アセット内のシェーダーステージを識別するための列挙型
    //--------------------------------------------------------------
    enum class ShaderStage {
        Unknown = 0,
        Vertex,      // VS
        Pixel,       // PS
        Compute,     // CS
        Geometry,    // GS
        Hull,        // HS
        Domain       // DS
    };
}    // namespace Tsukino::Shader
