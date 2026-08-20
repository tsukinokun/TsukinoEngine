//--------------------------------------------------------------
//! @file   ShaderSlots.hpp
//! @brief  シェーダースロット番号の一元管理
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {

    //--------------------------------------------------------------
    //! @brief 定数バッファスロット (b*)
    //--------------------------------------------------------------
    enum class CBSlot : unsigned int {
        Scene     = 0,    //!< b0 : フレーム共通データ（ビュー・プロジェクション・ライト）
        Transform = 1,    //!< b1 : オブジェクト固有データ（ワールド行列）
        Material  = 2,    //!< b2 : マテリアルデータ（PBRパラメータ）
        Skinning  = 3,    //!< b3 : ボーン行列（スキニングあり限定）
        Sky       = 4,    //!< b4 : 大気散乱パラメータ（Renderer.cppに実装のみ・enum未参照）
        Water     = 5,    //!< b5 : 水面パラメータ（Renderer.cppに実装のみ・enum未参照）
        Lights    = 6,    //!< b6 : 点光源・スポットライト配列（CBufferLights, ディファードLightingパス用）
    };

    //--------------------------------------------------------------
    //! @brief テクスチャスロット (t*)
    //! @note  t0〜t7  : マテリアルテクスチャ（描画オブジェクトごとに差し替え）
    //!        t8〜t15 : システムテクスチャ（フレーム単位で固定）
    //--------------------------------------------------------------
    enum class SRVSlot : unsigned int {
        // --- マテリアルテクスチャ (t0〜t7) ---
        Albedo            = 0,    //!< t0 : アルベドマップ
        Normal            = 1,    //!< t1 : ノーマルマップ
        MetallicRoughness = 2,    //!< t2 : メタリック・ラフネスマップ (G=roughness, B=metallic)
        Emissive          = 3,    //!< t3 : エミッシブマップ
        AO                = 4,    //!< t4 : アンビエントオクルージョンマップ
        // t5〜t7 : 将来のマテリアルテクスチャ拡張枠

        // --- システムテクスチャ (t8〜t15) ---
        ShadowMap = 8,    //!< t8 : シャドウマップ

        // --- G-Buffer（ディファードLightingパスで読む。GBufferパスの出力と対応） ---
        GBufferAlbedo   = 9,     //!< t9  : G-Buffer0 (rgb: albedo)
        GBufferNormal   = 10,    //!< t10 : G-Buffer1 (rgb: ワールド法線, a: ShadingModel ID)
        GBufferMaterial = 11,    //!< t11 : G-Buffer2 (r: metallic, g: roughness, b: AO, a: specular)
        GBufferEmissive = 12,    //!< t12 : G-Buffer3 (rgb: emissive + リム発光)
        GBufferDepth    = 13,    //!< t13 : 深度（G-Bufferパスと共有するDSVのSRVビュー）
        GBufferWorldPos = 14,    //!< t14 : G-Buffer4 (rgb: ワールド座標。頂点シェーダー補間値そのまま)
        // t15 : IBL等の将来枠
    };

    //--------------------------------------------------------------
    //! @brief サンプラースロット (s*)
    //! @note  SRVSlotと同じブロック境界に揃える
    //--------------------------------------------------------------
    enum class SamplerSlot : unsigned int {
        // --- マテリアルサンプラー (s0〜s7) ---
        Material = 0,    //!< s0 : マテリアルテクスチャ共通サンプラー

        // --- システムサンプラー (s8〜s15) ---
        ShadowMap = 8,    //!< s8 : シャドウマップ用比較サンプラー（SampleCmpLevelZero用）
        GBuffer   = 9,    //!< s9 : G-Buffer読み取り用ポイントサンプラー（LightingパスでPointClampを使う）
    };
}    // namespace Tsukino::Renderer
