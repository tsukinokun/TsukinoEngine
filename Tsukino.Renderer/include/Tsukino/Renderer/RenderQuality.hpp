//----------------------------------------------------------------------------
//! @file   RenderQuality.hpp
//! @brief  描画品質のプリセットとグラフィックスデバイスの情報
//! @detail 専用 GPU が無い環境でも動かすために、描画の重さを段階で下げる設定をまとめます。
//!         デバイスの作り方の指定と、作られたアダプタの情報もここに置きます。
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/Core/typedef.hpp>

#include <string>

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {

    //------------------------------------------------------------------------
    //! グラフィックスデバイスの選び方です。
    //------------------------------------------------------------------------
    enum class GraphicsDevicePreference : u8 {
        Auto,             //!< 高性能なハードウェアを優先し、作れなければ WARP（ソフトウェア描画）へ落とす
        ForceSoftware,    //!< 最初から WARP で作る（GPU が無い環境の確認用）
    };

    //------------------------------------------------------------------------
    //! グラフィックスデバイスの作り方の指定です。
    //------------------------------------------------------------------------
    struct GraphicsDeviceOptions {
        GraphicsDevicePreference preference = GraphicsDevicePreference::Auto;    //!< デバイスの選び方
    };

    //------------------------------------------------------------------------
    //! 作成したグラフィックスデバイスのアダプタ情報です。
    //------------------------------------------------------------------------
    struct GraphicsAdapterInfo {
        std::string name;                       //!< アダプタの名前（UTF-8）
        u32         vendorId         = 0;       //!< ベンダー ID
        u32         deviceId         = 0;       //!< デバイス ID
        bool        isSoftware       = false;   //!< ソフトウェア描画（WARP や Basic Render Driver）か
        bool        isUnifiedMemory  = false;   //!< CPU とメモリを共有する GPU（内蔵 GPU など）か
        u64         dedicatedMemory  = 0;       //!< 専用ビデオメモリ（バイト）
        u64         sharedMemory     = 0;       //!< 共有システムメモリ（バイト）
    };

    //------------------------------------------------------------------------
    //! 描画品質の段階です（上が高品質）。
    //------------------------------------------------------------------------
    enum class RenderQualityLevel : u8 {
        High,       //!< 等倍解像度・全カスケード・全段ブルーム（品質を下げない）
        Medium,     //!< 影とブルームを少し抑える
        Low,        //!< 内部解像度を下げ、影を簡略化する（内蔵 GPU 向け）
        Minimum,    //!< ブルームを切り、影を最小にする（ソフトウェア描画向け）
    };

    //------------------------------------------------------------------------
    //! 描画品質の設定です。
    //! @note  既定値は「品質を下げない」値で、Renderer の初期状態と同じ
    //------------------------------------------------------------------------
    struct RenderQualitySettings {
        float renderScale        = 1.0f;     //!< 3D を描く内部解像度の倍率（kMinRenderScale〜1。UI は常に等倍）
        u32   shadowMapSizeLimit = 0;        //!< シャドウマップの一辺の上限（0 で上限なし。シーンの値より大きくはしない）
        u32   shadowCascadeCount = 4;        //!< 実際に描くカスケードの枚数（1〜kShadowCascadeCount）
        bool  shadowSoftFilter   = true;     //!< true: 影を 3x3 で滑らかにする, false: 1タップ
        u32   bloomMipCount      = 6;        //!< ブルームの縮小段の上限（0 でブルームを切る）
        bool  motionBlur         = true;     //!< モーションブラーを許すか
        bool  drawSky            = true;     //!< 空を毎フレーム描くか（false なら背景はクリアカラー。環境光のベイクは行う）
        u32   textureMipSkip     = 0;        //!< 切り捨てるテクスチャの上位ミップの段数（読み込み時に効く）

        static constexpr float kMinRenderScale = 0.25f;    //!< renderScale の下限
    };

    //------------------------------------------------------------------------
    //! 段階に対応する描画品質の設定を返します。
    //! @param  [in] level 段階
    //! @return 描画品質の設定
    //------------------------------------------------------------------------
    [[nodiscard]]
    constexpr RenderQualitySettings GetRenderQualityPreset(RenderQualityLevel level) noexcept {
        RenderQualitySettings settings;
        switch(level) {
            case RenderQualityLevel::High:
                break;
            case RenderQualityLevel::Medium:
                settings.shadowMapSizeLimit = 2048;
                settings.shadowCascadeCount = 3;
                settings.bloomMipCount      = 4;
                break;
            case RenderQualityLevel::Low:
                settings.renderScale        = 0.75f;
                settings.shadowMapSizeLimit = 1024;
                settings.shadowCascadeCount = 2;
                settings.shadowSoftFilter   = false;
                settings.bloomMipCount      = 3;
                settings.motionBlur         = false;
                settings.textureMipSkip     = 1;
                break;
            case RenderQualityLevel::Minimum:
                settings.renderScale        = 0.6f;
                settings.shadowMapSizeLimit = 512;
                settings.shadowCascadeCount = 1;
                settings.shadowSoftFilter   = false;
                settings.bloomMipCount      = 0;
                settings.motionBlur         = false;
                settings.drawSky            = false;
                settings.textureMipSkip     = 1;
                break;
        }
        return settings;
    }

    //------------------------------------------------------------------------
    //! アダプタの性質から、最初に使う描画品質の段階を選びます。
    //! @param  [in] adapter アダプタ情報
    //! @return 推奨の段階
    //------------------------------------------------------------------------
    [[nodiscard]]
    constexpr RenderQualityLevel RecommendRenderQuality(const GraphicsAdapterInfo& adapter) noexcept {
        constexpr u64 kOneGiB = 1024ull * 1024ull * 1024ull;
        if(adapter.isSoftware)
            return RenderQualityLevel::Minimum;
        if(adapter.isUnifiedMemory || adapter.dedicatedMemory < kOneGiB)
            return RenderQualityLevel::Low;
        if(adapter.dedicatedMemory < 2 * kOneGiB)
            return RenderQualityLevel::Medium;
        return RenderQualityLevel::High;
    }

    //------------------------------------------------------------------------
    //! 段階の名前を返します（ログ用）。
    //! @param  [in] level 段階
    //! @return 英語の名前
    //------------------------------------------------------------------------
    [[nodiscard]]
    constexpr const char* GetRenderQualityName(RenderQualityLevel level) noexcept {
        switch(level) {
            case RenderQualityLevel::High:    return "High";
            case RenderQualityLevel::Medium:  return "Medium";
            case RenderQualityLevel::Low:     return "Low";
            case RenderQualityLevel::Minimum: return "Minimum";
        }
        return "Unknown";
    }

}    // namespace Tsukino::Renderer
