//--------------------------------------------------------------
//! @file   ConstantBuffer.hpp
//! @brief  VS用定数バッファ構造体（行列）
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
#include <cstddef>    // offsetof（b2のレイアウト検査に使う）
#include <Tsukino/Core/Math/Matrix.hpp>
#include <hlsl++.h>
// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {
    //--------------------------------------------------------------
    //! カスケードシャドウの枚数
    //!  Scene.hlsli の TSUKINO_SHADOW_CASCADE_COUNT と一致させること。
    //!       CBufferScene の cascadeViewProj の要素数がこれで決まるため、
    //!       b0 のレイアウトに直結する（ShadowPass::kCascadeCount もこれを使う）
    //--------------------------------------------------------------
    static constexpr unsigned int kShadowCascadeCount = 3;

    //--------------------------------------------------------------
    //! @struct CBufferScene
    //! @brief  スロット0 (b0) 用：フレーム内で全オブジェクト共通のデータ
    //--------------------------------------------------------------
    struct CBufferScene {
        Tsukino::Core::Math::matrix view;
        Tsukino::Core::Math::matrix projection;
        Tsukino::Core::Math::matrix viewProj;
        Tsukino::Core::Math::matrix invViewProj;      //!< viewProjの逆行列（スカイ・ポストエフェクト等で使用）
        Tsukino::Core::Math::matrix cascadeViewProj[kShadowCascadeCount];    //!< カスケードごとのライト空間ViewProjection（近→遠）
        hlslpp::float4              lightDir;         //!< ライトの方向
        hlslpp::float4              lightColor;       //!< ライト色と強度 xyz: 色(linear), w: 強度
        hlslpp::float4              cameraPos;        //!< カメラのワールド座標 xyz: 座標, w: 未使用
        Tsukino::Core::Math::matrix prevViewProj;     //!< 前フレームのViewProjection行列（速度バッファ生成用）

        //--------------------------------------------------------------
        // 以下はフレーム共通の「素材」。Rendererが毎フレーム自動で埋める。
        //
        // ここに置く理由: これが無かった頃は、時間を使う演出（水・フォグ・
        // 環境パーティクル・草）が全て自分専用の定数バッファを1本ずつ確保し、
        // その中に経過時間のコピーを持っていた。定数バッファは b0〜b13 の
        // 14本しかないため、演出を足すたびにスロットが減っていく状態だった。
        // UnityのTime / UEのView.GameTimeと同じく、共通の素材はここに集める。
        //
        // 末尾に足しているのは意図的。HLSLのcbufferは末尾メンバを宣言しなくても
        // 前方の配置が変わらないため、既存のシェーダーは無改修で動く
        // （AmbientParticle.vs.hlslがprevViewProjを省略しているのがその実例）
        //--------------------------------------------------------------
        hlslpp::float4 timeParams;      //!< x: 起動からの経過秒, y: 前フレームからの経過秒, z: sin(x), w: cos(x)
        hlslpp::float4 screenParams;    //!< xy: 描画領域の解像度(px), zw: その逆数(1/w, 1/h)
        hlslpp::float4 shadowParams;         //!< x: シャドウマップの一辺(px), y: その逆数(=texelSize), z: シャドウパスが描画中のカスケード番号, w: 1/奥行き（ワールド距離→深度値の換算）
        hlslpp::float4 cascadeTexelWorld;    //!< xyz: 各カスケードの1テクセルのワールド幅（シャドウバイアスの単位）, w: 予約
    };

    // b0のレイアウトもMaterial.hlsli同様に手で合わせるしかないので、機械的に見張る。
    // b2と違いパディングの細工は無いが、見張る理由はむしろこちらの方が強い。
    // 実際にShadowMapStatic.vs.hlslがinvViewProjの宣言を落としており、
    // 以降が64バイトずれてスタティックメッシュの影が壊れていた（Scene.hlsli参照）。
    // 先頭の行列を1本増減させると全メンバが動くので、要所のオフセットを固定する
    static_assert(sizeof(CBufferScene) == 624, "CBufferScene must stay 624 bytes to match Scene.hlsli (b0).");
    static_assert(offsetof(CBufferScene, cascadeViewProj) == 256, "cascadeViewProj must start at byte 256; a missing matrix before it shifts every cascade by 64 bytes.");
    static_assert(offsetof(CBufferScene, prevViewProj) == 496, "prevViewProj must sit at byte 496 (right after the cascade matrices).");
    static_assert(offsetof(CBufferScene, shadowParams) == 592, "shadowParams must sit at byte 592.");
    static_assert(offsetof(CBufferScene, cascadeTexelWorld) == 608, "cascadeTexelWorld must sit at byte 624; the shadow bias reads it per cascade.");

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
    //! @note   Material.hlsli の CBufferMaterial と1バイト単位で一致させること。
    //!         気をつける点が2つある。
    //!         1つ目は hlslpp::float3 がSIMDレジスタ幅の16バイトを占めること。
    //!         HLSLの float3 は12バイトで、直後のスカラーが同じ16バイト行へ
    //!         詰められるため、放っておくと metallic 以降が4バイトずれる。
    //!         これを防ぐため、HLSL側には emissive の直後に emissivePad を置いてある。
    //!         2つ目は float4 が16バイト境界を跨げないこと。alphaCutoff までの
    //!         4つのスカラーがちょうど1行を埋め、rimColor が48から始まる。
    //--------------------------------------------------------------
    struct CBufferMaterial {
        hlslpp::float4 baseColor;
        hlslpp::float3 emissive;       //!< xyz: 自発光色（hlslpp::float3は16バイト。余る4番目のレーンがHLSL側のemissivePad）
        float          metallic;
        float          roughness;
        float          specular;
        float          alphaCutoff;    //!< アルファテストのしきい値（0=無効）
        hlslpp::float4 rimColor;       //!< xyz: ふちの色, w: ふちの強さ
        hlslpp::float4 rimParams;      //!< x: ふちの鋭さ(pow指数), y: 全体の白発光量, zw: 予約
    };

    // b2のレイアウトはMaterial.hlsli側と手で合わせるしかないので、機械的に見張る。
    // 総サイズだけでは不十分な点に注意。hlslpp::float3が余分に食う4バイトと、
    // float4が16バイト境界まで送られる分は打ち消し合うことがあり、
    // 中身がずれていてもサイズだけは一致してしまう。だからオフセットも固定する。
    //
    // ただし守れるのはこちら側だけ。Material.hlsli の emissivePad を消しても
    // 以下のアサートは全て通り、描画結果が静かに壊れる。片方を触ったら必ず両方見る
    static_assert(sizeof(CBufferMaterial) == 80, "CBufferMaterial must stay 80 bytes to match Material.hlsli (b2).");
    static_assert(offsetof(CBufferMaterial, metallic) == 32, "metallic must sit at byte 32; emissivePad in Material.hlsli covers 28..31.");
    static_assert(offsetof(CBufferMaterial, alphaCutoff) == 44, "alphaCutoff must fill the last slot before rimColor.");
    static_assert(offsetof(CBufferMaterial, rimColor) == 48, "rimColor must start on the 16-byte boundary at 48.");
    static_assert(offsetof(CBufferMaterial, rimParams) == 64, "rimParams must start on the 16-byte boundary at 64.");

    //--------------------------------------------------------------
    //! @struct CBufferSkinning
    //! @brief  スロット3 (b3) 用：アニメーションするオブジェクトのボーン行列
    //--------------------------------------------------------------
    struct CBufferSkinning {
        hlslpp::float4x4 bones[128];    // hlslpp::float4x4 の配列（最大128本分）
    };

    //--------------------------------------------------------------
    //! @struct CBufferSkinningPrev
    //! @brief  スロット6 (b6) 用：前フレームのボーン行列（速度バッファ生成用）
    //! @note   CBufferSkinning と同じレイアウト。モーションブラーが無効なときは
    //!         転送もバインドも行わない（スキン1体あたり8KBの転送を節約する）。
    //--------------------------------------------------------------
    struct CBufferSkinningPrev {
        hlslpp::float4x4 bones[128];
    };

    //--------------------------------------------------------------
    //! @struct CBufferMotionBlur
    //! @brief  スロット7 (b7) 用：モーションブラーパラメータ
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
    //! @brief  スロット5 (b5) 用：点光源・スポットライト配列（ディファードLightingパス用）
    //--------------------------------------------------------------
    struct CBufferLights {
        unsigned int lightCount = 0;
        unsigned int pad[3]{};
        GPULight     lights[MAX_LIGHTS]{};
    };

    //--------------------------------------------------------------
    //! @struct CBufferFog
    //! @brief  スロット8 (b8) 用：フォグパラメータ
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

    //--------------------------------------------------------------
    //! 環境パーティクルの粒子数の上限
    //! @note AmbientParticleSystem がこの値でクランプする。
    //!       1粒 = 6頂点なので、上限では 393,216 頂点の単一Drawになる。
    //--------------------------------------------------------------
    static constexpr unsigned int kMaxAmbientParticles = 65536;

    //--------------------------------------------------------------
    //! スロット9 (b9) 用：環境パーティクルのパラメータ
    //! @note AmbientParticle.vs.hlsl の CBufferAmbientParticle と
    //!       1バイト単位で一致させること（全メンバfloat4で96バイト）。
    //!       ピクセルシェーダーはこのバッファを使わない。色も輝度も
    //!       フェードも頂点シェーダーが計算し、補間値として渡すため。
    //--------------------------------------------------------------
    struct CBufferAmbientParticle {
        hlslpp::float4 volumeParams;    //!< xyz: ボリュームの一辺の長さ, w: 経過時間（秒）
        hlslpp::float4 fadeParams;      //!< x: 境界フェード開始比率(0〜1), y: 近接フェード距離, z: 乱数シード（整数値のfloat）, w: 予約
        hlslpp::float4 sizeParams;      //!< x: 最小サイズ（半径）, y: 最大サイズ（半径）, z: 最小輝度, w: 最大輝度
        hlslpp::float4 driftParams;     //!< xyz: 一定ドリフト速度（ワールド単位/秒）, w: 揺らぎの角速度（rad/秒）
        hlslpp::float4 swayParams;      //!< x: 揺らぎの振幅, y: 速度倍率の下限, z: 速度倍率の上限, w: きらめきの強さ(0〜1)
        hlslpp::float4 colorParams;     //!< xyz: 粒子色（linear）, w: 全体の強度
    };

    //--------------------------------------------------------------
    //! @struct CBufferIBL
    //! @brief  スロット10 (b10) 用：IBL（スカイ由来の環境光）パラメータ
    //! @note   IBL.hlsli の CBufferIBL と1バイト単位で一致させること。
    //!         irradiance/prefiltered specular/BRDF LUTの3枚のテクスチャは
    //!         定数バッファではなくSRV（t17〜t19）経由で渡す。
    //--------------------------------------------------------------
    struct CBufferIBL {
        float specularMipCount = 1.0f;    //!< プレフィルタ済みスペキュラキューブマップのミップ数（roughness→lod変換に使用）
        float iblIntensity     = 1.0f;    //!< IBL全体の強度倍率
        float pad[2]{};                   //!< 16バイトアライメント用
    };

    //--------------------------------------------------------------
    //! @struct CBufferIBLBake
    //! @brief  スロット11 (b11) 用：IBLベイク（キャプチャ/畳み込み/プレフィルタ）専用の一時パラメータ
    //! @note   起動時の一発ベイク中だけバインドする一時バッファ。SpecularPrefilter.ps.hlsl の
    //!         CBufferIBLBake と1バイト単位で一致させること。
    //--------------------------------------------------------------
    struct CBufferIBLBake {
        float        roughness   = 0.0f;    //!< このmipに割り当てられたラフネス [0,1]
        unsigned int sampleCount = 32;       //!< GGX重要度サンプリングのサンプル数（mipが荒いほど増やす）
        float        pad[2]{};              //!< 16バイトアライメント用
    };

}    // namespace Tsukino::Renderer
