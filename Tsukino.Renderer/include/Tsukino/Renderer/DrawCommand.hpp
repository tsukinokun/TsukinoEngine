//------------------------------------------------------------
//! @file   DrawCommand.hpp
//! @brief  描画コマンド構造体の宣言
//! @author 山﨑愛
//------------------------------------------------------------
#pragma once
#include <Tsukino/Core/Math/Matrix.hpp>
#include <Tsukino/Core/typedef.hpp>
#include <Tsukino/Renderer/GpuHandles.hpp>
#include <Tsukino/Renderer/ShaderSlots.hpp>

// 名前空間 Tsukino::Renderer
namespace Tsukino::Renderer {
    // 前方宣言
    class Material;
    struct MeshBuffer;
    struct CBufferMaterial;
    struct CBufferScene;
    struct PipelineState;

    //------------------------------------------------------------
    //! @enum RenderPass
    //! @brief 描画パス
    //! @note  フォグはWorldの後・TransparentDepthの前に、深度バッファから画素の距離を求めて掛ける。
    //!        そのためWorldには「深度を書く」物だけを置く。深度を書かない物（半透明・加算の3D描画）を
    //!        Worldに置くと、その画素には奥の景色の距離ぶんのフォグが上から掛かり、背景によって消える。
    //!        深度を書かない3D描画はTransparentへ置き、自分の距離ぶんのフォグはシェーダーの中で掛ける
    //!        （FogPass::BindForwardFogがTransparentの間だけフォグ定数を張る）
    //------------------------------------------------------------
    enum class RenderPass : u8 {
        GBuffer,            // 不透明3Dモデル（ディファード）。G-Bufferへ書き込む
        World,              // 深度を書くフォワードの3D（デバッグ線など不透明）。フォグより前
        TransparentDepth,   // 半透明モデルの深度事前パス（Transparentの直前。色は書かず深度だけ埋める）
        Transparent,        // 深度を書かない3D（半透明モデル・ワールド空間スプライト・加算の光など）。フォグより後。
                            // 並べ替えはせず、積んだ順に描く（同じ場所に重ねる物は、下に敷く物から先に積めばよい）
        Overlay,            // フォント/UI
    };

    //------------------------------------------------------------
    //! @struct ClipRect
    //! @brief  描画を切り取る矩形（レンダーターゲットのピクセル。左上原点、右・下は含まない）
    //------------------------------------------------------------
    struct ClipRect {
        i32 left   = 0;
        i32 top    = 0;
        i32 right  = 0;
        i32 bottom = 0;
    };

    //------------------------------------------------------------
    //! @enum  DrawKind
    //! @brief 描画コマンドが何を描くか
    //------------------------------------------------------------
    enum class DrawKind : u8 {
        Mesh,       // material と mesh で描く（既定）
        Sprites,    // DrawCommandQueue::PushSprites で積んだスプライトの並び（文字など。画面のピクセルで描く）
        Lines,      // DrawCommandQueue::PushLines で積んだ線の頂点の列（2頂点で1本。デバッグ線など）
    };

    //------------------------------------------------------------
    //! @struct TexelRect
    //! @brief  テクスチャから切り出す矩形（テクセル。左上原点、右・下は含まない）
    //------------------------------------------------------------
    struct TexelRect {
        i32 left   = 0;
        i32 top    = 0;
        i32 right  = 0;
        i32 bottom = 0;
    };

    //------------------------------------------------------------
    //! @struct SpriteQuad
    //! @brief  スプライト1枚（テクスチャの一部を画面のピクセルへ貼る四角）
    //! @note   DrawKind::Sprites のコマンドが描く単位。並べた順に描かれ、
    //!         ブレンドは straight alpha（色にアルファを掛けていない）、深度は読むだけで書かない。
    //!         画面上の位置は、切り出した矩形の origin の点が (x, y) に来るように置き、
    //!         origin を軸に scale で拡大し、rotation で回す
    //------------------------------------------------------------
    struct SpriteQuad {
        TextureHandle             texture;                                // 貼るテクスチャ
        float                     x        = 0.0f;                        // 描く位置 X（ピクセル。左上原点）
        float                     y        = 0.0f;                        // 描く位置 Y（ピクセル。左上原点）
        TexelRect                 source;                                 // テクスチャから切り出す矩形
        float                     color[4] = {1.0f, 1.0f, 1.0f, 1.0f};    // テクスチャに掛ける色（RGBA）
        float                     rotation = 0.0f;                        // 回転（ラジアン。画面上で時計回りが正）
        float                     originX  = 0.0f;                        // 回転と拡大の軸 X（切り出した矩形の左上からのテクセル）
        float                     originY  = 0.0f;                        // 回転と拡大の軸 Y（切り出した矩形の左上からのテクセル）
        float                     scaleX   = 1.0f;                        // 横の拡大率
        float                     scaleY   = 1.0f;                        // 縦の拡大率
    };

    //------------------------------------------------------------
    //! @struct DrawCommand
    //! @brief  描画コマンドを表す構造体
    //------------------------------------------------------------
    struct DrawCommand {
        Material*                                 material;                            // どう描くか（シェーダー、テクスチャ、ステート）
        MeshBuffer*                               mesh;                                // 何を描くか（Quad, Cube, Model Mesh など）
        Tsukino::Core::Math::matrix               transform;                           // どこに描くか（モデル行列）
        RenderPass                                pass         = RenderPass::World;    // 描画パス
        CBufferMaterial*                          materialData = nullptr;              // マテリアルの定数データ（存在すれば）

