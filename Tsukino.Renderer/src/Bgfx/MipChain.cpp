//----------------------------------------------------------------------------
//! @file   MipChain.cpp
//! @brief  1段だけの画像から、1×1 までのミップを CPU で作る関数の実装
//! @detail 1段ずつ、上の段の 2×2 の画素を平均して半分の大きさの段を作ります。
//!         sRGB の画像は、GPU が sRGB のテクスチャを縮小するときと同じく、線形に直してから平均します。
//----------------------------------------------------------------------------
#include "MipChain.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {

    namespace {
        //! sRGB の 8bit 値 → 線形の値（0〜1）の表を作ります。
        //! @return 256 個の値
        std::array<float, 256> MakeSrgbToLinearTable() {
            std::array<float, 256> table{};
            for(u32 i = 0; i < 256; ++i) {
                const float c = static_cast<float>(i) / 255.0f;
                table[i]      = c <= 0.04045f ? c / 12.92f : std::pow((c + 0.055f) / 1.055f, 2.4f);
            }
            return table;
        }

        //! sRGB の 8bit 値の境目（i と i+1 の間 = (i + 0.5) / 255）を線形の値にした表を作ります。
        //! @return 255 個の値（昇順）
        //! @note   線形の値がいくつの境目を超えるかを数えると、sRGB に直して四捨五入した 8bit 値になる。
        //!         画素ごとに pow を呼ぶと大きな画像で遅い（2048x1280 で Debug 130ms ほど）ので、表を二分探索する
        std::array<float, 255> MakeLinearToSrgbThresholds() {
            std::array<float, 255> table{};
            for(u32 i = 0; i < 255; ++i) {
                const double c = (static_cast<double>(i) + 0.5) / 255.0;
                table[i]       = static_cast<float>(c <= 0.04045 ? c / 12.92 : std::pow((c + 0.055) / 1.055, 2.4));
            }
            return table;
        }

        //! 線形の値（0〜1）を sRGB の 8bit 値に直します。
        //! @param  [in] linear     線形の値
        //! @param  [in] thresholds MakeLinearToSrgbThresholds() の表
        //! @return sRGB の 8bit 値
        u8 LinearToSrgb(float linear, const std::array<float, 255>& thresholds) {
            // linear 以下の境目の数（std::upper_bound と同じ。Debug ビルドでも遅くならないよう素の配列で探す）
            const float* table = thresholds.data();
            u32          low   = 0;
            u32          high  = 255;
            while(low < high) {
                const u32 mid = (low + high) / 2;
                if(table[mid] <= linear)
                    low = mid + 1;
                else
                    high = mid;
            }
            return static_cast<u8>(low);
        }
    }    // namespace

    //------------------------------------------------------------------------
    //! 1画素 8bit × channels の画像から、1×1 までのミップを作ります。
    //------------------------------------------------------------------------
    std::vector<u8> BuildMipChain(u32 channels, bool srgb, u32 width, u32 height, const u8* top, u32 rowPitch) {
        static const std::array<float, 256> kSrgbToLinear     = MakeSrgbToLinearTable();
        static const std::array<float, 255> kLinearThresholds = MakeLinearToSrgbThresholds();

        //--------------------------------------------------------------------
        // 全段の大きさを先に数えて、まとめて確保する
        //--------------------------------------------------------------------
        size_t total = 0;
        for(u32 w = width, h = height;; w = std::max(1u, w / 2), h = std::max(1u, h / 2)) {
            total += static_cast<size_t>(w) * h * channels;
            if(w == 1 && h == 1)
                break;
        }
        std::vector<u8> result(total);

        // いちばん上の段は、行のすき間を詰めて写すだけ
        const size_t topPitch = static_cast<size_t>(width) * channels;
        for(u32 y = 0; y < height; ++y) std::memcpy(result.data() + topPitch * y, top + static_cast<size_t>(rowPitch) * y, topPitch);

        // 4成分のときは最後の1つ（アルファ）を線形のまま平均する
        const u32 colorChannels = (srgb && channels == 4) ? 3 : (srgb ? channels : 0);

        //--------------------------------------------------------------------
        // 上の段の 2×2 を平均して、次の段を作る（奇数の端は同じ画素を2回使う）
        //--------------------------------------------------------------------
        size_t srcOffset = 0;
        u32    srcWidth  = width;
        u32    srcHeight = height;
        while(srcWidth > 1 || srcHeight > 1) {
            const u32    dstWidth  = std::max(1u, srcWidth / 2);
            const u32    dstHeight = std::max(1u, srcHeight / 2);
            const size_t dstOffset = srcOffset + static_cast<size_t>(srcWidth) * srcHeight * channels;
            const u8*    src       = result.data() + srcOffset;
            u8*          dst       = result.data() + dstOffset;

            for(u32 y = 0; y < dstHeight; ++y) {
                const u32 y0 = std::min(y * 2, srcHeight - 1);
                const u32 y1 = std::min(y * 2 + 1, srcHeight - 1);
                for(u32 x = 0; x < dstWidth; ++x) {
                    const u32 x0   = std::min(x * 2, srcWidth - 1);
                    const u32 x1   = std::min(x * 2 + 1, srcWidth - 1);
                    const u8* p[4] = {
                        src + (static_cast<size_t>(y0) * srcWidth + x0) * channels,
                        src + (static_cast<size_t>(y0) * srcWidth + x1) * channels,
                        src + (static_cast<size_t>(y1) * srcWidth + x0) * channels,
                        src + (static_cast<size_t>(y1) * srcWidth + x1) * channels,
                    };
                    u8* out = dst + (static_cast<size_t>(y) * dstWidth + x) * channels;
                    for(u32 c = 0; c < channels; ++c) {
                        if(c < colorChannels) {
                            const float sum = kSrgbToLinear[p[0][c]] + kSrgbToLinear[p[1][c]] + kSrgbToLinear[p[2][c]] + kSrgbToLinear[p[3][c]];
                            out[c]          = LinearToSrgb(sum * 0.25f, kLinearThresholds);
                        } else {
                            out[c] = static_cast<u8>((static_cast<u32>(p[0][c]) + p[1][c] + p[2][c] + p[3][c] + 2) / 4);
                        }
                    }
                }
            }

            srcOffset = dstOffset;
            srcWidth  = dstWidth;
            srcHeight = dstHeight;
        }
        return result;
    }

}    // namespace Tsukino::Renderer
