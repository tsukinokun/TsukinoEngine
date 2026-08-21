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
        GBuffer,        // 不透明3Dモデル（ディファード）。G-Bufferへ書き込む
        World,          // 3D（デバッグ線などcustomDraw経由のフォワード不透明）/通常スプライト
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

        //--------------------------------------------------------
        // モーションブラー用の前フレームデータ
        // hasPrevFrame が false のときは prevTransform / prevBoneMatrices を
        // 一切読まない（速度ゼロとして扱う）。前フレームの値は
        // MotionVectorSnapshotSystem がフレーム先頭で退避したものを指す。
        //--------------------------------------------------------
        Tsukino::Core::Math::matrix prevTransform;               // 前フレームのモデル行列
        const void*                 prevBoneMatrices = nullptr;  // 前フレームのボーン行列配列（boneCount と同じ本数）
        bool                        hasPrevFrame     = false;    // 前フレームの値が有効か
    };
}    // namespace Tsukino::Renderer
