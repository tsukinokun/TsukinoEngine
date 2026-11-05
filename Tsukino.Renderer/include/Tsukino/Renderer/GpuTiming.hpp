//----------------------------------------------------------------------------
//! @file   GpuTiming.hpp
//! @brief  パス単位のGPU時間の計測結果
//! @detail Renderer::Render() の各パスにかかったGPU時間を、パス名とミリ秒の固定配列で表します。
//!         計測は GPU のタイムスタンプクエリで行い、結果は数フレーム遅れて届きます。
//!         Renderer::GetFrameStats().gpu から読みます。
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/Core/typedef.hpp>

#include <array>
#include <cstddef>

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {
    //------------------------------------------------------------------------
    //! GPU時間を計測するパスの種類です。
    //! @note  並びは Renderer::Render() の実行順。Count は数を表すための番兵
    //------------------------------------------------------------------------
    enum class GpuPass : u8 {
        Shadow,         //!< シャドウマップ
        Sky,            //!< スカイ（大気散乱）
        GBuffer,        //!< GBuffer（不透明3Dモデル）
        Lighting,       //!< ディファードライティング
        World,          //!< フォワード不透明と環境パーティクル
        Fog,            //!< フォグ
        ColorGrade,     //!< 球形の色調補正
        Transparent,    //!< 半透明（深度事前パスを含む）
        MotionBlur,     //!< モーションブラー
        Bloom,          //!< ブルーム
        Tonemap,        //!< トーンマップ
        Effects,        //!< エフェクト（IPostWorldPass）
        Overlay,        //!< UI・フォント・画面上の3Dモデル

        Count,
    };

    //! 計測するパスの数
    inline constexpr std::size_t kGpuPassCount = static_cast<std::size_t>(GpuPass::Count);

    //------------------------------------------------------------------------
    //! パスの表示名を返します。
    //! @param  [in] pass パスの種類
    //! @return 英字の表示名。範囲外なら "?"
    //------------------------------------------------------------------------
    [[nodiscard]]
    constexpr const char* GetGpuPassName(GpuPass pass) noexcept {
        constexpr std::array<const char*, kGpuPassCount> kNames = {
            "Shadow", "Sky", "GBuffer", "Lighting", "World", "Fog", "ColorGrade",
            "Transparent", "MotionBlur", "Bloom", "Tonemap", "Effects", "Overlay",
        };
        const std::size_t index = static_cast<std::size_t>(pass);
        return (index < kGpuPassCount) ? kNames[index] : "?";
    }

    //------------------------------------------------------------------------
    //! 1パス分のGPU時間です。
    //------------------------------------------------------------------------
    struct GpuPassTiming {
        const char* name = "";      //!< パスの表示名（GetGpuPassName の値）
        float       ms   = 0.0f;    //!< GPU時間（ミリ秒）
    };

    //------------------------------------------------------------------------
    //! 1フレーム分のGPU時間です。
    //! @note  GPU の結果を待たないよう、2フレーム以上前に描いたフレームの値が入る。
    //!        GPU の周波数が途中で変わったフレーム（disjoint）や、結果が間に合わなかった
    //!        フレームは捨て、最後に取れた値を持ち越す
    //------------------------------------------------------------------------
    struct GpuFrameTimings {
        std::array<GpuPassTiming, kGpuPassCount> passes{};    //!< パスごとの時間（GpuPass の順）
        float                                    frameMs = 0.0f;    //!< フレーム全体（パスの間の処理も含む）
        bool                                     valid   = false;   //!< 一度でも計測結果が取れたか
    };
}    // namespace Tsukino::Renderer
