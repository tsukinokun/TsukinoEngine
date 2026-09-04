//----------------------------------------------------------------------------
//! @file   IPhysicsDebugDraw.hpp
//! @brief  物理形状のデバッグ描画を受け取るインターフェース
//! @detail Tsukino.Physics は描画バックエンドを知りません。デバッグ形状の線分と
//!         三角形をこのインターフェースへ流すだけにしておき、実際の描画は
//!         上位モジュール（Tsukino.EngineIntegration）が実装します。
//----------------------------------------------------------------------------
#pragma once
#include <hlsl++.h>

// 名前空間 : Tsukino::Physics
namespace Tsukino::Physics {

    //------------------------------------------------------------------------
    //! デバッグ描画の出力先
    //------------------------------------------------------------------------
    class IPhysicsDebugDraw {
    public:
        //! デストラクタ
        virtual ~IPhysicsDebugDraw() = default;

        //! 線分を1本描画します。
        //! @param  [in] from  始点（ワールド座標）
        //! @param  [in] to    終点（ワールド座標）
        //! @param  [in] color 色（RGBA。各成分 0.0〜1.0）
        virtual void DrawLine(const hlslpp::float3& from, const hlslpp::float3& to, const hlslpp::float4& color) = 0;

        //! 三角形を1枚描画します。
        //! @param  [in] v0    頂点0（ワールド座標）
        //! @param  [in] v1    頂点1（ワールド座標）
        //! @param  [in] v2    頂点2（ワールド座標）
        //! @param  [in] color 色（RGBA。各成分 0.0〜1.0）
        virtual void DrawTriangle(const hlslpp::float3& v0,
                                  const hlslpp::float3& v1,
                                  const hlslpp::float3& v2,
                                  const hlslpp::float4& color) = 0;
    };

}    // namespace Tsukino::Physics
