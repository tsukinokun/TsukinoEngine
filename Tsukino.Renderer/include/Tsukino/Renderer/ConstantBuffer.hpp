//--------------------------------------------------------------
//! @file   ConstantBuffer.hpp
//! @brief  VS用定数バッファ構造体（行列）
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
#include <Tsukino/Core/Math/Matrix.hpp>
#include <hlsl++.h>
// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {
    //--------------------------------------------------------------
    //! @struct CBufferScene
    //! @brief  スロット0 (b0) 用：フレーム内で全オブジェクト共通のデータ
    //--------------------------------------------------------------
    struct CBufferScene {
        Tsukino::Core::Math::matrix view;
        Tsukino::Core::Math::matrix projection;
        Tsukino::Core::Math::matrix viewProj;
        Tsukino::Core::Math::matrix lightViewProj;    //!< ライト空間のViewProjection行列
        hlslpp::float4              lightDir;         //!< ライトの方向
    };

    //--------------------------------------------------------------
    //! @struct CBufferTransform
    //! @brief  スロット1 (b1) 用：オブジェクトごとの固有データ
    //--------------------------------------------------------------
    struct CBufferTransform {
        Tsukino::Core::Math::matrix world;
    };

    //--------------------------------------------------------------
    //! @struct CBufferMaterial
    //! @brief  スロット2 (b2) 用：マテリアルごとの固有データ
    //--------------------------------------------------------------
    struct CBufferMaterial {
        hlslpp::float4 baseColor;
        hlslpp::float3 emissive;
        float          metallic;
        float          roughness;
        hlslpp::float3 padding;
    };

    //--------------------------------------------------------------
    //! @struct CBufferSkinning
    //! @brief  スロット3 (b3) 用：アニメーションするオブジェクトのボーン行列
    //--------------------------------------------------------------
    struct CBufferSkinning {
        hlslpp::float4x4 bones[128];    // hlslpp::float4x4 の配列（最大128本分）
    };
}    // namespace Tsukino::Renderer
