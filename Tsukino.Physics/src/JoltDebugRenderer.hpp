//----------------------------------------------------------------------------
//! @file   JoltDebugRenderer.hpp
//! @brief  Jolt のデバッグ描画を IPhysicsDebugDraw へ中継するクラス
//! @detail Tsukino.Physics の内部専用ヘッダです。Jolt の Shape::Draw() は
//!         JPH::DebugRenderer を要求しますが、Tsukino.Physics は描画バックエンドを
//!         知らないため、受け取った線分と三角形をそのまま上位のシンクへ流します。
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/Physics/IPhysicsDebugDraw.hpp>

#include "JoltConversion.hpp"

#include <Jolt/Jolt.h>
#include <Jolt/Renderer/DebugRendererSimple.h>

#include <string_view>

// 名前空間 : Tsukino::Physics
namespace Tsukino::Physics {

    //------------------------------------------------------------------------
    //! Jolt のデバッグ描画を IPhysicsDebugDraw へ中継するクラス
    //------------------------------------------------------------------------
    class JoltDebugRenderer final : public JPH::DebugRendererSimple {
    public:
        //! コンストラクタ
        JoltDebugRenderer() { JPH::DebugRendererSimple::Initialize(); }

        //! 描画の出力先を設定します。
        //! @param  [in] sink 出力先。nullptr を渡すと描画を破棄します
        void SetSink(IPhysicsDebugDraw* sink) { m_sink = sink; }

        //! 線分を1本描画します。
        //! @param  [in] inFrom  始点
        //! @param  [in] inTo    終点
        //! @param  [in] inColor 色
        void DrawLine(JPH::RVec3Arg inFrom, JPH::RVec3Arg inTo, JPH::ColorArg inColor) override {
            if(!m_sink)
                return;
            m_sink->DrawLine(ToFloat3(inFrom), ToFloat3(inTo), ToFloat4(inColor));
        }

        //! 三角形を1枚描画します。
        //! @param  [in] inV1         頂点1
        //! @param  [in] inV2         頂点2
        //! @param  [in] inV3         頂点3
        //! @param  [in] inColor      色
        //! @param  [in] inCastShadow 影を落とすかどうか（このクラスでは未使用）
        void DrawTriangle(JPH::RVec3Arg inV1, JPH::RVec3Arg inV2, JPH::RVec3Arg inV3, JPH::ColorArg inColor, ECastShadow inCastShadow) override {
            if(!m_sink)
                return;
            m_sink->DrawTriangle(ToFloat3(inV1), ToFloat3(inV2), ToFloat3(inV3), ToFloat4(inColor));
        }

        //! 3D 空間へ文字列を描画します。
        //! @param  [in] inPosition 描画位置
        //! @param  [in] inString   描画する文字列
        //! @param  [in] inColor    色
        //! @param  [in] inHeight   文字高
        //! @note   文字描画は未対応のため何もしません
        void DrawText3D(JPH::RVec3Arg inPosition, const std::string_view& inString, JPH::ColorArg inColor, float inHeight) override {}

    private:
        IPhysicsDebugDraw* m_sink = nullptr;    //!< 描画の出力先
    };

}    // namespace Tsukino::Physics
