//--------------------------------------------------------------
//! @file   ShaderSlots.hpp
//! @brief  シェーダーへ渡すテクスチャと定数の枠
//! @detail 描画 API のレジスタ番号ではなく、役割で枠を表します。
//!         どの番号で GPU へ渡すかは Renderer の中（bgfx 版は TextureStages.hpp）が決めます。
//--------------------------------------------------------------
#pragma once
#include <Tsukino/Core/typedef.hpp>

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {

    //--------------------------------------------------------------
    //! @brief  マテリアルが持つテクスチャの枠
    //! @note   シェーダーは同じ順番で受ける（Shaders/bgfx/TextureStages.sh）
    //--------------------------------------------------------------
    enum class MaterialTextureSlot : u8 {
        Albedo = 0,           //!< アルベド
        Normal,               //!< 法線マップ
        MetallicRoughness,    //!< メタリック・ラフネス（G = roughness、B = metallic）
        Emissive,             //!< 発光
        AO,                   //!< アンビエントオクルージョン
        Count,                //!< 枠の数
    };

    //--------------------------------------------------------------
    //! @brief  ゲームが自前のシェーダーへ渡す定数の枠
    //! @note   エンジンはこの枠を使わないので、ゲームが何を入れても組み込みの描画とは衝突しない。
    //!         シェーダーは uniform vec4 u_user0[N] / u_user1[N] で受ける
    //--------------------------------------------------------------
    enum class UserUniformSlot : u8 {
        User0 = 0,    //!< 1本目（u_user0）
        User1,        //!< 2本目（u_user1）
        Count,        //!< 枠の数
    };
}    // namespace Tsukino::Renderer
