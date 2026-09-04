//----------------------------------------------------------------------------
//! @file   PhysicsMath.hpp
//! @brief  物理側と姿勢計算を揃えるための補助関数
//! @detail 親の姿勢へコライダーのローカルオフセットを合成する計算です。
//!         物理エンジンと同じ演算・同じ正規化で行う必要があるため、実装は
//!         Tsukino.Physics 側に置いてあります。
//----------------------------------------------------------------------------
#pragma once
#include <hlsl++.h>

// 名前空間 : Tsukino::Physics
namespace Tsukino::Physics {

    //------------------------------------------------------------------------
    //! 親の姿勢へローカルオフセットを合成した姿勢を求めます。
    //! @param  [in]  position       親の位置（ワールド）
    //! @param  [in]  rotation       親の向き
    //! @param  [in]  offsetPosition 親のローカル空間でのオフセット位置
    //! @param  [in]  offsetRotation 親のローカル空間でのオフセット回転
    //! @param  [out] outPosition    合成後の位置（ワールド）
    //! @param  [out] outRotation    合成後の向き
    //! @note   入力の回転は内部で正規化されます
    //------------------------------------------------------------------------
    void ComposeTransform(const hlslpp::float3&     position,
                          const hlslpp::quaternion& rotation,
                          const hlslpp::float3&     offsetPosition,
                          const hlslpp::quaternion& offsetRotation,
                          hlslpp::float3&           outPosition,
                          hlslpp::quaternion&       outRotation);

}    // namespace Tsukino::Physics
