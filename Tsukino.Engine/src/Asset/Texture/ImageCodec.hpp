//----------------------------------------------------------------------------
//! @file   ImageCodec.hpp
//! @brief  画像の読み込み・加工・圧縮・書き出し（Engine の内部用。bimg を使う）
//! @detail テクスチャ・キューブマップ・モデルに埋め込まれた画像のインポーターと、
//!         テクスチャのローダーが使います。どの OS でも同じコードで動きます。
//!         キャッシュは KTX（色空間・ミップ・圧縮形式を持てて、スマホの圧縮形式も入る入れ物）で書きます。
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/Core/Path.hpp>
#include <Tsukino/Core/typedef.hpp>
#include <Tsukino/GraphicsCommon/Texture/TextureFormat.hpp>

#include <array>
#include <cstddef>
#include <vector>

// 名前空間 Tsukino::Asset::Detail
namespace Tsukino::Asset::Detail {

    //----------------------------------------------------------------------------
    //! 1ピクセル4バイト（R, G, B, A の順）の、圧縮していない画像です。
    //----------------------------------------------------------------------------
    struct Rgba8Image {
        u32             width  = 0;    //!< 幅（ピクセル）
        u32             height = 0;    //!< 高さ（ピクセル）
        std::vector<u8> pixels;        //!< 左上から行ごとに並べた画素（width × height × 4 バイト）
    };

    //----------------------------------------------------------------------------
    //! キャッシュへ書くときの加工の指定です。
    //----------------------------------------------------------------------------
    struct ImageEncodeOptions {
        bool srgb         = true;     //!< 色の画像なら true（ミップは線形に直して平均し、KTX に sRGB と書く）
        bool generateMips = false;    //!< 1×1 までのミップを作るか
        //! 圧縮の形式（Unknown なら圧縮しない）。sRGB かどうかは srgb で決まるので、_sRGB の付かない方で指定してよい
        Tsukino::GraphicsCommon::TextureFormat compressedFormat = Tsukino::GraphicsCommon::TextureFormat::Unknown;
    };

    //! 画像ファイル（png / jpg / tga / bmp / dds / ktx / hdr / exr など）の中身を RGBA8 にします。
    //! @param  [in]  data 画像ファイルの中身
    //! @param  [in]  size data のバイト数
    //! @param  [out] out  読めた画像
    //! @return 読めたら true
    [[nodiscard]]
    bool DecodeImage(const void* data, std::size_t size, Rgba8Image& out);

    //! 画像の大きさを変えます（面積で平均する。縮小しても細かい模様がちらつかない）。
    //! @param  [in] source 元の画像
    //! @param  [in] width  新しい幅
    //! @param  [in] height 新しい高さ
    //! @param  [in] srgb   色の画像なら true（線形に直してから平均する）
    //! @return 大きさを変えた画像
    [[nodiscard]]
    Rgba8Image ResizeImage(const Rgba8Image& source, u32 width, u32 height, bool srgb);

    //! 完全に透明な画素の RGB を、近くの不透明な画素の色で塗り広げます。
    //! バイリニア補間で透明な画素の黒が混ざり、ふちが黒くにじむのを防ぎます。
    //! @param  [in,out] image 塗り広げる画像
    void DilateTransparentEdges(Rgba8Image& image);

    //! 画像がカットアウト（アルファで抜く形）を含むかを返します。
    //! ほぼ不透明な画像を誤検出しないよう、透明な画素が一定の割合を超えたときだけ真を返します。
    //! @param  [in] image          調べる画像
    //! @param  [in] ratioThreshold 透明とみなす画素の割合のしきい値
    //! @return カットアウトを含むなら true
    [[nodiscard]]
    bool HasCutoutAlpha(const Rgba8Image& image, float ratioThreshold = 0.0005f);

    //! 画像を加工して KTX で書き出します（足りないフォルダは作る）。
    //! @param  [in] path    書き出す先
    //! @param  [in] image   元の画像
    //! @param  [in] options 加工の指定
    //! @return 書けたら true
    [[nodiscard]]
    bool WriteTextureKtx(const Tsukino::Core::Path& path, const Rgba8Image& image, const ImageEncodeOptions& options);

    //! 6面をキューブマップにまとめ、加工して KTX で書き出します（足りないフォルダは作る）。
    //! @param  [in] path    書き出す先
    //! @param  [in] faces   +X, -X, +Y, -Y, +Z, -Z の順の6面（同じ大きさであること）
    //! @param  [in] options 加工の指定
    //! @return 書けたら true
    [[nodiscard]]
    bool WriteCubemapKtx(const Tsukino::Core::Path& path, const std::array<Rgba8Image, 6>& faces, const ImageEncodeOptions& options);

    //----------------------------------------------------------------------------
    //! テクスチャのファイル（KTX / DDS）の中身を、形式を変えずに読んだものです。
    //----------------------------------------------------------------------------
    struct TextureFileData {
        //! 1段ぶんの位置と大きさ
        struct Mip {
            std::size_t offset   = 0;    //!< pixels の中の先頭
            std::size_t size     = 0;    //!< バイト数
            u32         width    = 0;    //!< 幅（ピクセル）
            u32         height   = 0;    //!< 高さ（ピクセル）
            u32         rowPitch = 0;    //!< 1行（圧縮形式は4ピクセルの1列）のバイト数
        };

        Tsukino::GraphicsCommon::TextureFormat format = Tsukino::GraphicsCommon::TextureFormat::Unknown;    //!< 形式（sRGB かどうかを含む）
        std::vector<Mip>                       mips;      //!< 最上段から順の段（配列・キューブは先頭の1枚だけ）
        std::vector<u8>                        pixels;    //!< 全段を詰めたデータ
    };

    //! テクスチャのファイルを読みます。エンジンが扱わない形式は RGBA8 に直します。
    //! @param  [in]  data ファイルの中身
    //! @param  [in]  size data のバイト数
    //! @param  [out] out  読めたデータ
    //! @return 読めたら true
    [[nodiscard]]
    bool ReadTextureFile(const void* data, std::size_t size, TextureFileData& out);

    //! テクスチャのファイルの先頭から、形式と段の数だけを読みます（中身は読まない）。
    //! @param  [in]  data      ファイルの先頭（ヘッダを含む長さがあればよい）
    //! @param  [in]  size      data のバイト数
    //! @param  [out] format    形式（sRGB かどうかを含む）
    //! @param  [out] mipCount  段の数
    //! @return 読めたら true
    [[nodiscard]]
    bool ReadTextureFileInfo(const void* data, std::size_t size, Tsukino::GraphicsCommon::TextureFormat& format, u32& mipCount);

}    // namespace Tsukino::Asset::Detail
