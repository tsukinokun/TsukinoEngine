//-------------------------------------------------------------
//! @file   SpriteComponent.hpp
//! @brief  SpriteComponentクラスの宣言
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once
#include <Tsukino/Engine/Asset/AssetRef.hpp>

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
    //! @enum   SpriteFillMode
    //! @brief  スプライトの一部だけを塗る方法
    //! @note   Radial は画像の中心から見た角度で塗る範囲を決める（リング状のゲージ・
    //!         クールダウン表示向け）。塗らない部分は不透明度 0 になる
    //-------------------------------------------------------------
    enum class SpriteFillMode {
        None = 0,    // 画像全体を描く（既定）
        Radial,      // 中心を軸に、開始角から fillAmount の割合だけ扇形に描く
    };

    //-------------------------------------------------------------
    //! @struct SpriteComponent
    //! @brief  スプライト（2D画像）描画に必要な情報を管理するコンポーネント
    //-------------------------------------------------------------
    struct SpriteComponent {
        // テクスチャのハンドル
        Tsukino::Asset::AssetRef textureHandle;

        // 合成方法（既定はAlpha）
        SpriteBlendMode blendMode = SpriteBlendMode::Alpha;

        // 座標空間（既定はScreen）
        SpriteSpace space = SpriteSpace::Screen;

        // 色（Tint）: 各ピクセルに掛け合わせる色 (R, G, B, A)。linear の値で、1 を超える HDR も可
        // デフォルトは白 (1.0, 1.0, 1.0, 1.0) で、元の画像の色をそのまま表示します。
        // 見た目の色（sRGB）で指定したいときは Core::Math::Color::SrgbToLinear で変換して渡す
        hlslpp::float4 tintColor = hlslpp::float4(1.0f, 1.0f, 1.0f, 1.0f);

        // UV座標（画像のどの部分を切り取るか）
        // x, y: 左上の開始UV座標 (デフォルト 0, 0)
        // z, w: 右下の終了UV座標 または UVの幅と高さ (デフォルト 1, 1 で画像全体)
        hlslpp::float4 uvRect = hlslpp::float4(0.0f, 0.0f, 1.0f, 1.0f);

        // 描画順（小さいほど先に描かれる）。RenderPass::Overlayへ積まれるスプライトは
        // FontComponent::sortOrderと同じ1本の軸として比較されるため、
        // 画面固定UIの重なりはスプライトと文字の種類を問わずここの値だけで決まる
        int sortOrder = 0;

        // 塗り方（既定はNoneで画像全体を描く）。Radialにすると fillAmount の割合だけ扇形に描く
        SpriteFillMode fillMode = SpriteFillMode::None;

        // 塗る割合（0〜1）。fillModeがNoneのときは使わない
        float fillAmount = 1.0f;

        // 塗り始めの角度（度）。0 が真上で、fillClockwise の向きに増える
        float fillStartAngle = 0.0f;

        // 時計回りに塗るか（falseなら反時計回り）
        bool fillClockwise = true;

        //-------------------------------------------------------------
        // 9 スライス。画像の四辺の sliceBorder（画像のピクセル。x: 左, y: 上, z: 右, w: 下）の帯は伸ばさず、
        // 四隅はそのままの大きさ（× sliceScale）で、辺は1方向だけ、中央は両方向に伸ばして描く。
        // 角丸や縁取りの付いた板を、角を歪めずにいろいろな大きさで使える。
        // 大きさは今まで通り Transform の scale（画像の何倍か）で決める。すべて 0 なら画像全体を1枚で描く（既定）。
        // 円形の塗り（fillMode が Radial）とは一緒に使えない（Radial のときは1枚で描く）
        //-------------------------------------------------------------
        hlslpp::float4 sliceBorder = hlslpp::float4(0.0f, 0.0f, 0.0f, 0.0f);

        // 四隅を描く倍率（1 で画像のピクセルのまま。板を小さく作った画像で縁を太く見せたいときなどに使う）
        float sliceScale = 1.0f;

        // マウスの当たりを受けるか。false にすると InteractionSystem はこのスプライトを無いものとして扱い、
        // 奥のスプライトに当たりを通す（ボタンの上に重ねたアイコンなど、飾りの画像に使う）
        bool raycastTarget = true;
    };

}    // namespace Tsukino::BuiltIn::ECS
