//----------------------------------------------------------------------------
//! @file   DrawCommandExecutor.hpp
//! @brief  描画コマンド1本を GPU へ流す処理の宣言
//! @detail Tsukino.Renderer の内部専用ヘッダです。Transform・マテリアル・ボーン行列の
//!         定数バッファを持ち、DrawCommand の内容をバインドして描画し、描画統計を積みます。
//!         通常のパス用と、シャドウマップ用の2通りの実行を持ちます。
//!         通常のパスでは、メッシュのほかにスプライトの並び（文字など）と線の頂点の列（デバッグ線）も描きます。
//----------------------------------------------------------------------------
#pragma once
#include <wrl/client.h>
#include <Tsukino/Renderer/Renderer.hpp>

#include <SpriteBatch.h>

#include <memory>

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {
    using Microsoft::WRL::ComPtr;    // Renderer の内部用ヘッダなので、名前空間の中で使えるようにしておく

    //------------------------------------------------------------------------
    //! 描画コマンド1本を GPU へ流すクラスです。
    //! @note  Renderer が1つだけ所有する。デバイス・共有資源・シーン定数は Renderer の
    //!        他の部品から借りており、それらはこのクラスより長生きする
    //------------------------------------------------------------------------
    class DrawCommandExecutor {
    public:

        //! 定数バッファと、スプライト・線を描く資源を作成します。
        //! @param  [in] graphicsContext デバイスとパイプラインの設定先
        //! @param  [in] resources       共通ステートとサンプラーの取得元
        //! @param  [in] frameConstants  b0 の定数バッファの取得元
        //! @param  [in] drawQueue       スプライト・線の頂点の並びの取得元
        //! @param  [in] lineVS          線を描く頂点シェーダー
        //! @param  [in] linePS          線を描くピクセルシェーダー
        //! @return true: 作成成功, false: 作成失敗
        [[nodiscard]]
        bool Initialize(GraphicsContext&                   graphicsContext,
                        RenderResources&                   resources,
                        FrameConstants&                    frameConstants,
                        const DrawCommandQueue&            drawQueue,
                        const Tsukino::Asset::ShaderAsset* lineVS,
                        const Tsukino::Asset::ShaderAsset* linePS);

        //! 通常のパスの描画コマンドを実行します。
        //! @param  [in]     cmd               実行する描画コマンド
        //! @param  [in]     motionBlurEnabled このフレームでモーションブラーが有効か（速度を書き出すかの判定に使う）
        //! @param  [in,out] stats             描画統計の加算先
        void Execute(const DrawCommand& cmd, bool motionBlurEnabled, Renderer::FrameStats& stats);

        //! シャドウマップへ深度を書き込む描画コマンドを実行します。
        //! @param  [in]     cmd              実行する描画コマンド
        //! @param  [in]     staticPipeline   スタティックメッシュ用のシャドウパイプライン
        //! @param  [in]     skeletalPipeline スケルタルメッシュ用のシャドウパイプライン
        //! @param  [in,out] stats            描画統計の加算先
        void ExecuteShadow(const DrawCommand&     cmd,
                           const PipelineState*   staticPipeline,
                           const PipelineState*   skeletalPipeline,
                           Renderer::FrameStats&  stats);

    private:

        //! DrawKind::Sprites のコマンドを描きます。
        //! @param  [in]     cmd   実行する描画コマンド
        //! @param  [in,out] stats 描画統計の加算先
        void ExecuteSprites(const DrawCommand& cmd, Renderer::FrameStats& stats);

        //! DrawKind::Lines のコマンドを描きます。
        //! @param  [in]     cmd   実行する描画コマンド
        //! @param  [in,out] stats 描画統計の加算先
        void ExecuteLines(const DrawCommand& cmd, Renderer::FrameStats& stats);

        //! コマンドの切り取りを設定し、使うラスタライザを返します。
        //! @param  [in] cmd      描画コマンド
        //! @param  [in] fallback 切り取らないときのラスタライザ
        //! @return 切り取るときはシザー有効のラスタライザ、切り取らないときは fallback
        ID3D11RasterizerState* ApplyClipRect(const DrawCommand& cmd, ID3D11RasterizerState* fallback);

        //! スプライト・線を描いた後に、メッシュの描画やパスが前提とするステートへ戻します。
        //! @note   SpriteBatch はブレンド・深度・ラスタライザ・s0 のサンプラーを書き換えたまま返る。
        //!         戻さないと、ステートを自分で張らないパス（フルスクリーンの処理など）が崩れる
        void RestoreStateAfterBatch();

        //! パスごとの描画数を統計へ加えます。
        //! @param  [in]     pass  描いたパス
        //! @param  [in,out] stats 描画統計の加算先
        static void CountDrawCall(RenderPass pass, Renderer::FrameStats& stats);

        //! ボーン行列を定数バッファへ転送します。
        //! @param  [in] buffer       転送先の定数バッファ（DYNAMIC であること）
        //! @param  [in] boneMatrices ボーン行列の配列
        //! @param  [in] boneCount    ボーン数
        //! @return 実際に転送したバイト数（統計用）
        u32 UploadBoneMatrices(ID3D11Buffer* buffer, const void* boneMatrices, u32 boneCount);

        //! インスタンスごとのデータを頂点シェーダーへバインドします。
        //! @param  [in] srv バインドする SRV。nullptr ならスロットを明示的に空にする
        //! @note   スロットを空にする処理が要るのは、インスタンス描画をしない
        //!         コマンドが直前のコマンドの SRV を引き継いでしまうのを防ぐため
        void BindInstanceData(ID3D11ShaderResourceView* srv);

        //! ゲーム定義の定数バッファをバインドします。
        //! @param  [in] buffer バインドするバッファ。nullptr ならスロットを空にする
        //! @param  [in] slot   バインド先。User0 / User1 以外は無視する
        //! @note   エンジンが使う b0〜b9 を上書きされると描画が壊れるため、ゲーム予約枠以外は弾く
        void BindUserConstantBuffer(ID3D11Buffer* buffer, CBSlot slot);

        //! 頂点バッファとインデックスバッファをセットします。
        //! @param  [in] mesh       描画するメッシュ
        //! @param  [in] isSkeletal ボーンウェイトを使うか
        void BindMesh(const MeshBuffer& mesh, bool isSkeletal);

        //! 描画します（インスタンス数に応じて DrawIndexed と DrawIndexedInstanced を切り替えます）。
        //! @param  [in] cmd 描画するコマンド
        void Draw(const DrawCommand& cmd);

        //! シェーダー側のボーン配列の宣言数（CBufferSkinning::bones と揃えること）
        static constexpr u32 kMaxBoneCount = 128;

        //! 線の頂点バッファに1回で入る頂点数（超える分は何回かに分けて描く。偶数であること）
        static constexpr u32 kMaxLineVertices = 50000;

    private:
        GraphicsContext*        m_graphicsContext = nullptr;    // デバイスとパイプラインの設定先（借りている）
        RenderResources*        m_resources       = nullptr;    // 共通ステートとサンプラー（借りている）
        FrameConstants*         m_frameConstants  = nullptr;    // b0 の定数バッファ（借りている）
        const DrawCommandQueue* m_drawQueue       = nullptr;    // スプライト・線の頂点の並び（借りている）

        ComPtr<ID3D11Buffer> m_objectBuffer;          // Transform 用定数バッファ (b1)
        ComPtr<ID3D11Buffer> m_materialBuffer;        // マテリアル用定数バッファ (b2)
        ComPtr<ID3D11Buffer> m_skinningBuffer;        // ボーン行列用定数バッファ (b3)
        ComPtr<ID3D11Buffer> m_prevSkinningBuffer;    // 前フレームのボーン行列用定数バッファ (b7)

        std::unique_ptr<DirectX::SpriteBatch> m_spriteBatch;    // スプライトを描く（DrawKind::Sprites）

        ComPtr<ID3D11Buffer>       m_lineVB;             // 線の動的頂点バッファ（DrawKind::Lines）
        ComPtr<ID3D11VertexShader> m_lineVS;             // 線の頂点シェーダー
        ComPtr<ID3D11PixelShader>  m_linePS;             // 線のピクセルシェーダー
        ComPtr<ID3D11InputLayout>  m_lineInputLayout;    // 線の入力レイアウト
    };
}    // namespace Tsukino::Renderer
