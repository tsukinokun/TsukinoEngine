//-------------------------------------------------------------
//! @file   SpriteComponent.hpp
//! @brief  SpriteComponentクラスの宣言
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once
#include <Tsukino/Engine/Asset/AssetHandle.hpp>

#include <hlsl++.h>
// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {
    //-------------------------------------------------------------
    //! @struct SpriteComponent
    //! @brief  スプライト（2D画像）描画に必要な情報を管理するコンポーネント
    //-------------------------------------------------------------
    struct SpriteComponent {
        // テクスチャのハンドル
        Tsukino::Asset::AssetHandle textureHandle;

        // 色（Tint）: 各ピクセルに掛け合わせる色 (R, G, B, A)
        // デフォルトは白 (1.0, 1.0, 1.0, 1.0) で、元の画像の色をそのまま表示します
        hlslpp::float4 tintColor = hlslpp::float4(1.0f, 1.0f, 1.0f, 1.0f);

        // UV座標（画像のどの部分を切り取るか）
        // x, y: 左上の開始UV座標 (デフォルト 0, 0)
        // z, w: 右下の終了UV座標 または UVの幅と高さ (デフォルト 1, 1 で画像全体)
        hlslpp::float4 uvRect = hlslpp::float4(0.0f, 0.0f, 1.0f, 1.0f);

        // 描画の順番
        int sortOrder = 0;
    };

}    // namespace Tsukino::BuiltIn::ECS
