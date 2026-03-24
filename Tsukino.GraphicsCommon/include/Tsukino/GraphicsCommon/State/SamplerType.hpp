//-------------------------------------------------------------
//! @file   SamplerType.hpp
//! @brief  サンプラーの種類を定義する列挙型
//-------------------------------------------------------------
#pragma once
#include <Tsukino/Core/typedef.hpp>

namespace Tsukino::GraphicsCommon {
    //-------------------------------------------------------------
    //! @enum   SamplerType
    //! @brief  描画時のテクスチャサンプリング方式
    //-------------------------------------------------------------
    enum class SamplerType : u8 {
        PointWrap,           //!< ドット絵・UI用（補間なし / リピート）
        PointClamp,          //!< ドット絵・UI用（補間なし / 切り捨て）
        LinearWrap,          //!< 一般テクスチャ用（なめらか / リピート）
        LinearClamp,         //!< スプライト用（なめらか / 切り捨て）
        AnisotropicWrap,     //!< 高品質3D用（異方性フィルタ / リピート）
        AnisotropicClamp,    //!< 高品質3D用（異方性フィルタ / 切り捨て）
        Count
    };
}    // namespace Tsukino::GraphicsCommon
