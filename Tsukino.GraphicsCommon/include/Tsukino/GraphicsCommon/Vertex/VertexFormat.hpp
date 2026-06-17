//--------------------------------------------------------------
//! @file   VertexFormat.hpp
//! @brief  GPU に渡す頂点フォーマットの種類を定義
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
// 名前空間 : Tsukino::GraphicsCommon
namespace Tsukino::GraphicsCommon {
    //--------------------------------------------------------------
    //! @enum  VertexFormat
    //! @brief 頂点フォーマットの種類を定義する列挙型
    //--------------------------------------------------------------
    enum class VertexFormat {
        Unknown = 0,         // 不明なフォーマット
        PositionUV,          // 静的メッシュ
        PositionNormalUV,    // 通常の 3D Mesh
        Skinned,             // スキニング（アニメーション）
        Sprite,              // Sprite
        Particle,            // パーティクルなど
    };

}    // namespace Tsukino::GraphicsCommon
