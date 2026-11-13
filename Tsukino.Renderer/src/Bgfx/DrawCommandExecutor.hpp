//----------------------------------------------------------------------------
//! @file   DrawCommandExecutor.hpp
//! @brief  bgfx 版の、描画コマンド1本を GPU へ流す処理の宣言
//! @detail Tsukino.Renderer の内部専用ヘッダです。DX11 版と同じく、メッシュ・スプライトの並び
//!         （文字など）・線の頂点の列（デバッグ線）を描きます。描く先は bgfx のビュー番号で受け取ります。
//!         スプライトは DX11 版の DirectXTK SpriteBatch の代わりに、四角の頂点を CPU で作って描きます。
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/Renderer/Renderer.hpp>
#include <Tsukino/Renderer/Bgfx/UniformBlock.hpp>

#include <bgfx/bgfx.h>

#include <array>

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {

    //------------------------------------------------------------------------
    //! bgfx のビューの番号です（小さい番号から順に描かれる）。
    //! @note  パスを bgfx へ移すたびに、ここへ足していく
    //------------------------------------------------------------------------
    namespace BgfxView {
        inline constexpr bgfx::ViewId World   = 0;    // 3D（今はバックバッファへ直接描く）
        inline constexpr bgfx::ViewId Overlay = 1;    // UI と文字
    }    // namespace BgfxView

    //------------------------------------------------------------------------
    //! 描画コマンド1本を GPU へ流すクラスです（bgfx 版）。
    //! @note  Renderer が1つだけ所有する。描画の資源とシーン定数は Renderer の他の部品から
    //!        借りており、それらはこのクラスより長生きする
    //------------------------------------------------------------------------
    class DrawCommandExecutor {
    public:
        //! ユニフォームとシェーダーを破棄します。
        ~DrawCommandExecutor();

        //! ユニフォームと、スプライト・線を描くシェーダーを作成します。
        //! @param  [in] graphicsContext 描画領域の大きさの取得元
        //! @param  [in] resources       テクスチャ・メッシュ・パイプラインの取得元
        //! @param  [in] frameConstants  シーン定数の取得元
        //! @param  [in] drawQueue       スプライト・線の頂点の並びの取得元
        //! @param  [in] lineVS          線を描く頂点シェーダー（bgfx の形式）
        //! @param  [in] linePS          線を描くピクセルシェーダー（bgfx の形式）
        //! @return true: 作成成功, false: 作成失敗
        [[nodiscard]]
        bool Initialize(const GraphicsContext&             graphicsContext,
                        RenderResources&                   resources,
                        FrameConstants&                    frameConstants,
                        const DrawCommandQueue&            drawQueue,
                        const Tsukino::Asset::ShaderAsset* lineVS,
                        const Tsukino::Asset::ShaderAsset* linePS);

        //! 描画コマンドを実行します。
        //! @param  [in]     cmd   実行する描画コマンド
        //! @param  [in]     view  描く先のビュー
        //! @param  [in,out] stats 描画統計の加算先
        void Execute(const DrawCommand& cmd, bgfx::ViewId view, Renderer::FrameStats& stats);

    private:
        //! DrawKind::Mesh のコマンドを描きます。
        //! @param  [in]     cmd   実行する描画コマンド
        //! @param  [in]     view  描く先のビュー
        //! @param  [in,out] stats 描画統計の加算先
        void ExecuteMesh(const DrawCommand& cmd, bgfx::ViewId view, Renderer::FrameStats& stats);

        //! DrawKind::Sprites のコマンドを描きます。
        //! @param  [in]     cmd   実行する描画コマンド
        //! @param  [in]     view  描く先のビュー
        //! @param  [in,out] stats 描画統計の加算先
        void ExecuteSprites(const DrawCommand& cmd, bgfx::ViewId view, Renderer::FrameStats& stats);

        //! DrawKind::Lines のコマンドを描きます。
        //! @param  [in]     cmd   実行する描画コマンド
        //! @param  [in]     view  描く先のビュー
        //! @param  [in,out] stats 描画統計の加算先
        void ExecuteLines(const DrawCommand& cmd, bgfx::ViewId view, Renderer::FrameStats& stats);

        //! コマンドに切り取りがあれば、次の Submit にシザーを設定します。
        //! @param  [in] cmd 描画コマンド
        static void ApplyClipRect(const DrawCommand& cmd);

        //! パスごとの描画数を統計へ加えます。
        //! @param  [in]     pass  描いたパス
        //! @param  [in,out] stats 描画統計の加算先
        static void CountDrawCall(RenderPass pass, Renderer::FrameStats& stats);

        //! 変換済みのシェーダーを2本読み込んで、プログラムを作ります。
        //! @param  [in] vsName 頂点シェーダーの名前（"SpriteBatch.vs" のように拡張子を除いた名前）
        //! @param  [in] psName ピクセルシェーダーの名前
        //! @return プログラム（読めなければ無効なハンドル）
        static bgfx::ProgramHandle LoadProgram(const char* vsName, const char* psName);

        //! シェーダーアセット2本（bgfx の形式）から、プログラムを作ります。
        //! @param  [in] vs 頂点シェーダー
        //! @param  [in] ps ピクセルシェーダー
        //! @return プログラム（bgfx の形式でなければ無効なハンドル）
        static bgfx::ProgramHandle CreateProgram(const Tsukino::Asset::ShaderAsset* vs, const Tsukino::Asset::ShaderAsset* ps);

    private:
        const GraphicsContext*  m_graphicsContext = nullptr;    // 描画領域の大きさ（借りている）
        RenderResources*        m_resources       = nullptr;    // テクスチャ・メッシュ・パイプライン（借りている）
        FrameConstants*         m_frameConstants  = nullptr;    // シーン定数（借りている）
        const DrawCommandQueue* m_drawQueue       = nullptr;    // スプライト・線の頂点の並び（借りている）

        UniformBlock<CBufferTransform> m_transformUniform;    // 物ごとの定数（u_transform。DX11 版の b1）
        CBufferMaterial                m_lastMaterial{};      // 最後に送ったマテリアルの定数（materialData の無いコマンドが引き継ぐ）
        UniformBlock<CBufferMaterial>  m_materialUniform;     // マテリアルの定数（u_material。DX11 版の b2）

        //! マテリアルのテクスチャ（t0〜t4）のサンプラーのユニフォーム。シェーダーの SAMPLER2D の名前と揃える
        std::array<bgfx::UniformHandle, 5> m_materialSamplers{};

        bgfx::UniformHandle m_spriteSampler = BGFX_INVALID_HANDLE;    // スプライトの並びのテクスチャ（s_texture）
        bgfx::ProgramHandle m_spriteProgram = BGFX_INVALID_HANDLE;    // スプライトの並びを描くプログラム
        bgfx::ProgramHandle m_lineProgram   = BGFX_INVALID_HANDLE;    // 線を描くプログラム
        bgfx::VertexLayout  m_spriteLayout;                           // スプライトの頂点（位置・色・UV）
        bgfx::VertexLayout  m_lineLayout;                             // 線の頂点（DebugVertex）
    };

}    // namespace Tsukino::Renderer
