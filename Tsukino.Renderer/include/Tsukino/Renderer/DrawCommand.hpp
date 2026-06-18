//------------------------------------------------------------
//! @file   DrawCommand.hpp
//! @brief  描画コマンド構造体の宣言
//! @author 山﨑愛
//------------------------------------------------------------
#pragma once
#include <Tsukino/Core/Math/Matrix.hpp>
#include <Tsukino/Core/typedef.hpp>
// 名前空間 Tsukino::Renderer
namespace Tsukino::Renderer {
    // 前方宣言
    class Material;
    struct MeshBuffer;
    struct CBufferMaterial;

    //------------------------------------------------------------
    //! @enum RenderPass
    //! @brief 描画パス
    //------------------------------------------------------------
    enum class RenderPass : u8 {
        World,          // 3D/通常スプライト
        Transparent,    // 透明オブジェクト
        Water,          // 
        Overlay,        // フォント/UI
    };

    //------------------------------------------------------------
    //! @struct DrawCommand
    //! @brief  描画コマンドを表す構造体
    //------------------------------------------------------------
    struct DrawCommand {
        Material*                                 material;                            // どう描くか（シェーダー、テクスチャ、ステート）
        MeshBuffer*                               mesh;                                // 何を描くか（Quad, Cube, Model Mesh など）
        std::function<void(ID3D11DeviceContext*)> customDraw;                          // カスタム描画関数（nullptr なら通常描画）
        Tsukino::Core::Math::matrix               transform;                           // どこに描くか（モデル行列）
        RenderPass                                pass         = RenderPass::World;    // 描画パス
        CBufferMaterial*                          materialData = nullptr;              // マテリアルの定数データ（存在すれば）

        const void* boneMatrices = nullptr;    // ボーン行列の配列へのポインタ（スキニング用, 最大ボーン数は SkeletonOutputComponent 等に依存）
        u32         boneCount    = 0;          // スキニング用のボーン数
    };
}    // namespace Tsukino::Renderer
