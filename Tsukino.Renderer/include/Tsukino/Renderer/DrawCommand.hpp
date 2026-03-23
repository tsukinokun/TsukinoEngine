//------------------------------------------------------------
//! @file   DrawCommand.hpp
//! @brief  描画コマンド構造体の宣言
//! @author 山﨑愛
//------------------------------------------------------------
#pragma once
#include <Tsukino/Core/Math/Matrix.hpp>
// 名前空間 Tsukino::Renderer
namespace Tsukino::Renderer {
    // 前方宣言
    class Material;
    struct MeshBuffer;
    //------------------------------------------------------------
    //! @struct DrawCommand
    //! @brief  描画コマンドを表す構造体
    //------------------------------------------------------------
    struct DrawCommand {
        Material*         material;     // どう描くか（シェーダー、テクスチャ、ステート）
        MeshBuffer*       mesh;         // 何を描くか（Quad, Cube, Model Mesh など）
        Tsukino::Core::Math::matrix transform;    // どこに描くか（モデル行列）
    };
}    // namespace Tsukino::Renderer
