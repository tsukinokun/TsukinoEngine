//----------------------------------------------------------------------------
//! @file   DebugDraw.hpp
//! @brief  デバッグ用の線と三角形の積み場の宣言
//! @detail 1フレームの間に積まれた線と三角形の頂点を溜め、Submit() で線の描画コマンドとして
//!         DrawCommandQueue へ渡します。三角形は3本の辺（ワイヤーフレーム）として描かれます。
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/Renderer/DrawCommand.hpp>

#include <Tsukino/GraphicsCommon/Vertex/DebugVertex.hpp>

#include <vector>

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {
    class DrawCommandQueue;    // 前方宣言

    //------------------------------------------------------------------------
    //! デバッグ用の線と三角形を溜めておくクラスです。
    //! @note  Renderer が1つだけ所有し、Renderer::GetDebugDraw() で借りる。
    //!        積んだ頂点は Submit() を呼ぶと DrawKind::Lines のコマンドとして描画キューへ積まれる
    //!        （PhysicsSystem が衝突形状を積んで World パスへ渡している）。
    //!        Submit() されずに残った頂点は、Renderer::Render() の終わりに捨てられる
    //------------------------------------------------------------------------
    class DebugDraw {
    public:

        //! 線を1本積みます。
        //! @param  [in] v1 始点
        //! @param  [in] v2 終点
        void DrawLine(const Tsukino::GraphicsCommon::DebugVertex& v1, const Tsukino::GraphicsCommon::DebugVertex& v2);

        //! 三角形を1枚積みます（3本の辺として描かれます）。
        //! @param  [in] v1 頂点1
        //! @param  [in] v2 頂点2
        //! @param  [in] v3 頂点3
        void DrawTriangle(const Tsukino::GraphicsCommon::DebugVertex& v1,
                          const Tsukino::GraphicsCommon::DebugVertex& v2,
                          const Tsukino::GraphicsCommon::DebugVertex& v3);

        //! 積んだ線と三角形を、線の描画コマンドとして描画キューへ積み、空にします。
        //! @param  [in,out] queue 積む先の描画キュー（Renderer::GetDrawQueue()）
        //! @param  [in]     pass  描くパス（既定は World。深度テストありで不透明に描かれる）
        void Submit(DrawCommandQueue& queue, RenderPass pass = RenderPass::World);

        //! 積んだ線と三角形を描画せずに捨てます。
        void Clear();

    private:
        std::vector<Tsukino::GraphicsCommon::DebugVertex> m_lineVertices;    // 積まれた線の頂点（2頂点で1本。三角形は辺に分けてここへ積む）
    };
}    // namespace Tsukino::Renderer
