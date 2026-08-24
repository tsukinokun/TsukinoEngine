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
    //! @enum   SpriteBlendMode
    //! @brief  スプライトの合成方法
    //! @note   Tsukino::Renderer::BlendModeをそのまま持ち込むとコンポーネント層が
    //!         レンダラーへ依存してしまうため、ここではSpriteRenderSystem側だけが
    //!         知っていればよい最小限の選択肢をローカルに持つ
    //-------------------------------------------------------------
    enum class SpriteBlendMode {
        Alpha = 0,    // 通常の半透明合成（既定）
        Additive,     // 加算合成。発光表現に使う
    };

    //-------------------------------------------------------------
    //! @enum   SpriteSpace
    //! @brief  スプライトをどの空間の座標として扱うか
    //! @note   TransformComponent.position/scaleの意味がこれで変わる：
    //!         Screen : positionは画面ピクセル座標（左上原点）、scaleはテクスチャピクセル寸法への倍率。
    //!                  常に画面最前面（RenderPass::Overlay、深度テストなし）に描かれる。HPバー等のHUD向け
    //!         World  : positionは3Dワールド座標、scaleはワールド単位（1ユニット≒1cm）でのテクスチャ寸法への倍率。
    //!                  主カメラを向くビルボードとして、3Dシーンの深度と正しく前後判定される（RenderPass::World）。
    //!                  ワールド上を動く物（EXP玉等）向け
    //-------------------------------------------------------------
    enum class SpriteSpace {
        Screen = 0,    // 画面固定のHUD（既定）
        World,         // 3Dワールド空間のビルボード（深度テストあり）
    };

    //-------------------------------------------------------------
    //! @struct SpriteComponent
    //! @brief  スプライト（2D画像）描画に必要な情報を管理するコンポーネント
    //-------------------------------------------------------------
    struct SpriteComponent {
        // テクスチャのハンドル
        Tsukino::Asset::AssetHandle textureHandle;

        // 合成方法（既定はAlpha）
        SpriteBlendMode blendMode = SpriteBlendMode::Alpha;

        // 座標空間（既定はScreen）
        SpriteSpace space = SpriteSpace::Screen;

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
