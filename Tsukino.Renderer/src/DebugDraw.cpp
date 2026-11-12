//----------------------------------------------------------------------------
//! @file   DebugDraw.cpp
//! @brief  デバッグ用の線と三角形の積み場の実装
//----------------------------------------------------------------------------
#include <Tsukino/Renderer/DebugDraw.hpp>
#include <Tsukino/Renderer/DrawCommandQueue.hpp>

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {
    //------------------------------------------------------------------------
    //! 線を1本積みます。
    //------------------------------------------------------------------------
    void DebugDraw::DrawLine(const Tsukino::GraphicsCommon::DebugVertex& v1, const Tsukino::GraphicsCommon::DebugVertex& v2) {
        m_lineVertices.push_back(v1);
        m_lineVertices.push_back(v2);
    }

    //------------------------------------------------------------------------
    //! 三角形を1枚積みます（3本の辺として描かれます）。
    //------------------------------------------------------------------------
    void DebugDraw::DrawTriangle(const Tsukino::GraphicsCommon::DebugVertex& v1,
                                 const Tsukino::GraphicsCommon::DebugVertex& v2,
                                 const Tsukino::GraphicsCommon::DebugVertex& v3) {
        // ワイヤーフレームで塗るのと同じ見た目になるよう、3本の辺を線として積む
        DrawLine(v1, v2);
        DrawLine(v2, v3);
        DrawLine(v3, v1);
    }

    //------------------------------------------------------------------------
    //! 積んだ線と三角形を、線の描画コマンドとして描画キューへ積み、空にします。
    //------------------------------------------------------------------------
    void DebugDraw::Submit(DrawCommandQueue& queue, RenderPass pass) {
        DrawCommand cmd{};
        cmd.pass = pass;
        queue.PushLines(cmd, m_lineVertices);
        m_lineVertices.clear();
    }

    //------------------------------------------------------------------------
    //! 積んだ線と三角形を描画せずに捨てます。
    //------------------------------------------------------------------------
    void DebugDraw::Clear() {
        m_lineVertices.clear();
    }
}    // namespace Tsukino::Renderer
