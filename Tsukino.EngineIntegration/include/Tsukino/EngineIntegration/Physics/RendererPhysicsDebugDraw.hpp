//----------------------------------------------------------------------------
//! @file   RendererPhysicsDebugDraw.hpp
//! @brief  物理デバッグ描画を Tsukino::Renderer へ中継するクラス
//! @detail Tsukino.Physics が公開する IPhysicsDebugDraw の実装です。下位層の
//!         Tsukino.Physics は描画バックエンドを知らないため、実際に Renderer へ
//!         線分・三角形を積むのは上位層であるこちらの役目になります。
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/Physics/IPhysicsDebugDraw.hpp>

#include <Tsukino/GraphicsCommon/Vertex/DebugVertex.hpp>
#include <Tsukino/Renderer/Renderer.hpp>

// 名前空間 : Tsukino::EngineIntegration
namespace Tsukino::EngineIntegration {

    //------------------------------------------------------------------------
    //! 物理デバッグ描画の出力先（Renderer 実装）
    //------------------------------------------------------------------------
    class RendererPhysicsDebugDraw final : public Tsukino::Physics::IPhysicsDebugDraw {
    public:
        //! コンストラクタ
        //! @param  [in] renderer 描画先のレンダラー
        explicit RendererPhysicsDebugDraw(Tsukino::Renderer::Renderer* renderer) : m_renderer(renderer) {}

        //! 線分を1本描画します。
        //! @param  [in] from  始点（ワールド座標）
        //! @param  [in] to    終点（ワールド座標）
        //! @param  [in] color 色（RGBA。各成分 0.0〜1.0）
        void DrawLine(const hlslpp::float3& from, const hlslpp::float3& to, const hlslpp::float4& color) override {
            if(!m_renderer)
                return;

            m_renderer->DrawDebugLine(MakeVertex(from, color), MakeVertex(to, color));
        }

        //! 三角形を1枚描画します。
        //! @param  [in] v0    頂点0（ワールド座標）
        //! @param  [in] v1    頂点1（ワールド座標）
        //! @param  [in] v2    頂点2（ワールド座標）
        //! @param  [in] color 色（RGBA。各成分 0.0〜1.0）
        void DrawTriangle(const hlslpp::float3& v0,
                          const hlslpp::float3& v1,
                          const hlslpp::float3& v2,
                          const hlslpp::float4& color) override {
            if(!m_renderer)
                return;

            m_renderer->DrawDebugTriangle(MakeVertex(v0, color), MakeVertex(v1, color), MakeVertex(v2, color));
        }

    private:
        //! 位置と色からデバッグ頂点を組み立てます。
        //! @param  [in] position 頂点の位置
        //! @param  [in] color    頂点の色
        //! @return 組み立てた頂点
        static Tsukino::GraphicsCommon::DebugVertex MakeVertex(const hlslpp::float3& position, const hlslpp::float4& color) {
            return Tsukino::GraphicsCommon::DebugVertex{
                {position.x, position.y, position.z},
                {color.x,    color.y,    color.z,    color.w}
            };
        }

        Tsukino::Renderer::Renderer* m_renderer = nullptr;    //!< 描画先のレンダラー
    };

}    // namespace Tsukino::EngineIntegration
