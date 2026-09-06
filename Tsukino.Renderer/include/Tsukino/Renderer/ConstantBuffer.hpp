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
        Tsukino::Core::Math::matrix invViewProj;      //!< viewProjの逆行列（スカイ・ポストエフェクト等で使用）
        Tsukino::Core::Math::matrix lightViewProj;    //!< ライト空間のViewProjection行列
        hlslpp::float4              lightDir;         //!< ライトの方向
        hlslpp::float4              lightColor;       //!< ライト色と強度 xyz: 色(linear), w: 強度
        hlslpp::float4              cameraPos;        //!< カメラのワールド座標 xyz: 座標, w: 未使用
        Tsukino::Core::Math::matrix prevViewProj;     //!< 前フレームのViewProjection行列（速度バッファ生成用）
    };

    //--------------------------------------------------------------
    //! @struct CBufferTransform
    //! @brief  スロット1 (b1) 用：オブジェクトごとの固有データ
    //! @note   b1を「world 1本だけ」で宣言している既存シェーダー（ShadowMap系・Sprite）は
    //!         先頭64バイトしか読まないため、末尾に追加する分には無変更で動く。
    //--------------------------------------------------------------
    struct CBufferTransform {
        Tsukino::Core::Math::matrix world;
        Tsukino::Core::Math::matrix prevWorld;      //!< 前フレームのワールド行列（速度バッファ生成用）
        hlslpp::float4              motionFlags;    //!< x: 1=前フレーム有効 / 0=速度ゼロ, yzw: 予約
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
        float          specular;
        hlslpp::float4 rimColor;     //!< xyz: ふちの色, w: ふちの強さ（旧paddingを転用。ハイライト演出用）
        hlslpp::float4 rimParams;    //!< x: ふちの鋭さ(pow指数), y: 全体の白発光量, z: alphaCutoff（0=アルファテスト無効）, w: 予約
    };

    //--------------------------------------------------------------
    //! @struct CBufferSkinning
    //! @brief  スロット3 (b3) 用：アニメーションするオブジェクトのボーン行列
    //--------------------------------------------------------------
    struct CBufferSkinning {
        hlslpp::float4x4 bones[128];    // hlslpp::float4x4 の配列（最大128本分）
    };

    //--------------------------------------------------------------
    //! @struct CBufferSkinningPrev
    //! @brief  スロット7 (b7) 用：前フレームのボーン行列（速度バッファ生成用）
    //! @note   CBufferSkinning と同じレイアウト。モーションブラーが無効なときは
    //!         転送もバインドも行わない（スキン1体あたり8KBの転送を節約する）。
    //--------------------------------------------------------------
    struct CBufferSkinningPrev {
        hlslpp::float4x4 bones[128];
    };

    //--------------------------------------------------------------
    //! @struct CBufferMotionBlur
    //! @brief  スロット8 (b8) 用：モーションブラーパラメータ
    //! @note   速度バッファには「1フレームあたりの生のUV移動量」だけが入っている。
    //!         強度・シャッター補正はすべてここで掛ける（G-Bufferをタイミング
    //!         パラメータから独立させるため）。
    //--------------------------------------------------------------
    struct CBufferMotionBlur {
        float strength      = 1.0f;     //!< 速度ベクトルの倍率（攻撃時にアプリ側が上げる）
        float maxBlurRadius = 0.03f;    //!< UV単位のブラー長クランプ
        float shutterScale  = 1.0f;     //!< 可変フレームレート補正 (targetFps * deltaTime)
        int   sampleCount   = 8;        //!< サンプル数（1〜kMotionBlurMaxSamples）
    };

    //--------------------------------------------------------------
    //! @brief モーションブラーのサンプル数上限（MotionBlur.ps.hlsl と一致させること）
    //--------------------------------------------------------------
    static constexpr int kMotionBlurMaxSamples = 16;

    //--------------------------------------------------------------
    //! @struct CBufferSky
    //! @brief  スロット4 (b4) 用：大気散乱パラメータ
    //--------------------------------------------------------------
    struct CBufferSky {
        //----------------------------------------------------------
        // 散乱パラメータ
        //----------------------------------------------------------
        float rayleighScattering;    //!< レイリー散乱の強さ
        float mieScattering;         //!< ミー散乱の強さ
        float mieAnisotropy;         //!< ミー散乱の異方性
        float sunIntensity;          //!< 太陽の強度

        //----------------------------------------------------------
        // 大気パラメータ
        //----------------------------------------------------------
        float atmosphereHeight;    //!< 大気の厚さ
        float planetRadius;        //!< 地球の半径
        float sunDiskSize;         //!< 太陽円盤の大きさ
        float padding0;            //!< 16バイトアライメント用

        //----------------------------------------------------------
        // 地面カラー・太陽方向
        //----------------------------------------------------------
        hlslpp::float4 groundColor;     //!< xyz: 地面カラー, w: 未使用
        hlslpp::float4 sunDirection;    //!< xyz: 太陽方向（正規化済み）, w: 未使用
    };

    //--------------------------------------------------------------
    //! @struct GPULight
    //! @brief  点光源・スポットライト1灯分のGPU転送用データ (64B)
    //! @note   Lighting.hlsli の GPULight と1バイト単位で一致させること
    //--------------------------------------------------------------
    struct GPULight {
        hlslpp::float4 positionRange;     //!< xyz: ワールド座標, w: 影響半径
        hlslpp::float4 colorIntensity;    //!< xyz: 色(linear), w: 強度
        hlslpp::float4 directionType;     //!< xyz: 方向（スポットのみ有効）, w: 0=Point, 1=Spot
        hlslpp::float4 spotParams;        //!< x: cos(内側角), y: cos(外側角), zw: 予約
    };

    //--------------------------------------------------------------
    //! @brief 同時に扱える点光源・スポットライトの上限数
    //--------------------------------------------------------------
    static constexpr unsigned int MAX_LIGHTS = 64;

    //--------------------------------------------------------------
    //! @struct CBufferLights
    //! @brief  スロット6 (b6) 用：点光源・スポットライト配列（ディファードLightingパス用）
    //--------------------------------------------------------------
    struct CBufferLights {
        unsigned int lightCount = 0;
        unsigned int pad[3]{};
        GPULight     lights[MAX_LIGHTS]{};
    };

    //--------------------------------------------------------------
    //! @struct CBufferWater
    //! @brief  スロット5 (b5) 用：水面パラメータ
    //--------------------------------------------------------------
    struct CBufferWater {
        float time;            //!< 経過時間（UVスクロール用）
        float waveSpeed;       //!< 波のスクロール速度
        float waveScale;       //!< 波のUVスケール
        float fresnelPower;    //!< フレネルの強さ

        hlslpp::float4 shallowColor;    //!< 浅瀬の色 xyz: 色, w: 未使用
        hlslpp::float4 deepColor;       //!< 深部の色 xyz: 色, w: 未使用
    };

    //--------------------------------------------------------------
    //! @struct CBufferFog
    //! @brief  スロット9 (b9) 用：フォグパラメータ
    //! @note   Fog.ps.hlsl の CBufferFog と1バイト単位で一致させること。
    //!         距離フォグ・高さフォグ・ノイズ揺らぎをまとめて持つ。
    //--------------------------------------------------------------
    struct CBufferFog {
        hlslpp::float4 color;             //!< xyz: フォグ色(linear), w: 距離フォグ密度
        hlslpp::float4 distanceParams;    //!< x: 開始距離, y: 最大不透明度, z: 高さフォグ有効(0/1), w: 予約
        hlslpp::float4 heightParams;      //!< x: 基準高さ, y: 高さ減衰, z: 高さフォグ密度, w: 予約
        hlslpp::float4 sunColor;          //!< xyz: 太陽方向の散乱色, w: 散乱の鋭さ(pow指数)
        hlslpp::float4 noiseParams;       //!< x: ノイズスケール, y: ノイズ強度, z: 経過時間, w: ノイズ有効(0/1)
        hlslpp::float4 windParams;        //!< xyz: 風向き(正規化済み), w: 風速
    };

}    // namespace Tsukino::Renderer
