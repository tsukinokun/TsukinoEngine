//----------------------------------------------------------------------------
//! @file   DebugDraw.hpp
//! @brief  デバッグ用の線と三角形の描画の宣言
//! @detail 1フレームの間に積まれた線と三角形の頂点を溜め、Flush() でまとめて描画します。
//!         三角形はワイヤーフレームで描きます。
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/Core/WindowsLean.hpp>

#include <Tsukino/GraphicsCommon/Vertex/DebugVertex.hpp>

#include <wrl/client.h>
#include <d3d11.h>

#include <vector>

namespace Tsukino::Asset {
    class ShaderAsset;    // 前方宣言
}

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {
    class GraphicsContext;    // 前方宣言
    class RenderResources;    // 前方宣言
    class FrameConstants;     // 前方宣言

    //------------------------------------------------------------------------
    //! デバッグ用の線と三角形を描画するクラスです。
    //! @note  Renderer が1つだけ所有し、Renderer::GetDebugDraw() で借りる。
    //!        積んだ頂点は、描画コマンドの customDraw から Flush() を呼んだときに描かれる
    //!        （PhysicsSystem / ゲームの CombatSystem が World パスへそのコマンドを積んでいる）
    //------------------------------------------------------------------------
    class DebugDraw {
    public:

        //! シェーダーと頂点バッファを作成します。
        //! @param  [in] graphicsContext デバイスの取得元
        //! @param  [in] resources       共通ステートの取得元
        //! @param  [in] frameConstants  b0 の転送先
        //! @param  [in] vs              デバッグ線用の頂点シェーダー
        //! @param  [in] ps              デバッグ線用のピクセルシェーダー
        //! @return true: 作成成功, false: 作成失敗
        [[nodiscard]]
        bool Initialize(GraphicsContext&                   graphicsContext,
                        RenderResources&                   resources,
                        FrameConstants&                    frameConstants,
                        const Tsukino::Asset::ShaderAsset* vs,
                        const Tsukino::Asset::ShaderAsset* ps);

        //! 線を1本積みます。
        //! @param  [in] v1 始点
        //! @param  [in] v2 終点
        void DrawLine(const Tsukino::GraphicsCommon::DebugVertex& v1, const Tsukino::GraphicsCommon::DebugVertex& v2);

        //! 三角形を1枚積みます（ワイヤーフレームで描かれます）。
        //! @param  [in] v1 頂点1
        //! @param  [in] v2 頂点2
        //! @param  [in] v3 頂点3
        void DrawTriangle(const Tsukino::GraphicsCommon::DebugVertex& v1,
                          const Tsukino::GraphicsCommon::DebugVertex& v2,
                          const Tsukino::GraphicsCommon::DebugVertex& v3);

        //! 積んだ線と三角形を描画し、空にします。
        void Flush();

        //! 積んだ線と三角形を描画せずに捨てます。
        //! @note   リサイズでパイプラインの状態が落ちるときに使う
        void Clear();

    private:
        //! 1回に描ける頂点数の上限（頂点バッファの大きさ）
        static constexpr size_t kMaxVertices = 50000;

        GraphicsContext* m_graphicsContext = nullptr;    // デバイスの取得元（借りている）
        RenderResources* m_resources       = nullptr;    // 共通ステートの取得元（借りている）
        FrameConstants*  m_frameConstants  = nullptr;    // b0 の転送先（借りている）

        std::vector<Tsukino::GraphicsCommon::DebugVertex> m_lineVertices;        // 積まれた線の頂点
        std::vector<Tsukino::GraphicsCommon::DebugVertex> m_triangleVertices;    // 積まれた三角形の頂点

        Microsoft::WRL::ComPtr<ID3D11Buffer>       m_lineVB;        // 線用の動的頂点バッファ
        Microsoft::WRL::ComPtr<ID3D11Buffer>       m_triangleVB;    // 三角形用の動的頂点バッファ
        Microsoft::WRL::ComPtr<ID3D11VertexShader> m_vs;            // 頂点シェーダー
        Microsoft::WRL::ComPtr<ID3D11PixelShader>  m_ps;            // ピクセルシェーダー
        Microsoft::WRL::ComPtr<ID3D11InputLayout>  m_inputLayout;   // 入力レイアウト
    };
}    // namespace Tsukino::Renderer
