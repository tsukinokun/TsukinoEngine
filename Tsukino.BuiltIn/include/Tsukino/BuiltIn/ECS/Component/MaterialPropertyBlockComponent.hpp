//----------------------------------------------------------------------------
//! @file   MaterialPropertyBlockComponent.hpp
//! @brief  エンティティ単位でマテリアルの値を上書きするコンポーネント
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/GraphicsCommon/Material/MaterialData.hpp>

#include <hlsl++.h>

#include <optional>

// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {

    //! エンティティ単位でマテリアルの値を上書きする設定です（Unity の MaterialPropertyBlock に当たる）。
    //! @note ModelSystem が、マテリアル（モデルのもの、または ModelComponent::materials で差し替えたもの）の値を
    //!       CBufferMaterial に詰めた後で、値の入っている項目だけを置き換える。アセットは共有したまま
    //!       変えないので、同じモデルを使う別のエンティティには影響せず、色ごとにアセットを複製する必要も無い。
    //!
    //!       値は「置き換え」。baseColor はシェーダーでアルベドテクスチャに掛かるので、
    //!       テクスチャ付きのモデルでもテクスチャの模様を残したまま色味を変えられる。
    //!       メッシュが複数のマテリアルを使う場合は、そのすべてに同じ値が入る。
    //!       不透明（GBuffer）と半透明（フォワード）のどちらの描画にも効く。
    //!
    //!       マテリアルそのものを別のものにしたいとき（質感そのものを変えたいとき）は
    //!       ModelComponent::materials を使う。こちらはエンティティごとの小さな違い（色だけを変えるなど）に使う。
    //!       shadingModel も上書きできるので、モデルに入っているマテリアル（テクスチャ）のまま照らし方だけを変えられる
    struct MaterialPropertyBlockComponent {
        std::optional<hlslpp::float4> baseColor;    // 基本色（アルファを含む）
        std::optional<hlslpp::float3> emissive;     // 自己発光の色（HDR なので 1.0 超も可）
        std::optional<float>          metallic;     // メタリック（0〜1）
        std::optional<float>          roughness;    // ラフネス（0〜1）

        //--------------------------------------------------------------
        // 照らし方（意味は MaterialData の同名の項目と同じ）
        //--------------------------------------------------------------
        std::optional<Tsukino::GraphicsCommon::ShadingModel> shadingModel;        // 照らし方（PBR / Unlit / Toon）
        std::optional<float>                                 toonThreshold;       // トゥーンの明るい側と暗い側の境目（0〜1）
        std::optional<float>                                 toonSmoothness;      // トゥーンの境目のぼかし幅（0〜1）
        std::optional<hlslpp::float3>                        toonShadeColor;      // トゥーンの暗い側に掛ける色（0〜1）
        std::optional<float>                                 toonSpecularSize;    // トゥーンのハイライトの大きさ（0〜1。0 で無し）
    };
}    // namespace Tsukino::BuiltIn::ECS
