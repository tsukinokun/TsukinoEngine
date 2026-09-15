//----------------------------------------------------------------------------
//! @file   DrawCommandExecutor.hpp
//! @brief  描画コマンド1本を GPU へ流す処理の宣言
//! @detail Tsukino.Renderer の内部専用ヘッダです。Transform・マテリアル・ボーン行列の
//!         定数バッファを持ち、DrawCommand の内容をバインドして描画し、描画統計を積みます。
//!         通常のパス用と、シャドウマップ用の2通りの実行を持ちます。
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/Renderer/Renderer.hpp>

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {
    //------------------------------------------------------------------------
    //! 描画コマンド1本を GPU へ流すクラスです。
    //! @note  Renderer が1つだけ所有する。デバイス・共有資源・シーン定数は Renderer の
    //!        他の部品から借りており、それらはこのクラスより長生きする
    //------------------------------------------------------------------------
    class DrawCommandExecutor {
    public:

        //! 定数バッファを作成します。
        //! @param  [in] graphicsContext デバイスとパイプラインの設定先
        //! @param  [in] resources       共通ステートとサンプラーの取得元
        //! @param  [in] frameConstants  b0 の定数バッファの取得元
        //! @return true: 作成成功, false: 作成失敗
        [[nodiscard]]
        bool Initialize(GraphicsContext& graphicsContext, RenderResources& resources, FrameConstants& frameConstants);

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

    private:
        GraphicsContext* m_graphicsContext = nullptr;    // デバイスとパイプラインの設定先（借りている）
        RenderResources* m_resources       = nullptr;    // 共通ステートとサンプラー（借りている）
        FrameConstants*  m_frameConstants  = nullptr;    // b0 の定数バッファ（借りている）

        ComPtr<ID3D11Buffer> m_objectBuffer;          // Transform 用定数バッファ (b1)
        ComPtr<ID3D11Buffer> m_materialBuffer;        // マテリアル用定数バッファ (b2)
        ComPtr<ID3D11Buffer> m_skinningBuffer;        // ボーン行列用定数バッファ (b3)
        ComPtr<ID3D11Buffer> m_prevSkinningBuffer;    // 前フレームのボーン行列用定数バッファ (b7)
    };
}    // namespace Tsukino::Renderer
