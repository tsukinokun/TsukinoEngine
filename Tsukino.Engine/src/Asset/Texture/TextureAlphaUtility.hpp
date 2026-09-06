//----------------------------------------------------------------------------
//! @file   TextureAlphaUtility.hpp
//! @brief  テクスチャのアルファチャンネルに関する共通処理
//! @detail 透明部分のふちの色にじみ対策と、カットアウト（アルファ抜き）の
//!         自動判定を提供します。単体画像を読む TextureImporter と、
//!         モデル埋め込みテクスチャを読む ModelImporter の双方から使います。
//----------------------------------------------------------------------------
#pragma once

#include <DirectXTex/DirectXTex.h>

#include <cstdint>
#include <cstring>
#include <vector>

// 名前空間 Tsukino::Asset::Detail
namespace Tsukino::Asset::Detail {

    //----------------------------------------------------------------------------
    //! 1ピクセル4バイト・8bit/チャンネルの形式かどうかを返します。
    //! アルファが常に末尾バイトに来る形式だけを対象にするための判定で、
    //! R/Bのチャンネル順はこのファイルの処理内容には影響しません。
    //! @param  [in] format 判定するフォーマット
    //! @return 対象の形式なら true。
    //----------------------------------------------------------------------------
    inline bool IsRGBA8Like(DXGI_FORMAT format) {
        switch(format) {
        case DXGI_FORMAT_R8G8B8A8_UNORM:
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
        case DXGI_FORMAT_B8G8R8A8_UNORM:
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
            return true;
        default:
            return false;
        }
    }

    //----------------------------------------------------------------------------
    //! 完全に透明なピクセルのRGBを、近傍の不透明なピクセルの色で塗り広げます。
    //! バイリニア補間サンプリング時に透明ピクセルの黒いRGBが不透明ピクセルの
    //! 色と混ざり込み、端が黒く滲んで見える問題への対策です。
    //! 対象外の形式が渡された場合は何もせずに返します。
    //! @param [in,out] image 塗り広げる対象の画像
    //----------------------------------------------------------------------------
    inline void DilateTransparentEdges(DirectX::ScratchImage& image) {
        const DirectX::Image* img = image.GetImage(0, 0, 0);
        if(!img) {
            return;
        }

        // 1ピクセル4バイト・8bit/チャンネルの形式以外は対象外（安全側に倒す）
        if(!IsRGBA8Like(img->format)) {
            return;
        }

        const size_t width    = img->width;
        const size_t height   = img->height;
        const size_t rowPitch = img->rowPitch;

        auto pixelAt = [rowPitch](uint8_t* base, size_t x, size_t y) -> uint8_t* {
            return base + y * rowPitch + x * 4;
        };

        std::vector<uint8_t> work(img->pixels, img->pixels + img->slicePitch);

        // バイリニア補間の影響範囲をカバーするため複数回にわたり外側へ色を伝播させる
        constexpr int kIterations = 3;
        for(int iter = 0; iter < kIterations; ++iter) {
            const std::vector<uint8_t> src = work;

            for(size_t y = 0; y < height; ++y) {
                for(size_t x = 0; x < width; ++x) {
                    const uint8_t* srcPixel = pixelAt(const_cast<uint8_t*>(src.data()), x, y);
                    if(srcPixel[3] != 0) {
                        continue;    // 不透明（もしくは半透明）ピクセルはそのまま維持
                    }

                    uint32_t sumR = 0, sumG = 0, sumB = 0, count = 0;
                    for(int dy = -1; dy <= 1; ++dy) {
                        for(int dx = -1; dx <= 1; ++dx) {
                            if(dx == 0 && dy == 0) {
                                continue;
                            }

                            const int64_t nx = static_cast<int64_t>(x) + dx;
                            const int64_t ny = static_cast<int64_t>(y) + dy;
                            if(nx < 0 || ny < 0 || nx >= static_cast<int64_t>(width) || ny >= static_cast<int64_t>(height)) {
                                continue;
                            }

                            const uint8_t* neighbor = pixelAt(const_cast<uint8_t*>(src.data()), static_cast<size_t>(nx), static_cast<size_t>(ny));
                            if(neighbor[3] == 0) {
                                continue;    // 近傍も透明なら色の供給元にしない
                            }

                            sumR += neighbor[0];
                            sumG += neighbor[1];
                            sumB += neighbor[2];
                            ++count;
                        }
                    }

                    if(count == 0) {
                        continue;    // 今回は塗れなかった（次のイテレーションで伝播を試みる）
                    }

                    uint8_t* dstPixel = pixelAt(work.data(), x, y);
                    dstPixel[0]       = static_cast<uint8_t>(sumR / count);
                    dstPixel[1]       = static_cast<uint8_t>(sumG / count);
                    dstPixel[2]       = static_cast<uint8_t>(sumB / count);
                    // alpha(dstPixel[3])は0のまま変更しない
                }
            }
        }

        memcpy(img->pixels, work.data(), img->slicePitch);
    }

    //----------------------------------------------------------------------------
    //! 画像がカットアウト（アルファで抜く形状）を含むかどうかを判定します。
    //! ほぼ不透明なテクスチャを誤検出しないよう、透明テクセルが一定の割合を
    //! 超えたときだけ真を返します。対象外の形式は常に偽を返します。
    //! @param  [in] image          判定する画像
    //! @param  [in] ratioThreshold 透明とみなすテクセルの割合のしきい値
    //! @return カットアウトを含むなら true。
    //----------------------------------------------------------------------------
    inline bool HasCutoutAlpha(const DirectX::ScratchImage& image, float ratioThreshold = 0.0005f) {
        const DirectX::Image* img = image.GetImage(0, 0, 0);
        if(!img) {
            return false;
        }

        if(!IsRGBA8Like(img->format)) {
            return false;    // アルファチャンネルを持たない形式
        }

        const size_t width  = img->width;
        const size_t height = img->height;
        if(width == 0 || height == 0) {
            return false;
        }

        // アルファが中間値のテクセル（ふち）だけの画像もあるため、
        // 「ほぼ透明」と言える 128 未満を数える
        size_t transparentCount = 0;
        for(size_t y = 0; y < height; ++y) {
            const uint8_t* row = img->pixels + y * img->rowPitch;
            for(size_t x = 0; x < width; ++x) {
                if(row[x * 4 + 3] < 128) {
                    ++transparentCount;
                }
            }
        }

        const double ratio = static_cast<double>(transparentCount) / static_cast<double>(width * height);
        return ratio > static_cast<double>(ratioThreshold);
    }

}    // namespace Tsukino::Asset::Detail
