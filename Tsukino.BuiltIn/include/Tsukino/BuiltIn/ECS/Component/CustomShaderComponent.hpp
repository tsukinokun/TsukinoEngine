//--------------------------------------------------------------
//! @file   CustomShaderComponent.hpp
//! @brief  モデルを自作のシェーダーで描くためのコンポーネント
//--------------------------------------------------------------
#pragma once
#include <Tsukino/Engine/Asset/AssetRef.hpp>

#include <hlsl++.h>

#include <array>

// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {

    //--------------------------------------------------------------
    //! @struct CustomShaderComponent
    //! @brief  ModelComponent のモデルを、自作のシェーダーで描く設定です（Unity の Material にシェーダーを指定するのに当たる）。
    //! @note   ModelSystem が、通常のディファード描画（GBuffer）の代わりに、指定したシェーダーで
    //!         深度を書くフォワード（RenderPass::World）に描く。ライティングは掛からず、色はシェーダーが決める。
    //!         影は落とさない（影の描画は GBuffer パスの物だけを対象にするため）。
    //!         ModelComponent と同じエンティティに付ける。pixelShader が無効なら通常どおりに描く。
    //!
    //!         シェーダーはゲーム側の tsukino_shaders() のフォルダ（例: Assets/Shaders）に置いた .sc を、
    //!         AssetManager::Load("Assets/Shaders/MyColor.ps.sc") のように .sc のパスで読んだものを入れる。
    //!         ピクセルシェーダーの入力は組み込みの頂点シェーダーの出力に合わせる:
    //!           $input v_worldPos, v_normal, v_texcoord0, v_curClip, v_prevClip
    //!         params は毎フレーム書き換えてよい（値はシェーダーの uniform vec4 u_user0[4] に届く）。
    //!         経過時間は組み込みの Scene.sh の timeParams（x = 秒）でも読める。
    //--------------------------------------------------------------
    struct CustomShaderComponent {
        Tsukino::Asset::AssetRef pixelShader;     // ピクセルシェーダー（.ps.sc）。無効なら通常の描画
        Tsukino::Asset::AssetRef vertexShader;    // 頂点シェーダー（.vs.sc）。無効なら組み込みのモデル用を使う

        // シェーダーの u_user0[0]〜u_user0[3] に届く値（色・進み具合など、用途は自由）
        std::array<hlslpp::float4, 4> params = {hlslpp::float4(0.0f), hlslpp::float4(0.0f), hlslpp::float4(0.0f), hlslpp::float4(0.0f)};
    };
}    // namespace Tsukino::BuiltIn::ECS
