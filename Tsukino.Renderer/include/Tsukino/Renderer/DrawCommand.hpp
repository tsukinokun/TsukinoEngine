//------------------------------------------------------------
//! @file   DrawCommand.hpp
//! @brief  描画コマンド構造体の宣言
//! @author 山﨑愛
//------------------------------------------------------------
#pragma once
// windows.h の min/max マクロを避けるため、DX11 / Effekseer より先に通す
#include <Tsukino/Core/WindowsLean.hpp>
#include <Tsukino/Core/Math/Matrix.hpp>
#include <Tsukino/Core/typedef.hpp>
#include <Tsukino/Renderer/ShaderSlots.hpp>

#include <d3d11.h>
#include <functional>
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
        GBuffer,            // 不透明3Dモデル（ディファード）。G-Bufferへ書き込む
        World,              // 3D（デバッグ線などcustomDraw経由のフォワード不透明）/通常スプライト
        TransparentDepth,   // 半透明モデルの深度事前パス（Transparentの直前。色は書かず深度だけ埋める）
        Transparent,        // 透明オブジェクト
        Overlay,            // フォント/UI
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

        //! 同一パス内の描画順（小さいほど先に描く＝奥）。
        //! 現状これを見て並べ替えるのは Overlay パスだけで、他のパスは
        //! 積んだ順のまま実行される。3Dパスの前後は深度バッファが決めるため、
        //! 順序キーを持ち込むと「深度とキーのどちらが正か」が二重定義になる。
        //! Overlay は深度を使わないので、明示的な層の指定がここにしか無い
        int sortOrder = 0;

        const void* boneMatrices = nullptr;    // ボーン行列の配列へのポインタ（スキニング用, 最大ボーン数は SkeletonOutputComponent 等に依存）
        u32         boneCount    = 0;          // スキニング用のボーン数

        //--------------------------------------------------------
        // インスタンス描画。
        // instanceCount が 1 のときは従来どおり DrawIndexed を通るため、
        // 既存の描画コマンドは1つも影響を受けない（既定値が 1 なのはそのため）。
        //
        // 1インスタンスあたりのデータは頂点バッファではなく StructuredBuffer で
        // 渡す。頂点バッファ方式にすると入力レイアウトをフォーマットごとに
        // 増やす羽目になり、VertexFormat が PipelineKey に含まれている以上、
        // パイプラインのキャッシュが倍々に膨らむため。
        //
        // instanceData は必須ではない。インスタンスごとの値を SV_InstanceID の
        // ハッシュから毎フレーム計算する使い方（草など）では nullptr のままでよい。
        //--------------------------------------------------------
        u32                       instanceCount = 1;          // 描画するインスタンス数（1 なら非インスタンス描画）
        ID3D11ShaderResourceView* instanceData  = nullptr;    // 1インスタンスあたりのデータ（頂点シェーダーの t スロットへバインドされる）

        //--------------------------------------------------------
        // 影を落とすか。
        // シャドウパスは GBuffer のコマンドを固定のシャドウ用シェーダーで
        // 描き直すため、頂点シェーダーが独自に頂点を組み立てるオブジェクト
        // （草など）は、そのままでは形が再現できずに壊れた影が出る。
        // そういうコマンドはここを false にしてシャドウパスから外す。
        //
        // 既定は true なので、既存のコマンドは今までどおり影を落とす。
        //--------------------------------------------------------
        bool castsShadow = true;    // false ならシャドウパスをスキップする

        //--------------------------------------------------------
        // ゲーム定義の定数バッファ。
        // ゲームが自前のシェーダーで描くとき、そのシェーダーへ
        // パラメータを渡すための口。VS / PS の両方へバインドされる。
        //
        // エンジンが使う b0〜b9 とは別枠（CBSlot::User0 / User1）なので、
        // ゲームが何を入れてもエンジン側の描画とは衝突しない。
        // 時間や解像度は b0（CBufferScene）から取れるので、それだけで
        // 足りる演出はこの枠を使う必要すら無い。
        //
        // 作成・更新は DX11/UserConstantBuffer.hpp のヘルパを使う
        //--------------------------------------------------------
        ID3D11Buffer* userConstantBuffer = nullptr;             // ゲーム定義の定数バッファ（不要なら nullptr）
        CBSlot        userConstantSlot   = CBSlot::User0;       // バインド先（User0 / User1）

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