        //--------------------------------------------------------
        // 描くものの種類。Sprites / Lines のコマンドは material と mesh を使わず、
        // DrawCommandQueue が持つスプライト・線の頂点の並びのうち
        // [batchFirst, batchFirst + batchCount) を描く。
        // この3つは DrawCommandQueue::PushSprites / PushLines が設定するので、手で書かない
        //--------------------------------------------------------
        DrawKind kind       = DrawKind::Mesh;    // 描くものの種類
        u32      batchFirst = 0;                 // キューの並びの中の先頭
        u32      batchCount = 0;                 // 描く数（スプライトの枚数、または線の頂点数）

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
        // ハッシュから毎フレーム計算する使い方（草など）では無効なハンドルのままでよい。
        // 作成・更新は RenderResources::CreateInstanceBuffer / UpdateInstanceBuffer を使う
        //--------------------------------------------------------
        u32          instanceCount = 1;    // 描画するインスタンス数（1 なら非インスタンス描画）
        BufferHandle instanceData;         // 1インスタンスあたりのデータ（頂点シェーダーの t スロットへバインドされる）

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
        // 網点（ディザ）で少しずつ消す・出すときの見えている割合。
        // 1 で普通に描き、0 に近いほど 4×4 の網点で多くの画素を抜く（0 で全部抜ける）。
        // 描画距離の端で物を急に出し入れせず、少しずつ現すために使う（LOD の切り替えも同じ）。
        // 半透明にはしないので、ディファードのまま描けて影とも両立する。
        // GBuffer パスとシャドウパスの両方で効く（GBuffer.ps.hlsl / ShadowMap.ps.hlsl が b1 の motionFlags.y から読む）
        //--------------------------------------------------------
        float ditherFade = 1.0f;

        //--------------------------------------------------------
        // 遮り抜きの対象か（FrameConstants::SetOcclusionCutout）。
        // true の物は、注目している物より手前にあって画面上の円の中に入る画素が網点で抜ける。
        // GBuffer パスだけで効き、影（シャドウパス）は抜かない（b1 の motionFlags.z で渡す）
        //--------------------------------------------------------
        bool occlusionCutout = false;

        //--------------------------------------------------------
        // 影を描くときに使うパイプライン。null なら組み込みの影用（ShadowMapStatic / ShadowMap）を使う。
        // 自前の頂点シェーダーで頂点を動かす物（風で揺れる木など）は、組み込みの影用では
        // 動かない形で影が描かれてしまう。同じ動きをする影用の頂点シェーダーで作った
        // パイプラインをここへ渡すと、影も本体と一緒に動く。
        // ピクセルシェーダーは組み込みの ShadowMap.ps.hlsl を使えば、くり抜きと網点フェードもそのまま効く。
        // 影の描画でも b1・b12（userConstantBuffer）は束ねられるので、本体と同じパラメータが読める
        //--------------------------------------------------------
        const PipelineState* shadowPipeline = nullptr;

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
        // 作成・更新は RenderResources::CreateUserConstantBuffer / UpdateUserConstantBuffer を使う
        //--------------------------------------------------------
        BufferHandle userConstantBuffer;                      // ゲーム定義の定数バッファ（不要なら無効なハンドル）
        CBSlot       userConstantSlot   = CBSlot::User0;      // バインド先（User0 / User1）

        //--------------------------------------------------------
        // モーションブラー用の前フレームデータ
        // hasPrevFrame が false のときは prevTransform / prevBoneMatrices を
        // 一切読まない（速度ゼロとして扱う）。前フレームの値は
        // MotionVectorSnapshotSystem がフレーム先頭で退避したものを指す。
        //--------------------------------------------------------
        Tsukino::Core::Math::matrix prevTransform;               // 前フレームのモデル行列
        const void*                 prevBoneMatrices = nullptr;  // 前フレームのボーン行列配列（boneCount と同じ本数）
        bool                        hasPrevFrame     = false;    // 前フレームの値が有効か

        //--------------------------------------------------------
        // 切り取り（シザー）。
        // hasClipRect が true のコマンドは clipRect の外に描かれない（スクロールする UI の枠など）。
        // 既定は false なので、既存の描画コマンドは今までどおり全体に描かれる。
        // DrawKind によらず、どの種類のコマンドにも効く
        //--------------------------------------------------------
        bool     hasClipRect = false;    // clipRect で切り取るか
        ClipRect clipRect;               // 切り取る矩形

        //--------------------------------------------------------
        // カメラの差し替え（Overlay パスに 3D モデルを積むとき。ScreenModelComponent 参照）。
        // nullptr でないコマンドは、b0 のカメラ（view / projection / viewProj / invViewProj / cameraPos）を
        // これに替えて描く。ライトなどほかの値はワールドのまま使う。
        // Overlay パスでこれを持つコマンドが来たときだけ深度バッファを使う（その最初の1回で深度を消す）。
        // 既定は nullptr なので、既存の描画コマンドは今までどおり描かれる。
        // 実体は DrawCommandQueue::AllocSceneData() で確保する
        //--------------------------------------------------------
        const CBufferScene* cameraOverride = nullptr;    // 差し替えるカメラ（不要なら nullptr）
    };
}    // namespace Tsukino::Renderer
