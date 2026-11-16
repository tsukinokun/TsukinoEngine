//----------------------------------------------------------------------------
//! @file   ImageCodec.cpp
//! @brief  画像の読み込み・加工・圧縮・書き出しの実装（bimg）
//----------------------------------------------------------------------------
#include "ImageCodec.hpp"

#include <Tsukino/Core/IO/FileSystem.hpp>
#include <Tsukino/Core/Log.hpp>

#include <bimg/bimg.h>
#include <bimg/decode.h>
#include <bimg/encode.h>
#include <bx/allocator.h>
#include <bx/error.h>
#include <bx/readerwriter.h>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <memory>
#include <utility>

// 名前空間 Tsukino::Asset::Detail
namespace Tsukino::Asset::Detail {
    namespace {
        using Tsukino::GraphicsCommon::TextureFormat;

        //! bimg に渡すメモリの確保先（どのスレッドから使ってもよい）
        bx::AllocatorI* Allocator() {
            static bx::DefaultAllocator allocator;
            return &allocator;
        }

        //! bimg の画像を自動で解放するための削除子
        struct ContainerDeleter {
            void operator()(bimg::ImageContainer* container) const {
                if(container)
                    bimg::imageFree(container);
            }
        };
        using ContainerPtr = std::unique_ptr<bimg::ImageContainer, ContainerDeleter>;

        //----------------------------------------------------------------------------
        //! エンジンの形式と bimg の形式（＋ sRGB かどうか）の対応です。
        //----------------------------------------------------------------------------
        struct FormatPair {
            TextureFormat              engine;
            bimg::TextureFormat::Enum  bimg;
            bool                       srgb;
        };
        constexpr FormatPair kFormatPairs[] = {
            {TextureFormat::R8,         bimg::TextureFormat::R8,      false},
            {TextureFormat::RG8,        bimg::TextureFormat::RG8,     false},
            {TextureFormat::RGBA8,      bimg::TextureFormat::RGBA8,   false},
            {TextureFormat::RGBA8_sRGB, bimg::TextureFormat::RGBA8,   true },
            {TextureFormat::BGRA8,      bimg::TextureFormat::BGRA8,   false},
            {TextureFormat::BGRA8_sRGB, bimg::TextureFormat::BGRA8,   true },
            {TextureFormat::R16,        bimg::TextureFormat::R16,     false},
            {TextureFormat::RGBA16,     bimg::TextureFormat::RGBA16,  false},
            {TextureFormat::R16F,       bimg::TextureFormat::R16F,    false},
            {TextureFormat::RGBA16F,    bimg::TextureFormat::RGBA16F, false},
            {TextureFormat::R32F,       bimg::TextureFormat::R32F,    false},
            {TextureFormat::RGBA32F,    bimg::TextureFormat::RGBA32F, false},
            {TextureFormat::RGB10A2,    bimg::TextureFormat::RGB10A2, false},
            {TextureFormat::BC1,        bimg::TextureFormat::BC1,     false},
            {TextureFormat::BC1_sRGB,   bimg::TextureFormat::BC1,     true },
            {TextureFormat::BC3,        bimg::TextureFormat::BC3,     false},
            {TextureFormat::BC3_sRGB,   bimg::TextureFormat::BC3,     true },
            {TextureFormat::BC4,        bimg::TextureFormat::BC4,     false},
            {TextureFormat::BC5,        bimg::TextureFormat::BC5,     false},
            {TextureFormat::BC7,        bimg::TextureFormat::BC7,     false},
            {TextureFormat::BC7_sRGB,   bimg::TextureFormat::BC7,     true },
            {TextureFormat::ASTC4x4,      bimg::TextureFormat::ASTC4x4, false},
            {TextureFormat::ASTC4x4_sRGB, bimg::TextureFormat::ASTC4x4, true },
            {TextureFormat::ASTC6x6,      bimg::TextureFormat::ASTC6x6, false},
            {TextureFormat::ASTC6x6_sRGB, bimg::TextureFormat::ASTC6x6, true },
        };

        //! bimg の形式と sRGB かどうかから、エンジンの形式を返します（無ければ Unknown）。
        TextureFormat FromBimg(bimg::TextureFormat::Enum format, bool srgb) {
            for(const FormatPair& pair : kFormatPairs) {
                if(pair.bimg == format && pair.srgb == srgb)
                    return pair.engine;
            }
            // sRGB を持てない形式（R8 など）は、印が付いていても線形として扱う
            for(const FormatPair& pair : kFormatPairs) {
                if(pair.bimg == format && !pair.srgb)
                    return pair.engine;
            }
            return TextureFormat::Unknown;
        }

        //! エンジンの形式に当たる bimg の形式を返します（無ければ Count）。
        bimg::TextureFormat::Enum ToBimg(TextureFormat format) {
            for(const FormatPair& pair : kFormatPairs) {
                if(pair.engine == format)
                    return pair.bimg;
            }
            return bimg::TextureFormat::Count;
        }

        //----------------------------------------------------------------------------
        //! sRGB の値（0〜255）を線形（0〜1）にする表を返します。
        //----------------------------------------------------------------------------
        const std::array<float, 256>& SrgbToLinearTable() {
            static const std::array<float, 256> table = [] {
                std::array<float, 256> values{};
                for(int i = 0; i < 256; ++i) {
                    const float c = static_cast<float>(i) / 255.0f;
                    values[i]     = (c <= 0.04045f) ? c / 12.92f : std::pow((c + 0.055f) / 1.055f, 2.4f);
                }
                return values;
            }();
            return table;
        }

        //! 線形（0〜1）を sRGB の値（0〜255）にします。
        u8 LinearToSrgb(float value) {
            value         = std::clamp(value, 0.0f, 1.0f);
            const float c = (value <= 0.0031308f) ? value * 12.92f : 1.055f * std::pow(value, 1.0f / 2.4f) - 0.055f;
            return static_cast<u8>(std::lround(c * 255.0f));
        }

        //----------------------------------------------------------------------------
        //! RGBA8 の画像（1枚または6面）から bimg の画像を作り、ミップと圧縮を掛けます。
        //! @param  [in] faces   面（1枚なら普通のテクスチャ、6枚ならキューブマップ）
        //! @param  [in] options 加工の指定
        //! @return 加工した画像（失敗したら nullptr）
        //----------------------------------------------------------------------------
        ContainerPtr BuildContainer(const std::vector<const Rgba8Image*>& faces, const ImageEncodeOptions& options) {
            const bool cubeMap = faces.size() == 6;
            const u32  width   = faces.front()->width;
            const u32  height  = faces.front()->height;

            ContainerPtr container(bimg::imageAlloc(Allocator(), bimg::TextureFormat::RGBA8, static_cast<u16>(width), static_cast<u16>(height), 1, 1,
                                                    cubeMap, false));
            if(!container)
                return nullptr;

            for(std::size_t side = 0; side < faces.size(); ++side) {
                bimg::ImageMip mip{};
                if(!bimg::imageGetRawData(*container, static_cast<u16>(side), 0, container->m_data, container->m_size, mip))
                    return nullptr;
                std::memcpy(const_cast<u8*>(mip.m_data), faces[side]->pixels.data(), static_cast<std::size_t>(width) * height * 4);
            }

            // ミップ（sRGB の色は線形にしてから平均する。そのまま平均すると縮んだ段が暗くなる）
            if(options.generateMips) {
                ContainerPtr mips(bimg::imageGenerateMips(Allocator(), *container, options.srgb));
                if(!mips)
                    return nullptr;
                container = std::move(mips);
            }

            // 圧縮
            if(options.compressedFormat != TextureFormat::Unknown) {
                bimg::TextureFormat::Enum target = ToBimg(options.compressedFormat);
                if(target == bimg::TextureFormat::BC7) {
                    // bimg は BC7 を作れない。アルファも持てる BC3 で代える
                    Tsukino::Core::Log::Warn("ImageCodec: BC7 encoding is not available. Using BC3 instead.");
                    target = bimg::TextureFormat::BC3;
                }
                if(target == bimg::TextureFormat::Count || !bimg::isCompressed(target)) {
                    Tsukino::Core::Log::Error("ImageCodec: Unsupported compressed format.");
                    return nullptr;
                }

                //------------------------------------------------------------
                // 速い探索で圧縮する（libsquish の range fit）。初回起動で数千枚を変換するので、
                // 丁寧な探索（cluster fit）では待ちが長すぎる。画質は以前の DirectXTex の既定と同じ程度
                //------------------------------------------------------------
                bx::Error    error;
                ContainerPtr encoded(bimg::imageEncode(Allocator(), target, bimg::Quality::Fastest, *container, &error));
                if(!encoded || !error.isOk())
                    return nullptr;
                container = std::move(encoded);
            }

            container->m_srgb = options.srgb;
            return container;
        }

        //----------------------------------------------------------------------------
        //! bimg の画像を KTX にしてファイルへ書きます。
        //----------------------------------------------------------------------------
        bool WriteKtx(const Tsukino::Core::Path& path, bimg::ImageContainer& container) {
            bx::MemoryBlock  block(Allocator());
            bx::MemoryWriter writer(&block);
            bx::Error        error;
            bimg::imageWriteKtx(&writer, container, container.m_data, container.m_size, &error);
            if(!error.isOk())
                return false;

            const int64_t size = writer.seek(0, bx::Whence::Current);
            return Tsukino::IO::FileSystem::WriteBinary(path, block.more(0), static_cast<std::size_t>(size));
        }

        //! 4 の倍数へ切り上げます（BC 圧縮の最上段は4の倍数の大きさが要る）。
        u32 RoundUpTo4(u32 value) {
            return (value + 3u) & ~3u;
        }

        //! BC 圧縮の形式かを返します（ASTC は大きさの決まりが無い）。
        bool IsBcFormat(TextureFormat format) {
            const bimg::TextureFormat::Enum target = ToBimg(format);
            return target >= bimg::TextureFormat::BC1 && target <= bimg::TextureFormat::BC7;
        }
    }    // namespace

    //----------------------------------------------------------------------------
    //! 画像ファイルの中身を RGBA8 にします。
    //----------------------------------------------------------------------------
    bool DecodeImage(const void* data, std::size_t size, Rgba8Image& out) {
        if(!data || size == 0)
            return false;

        bx::Error    error;
        ContainerPtr container(bimg::imageParse(Allocator(), data, static_cast<u32>(size), bimg::TextureFormat::RGBA8, &error));
        if(!container || !error.isOk())
            return false;

        bimg::ImageMip mip{};
        if(!bimg::imageGetRawData(*container, 0, 0, container->m_data, container->m_size, mip))
            return false;

        out.width  = mip.m_width;
        out.height = mip.m_height;
        out.pixels.assign(mip.m_data, mip.m_data + static_cast<std::size_t>(mip.m_width) * mip.m_height * 4);
        return true;
    }

    //----------------------------------------------------------------------------
    //! 画像の大きさを変えます（面積で平均する）。
    //----------------------------------------------------------------------------
    Rgba8Image ResizeImage(const Rgba8Image& source, u32 width, u32 height, bool srgb) {
        Rgba8Image result;
        result.width  = width;
        result.height = height;
        result.pixels.resize(static_cast<std::size_t>(width) * height * 4);
        if(source.width == 0 || source.height == 0 || width == 0 || height == 0)
            return result;

        const std::array<float, 256>& toLinear = SrgbToLinearTable();
        const float                   scaleX   = static_cast<float>(source.width) / static_cast<float>(width);
        const float                   scaleY   = static_cast<float>(source.height) / static_cast<float>(height);

        for(u32 y = 0; y < height; ++y) {
            // この画素が覆う元の範囲（縦）。拡大のときも最低1画素を見る
            const float y0 = static_cast<float>(y) * scaleY;
            const float y1 = std::max(y0 + 1e-4f, static_cast<float>(y + 1) * scaleY);
            for(u32 x = 0; x < width; ++x) {
                const float x0 = static_cast<float>(x) * scaleX;
                const float x1 = std::max(x0 + 1e-4f, static_cast<float>(x + 1) * scaleX);

                float sum[4]   = {};
                float totalW   = 0.0f;
                for(u32 sy = static_cast<u32>(y0); sy < source.height && static_cast<float>(sy) < y1; ++sy) {
                    const float wy = std::min(y1, static_cast<float>(sy + 1)) - std::max(y0, static_cast<float>(sy));
                    for(u32 sx = static_cast<u32>(x0); sx < source.width && static_cast<float>(sx) < x1; ++sx) {
                        const float wx = std::min(x1, static_cast<float>(sx + 1)) - std::max(x0, static_cast<float>(sx));
                        const float w  = wx * wy;
                        const u8*   p  = &source.pixels[(static_cast<std::size_t>(sy) * source.width + sx) * 4];
                        for(int c = 0; c < 3; ++c)
                            sum[c] += w * (srgb ? toLinear[p[c]] : static_cast<float>(p[c]) / 255.0f);
                        sum[3] += w * static_cast<float>(p[3]) / 255.0f;
                        totalW += w;
                    }
                }

                u8* d = &result.pixels[(static_cast<std::size_t>(y) * width + x) * 4];
                if(totalW <= 0.0f)
                    continue;
                for(int c = 0; c < 3; ++c) {
                    const float v = sum[c] / totalW;
                    d[c]          = srgb ? LinearToSrgb(v) : static_cast<u8>(std::lround(std::clamp(v, 0.0f, 1.0f) * 255.0f));
                }
                d[3] = static_cast<u8>(std::lround(std::clamp(sum[3] / totalW, 0.0f, 1.0f) * 255.0f));
            }
        }
        return result;
    }

    //----------------------------------------------------------------------------
    //! 完全に透明な画素の RGB を、近くの不透明な画素の色で塗り広げます。
    //----------------------------------------------------------------------------
    void DilateTransparentEdges(Rgba8Image& image) {
        const std::size_t width  = image.width;
        const std::size_t height = image.height;
        if(width == 0 || height == 0)
            return;

        std::vector<u8> work = image.pixels;

        // バイリニア補間の影響範囲をカバーするため複数回にわたり外側へ色を伝播させる
        constexpr int kIterations = 3;
        for(int iter = 0; iter < kIterations; ++iter) {
            const std::vector<u8> src = work;

            for(std::size_t y = 0; y < height; ++y) {
                for(std::size_t x = 0; x < width; ++x) {
                    if(src[(y * width + x) * 4 + 3] != 0)
                        continue;    // 不透明（もしくは半透明）な画素はそのまま

                    u32 sumR = 0, sumG = 0, sumB = 0, count = 0;
                    for(int dy = -1; dy <= 1; ++dy) {
                        for(int dx = -1; dx <= 1; ++dx) {
                            if(dx == 0 && dy == 0)
                                continue;
                            const std::int64_t nx = static_cast<std::int64_t>(x) + dx;
                            const std::int64_t ny = static_cast<std::int64_t>(y) + dy;
                            if(nx < 0 || ny < 0 || nx >= static_cast<std::int64_t>(width) || ny >= static_cast<std::int64_t>(height))
                                continue;

                            const u8* neighbor = &src[(static_cast<std::size_t>(ny) * width + static_cast<std::size_t>(nx)) * 4];
                            if(neighbor[3] == 0)
                                continue;    // 近くも透明なら色の供給元にしない
                            sumR += neighbor[0];
                            sumG += neighbor[1];
                            sumB += neighbor[2];
                            ++count;
                        }
                    }
                    if(count == 0)
                        continue;    // 今回は塗れなかった（次の回で伝播する）

                    u8* dst = &work[(y * width + x) * 4];
                    dst[0]  = static_cast<u8>(sumR / count);
                    dst[1]  = static_cast<u8>(sumG / count);
                    dst[2]  = static_cast<u8>(sumB / count);
                    // アルファは 0 のまま
                }
            }
        }
        image.pixels = std::move(work);
    }

    //----------------------------------------------------------------------------
    //! 画像がカットアウトを含むかを返します。
    //----------------------------------------------------------------------------
    bool HasCutoutAlpha(const Rgba8Image& image, float ratioThreshold) {
        const std::size_t count = static_cast<std::size_t>(image.width) * image.height;
        if(count == 0)
            return false;

        // アルファが中間値の画素（ふち）だけの画像もあるため、「ほぼ透明」と言える 128 未満を数える
        std::size_t transparent = 0;
        for(std::size_t i = 0; i < count; ++i) {
            if(image.pixels[i * 4 + 3] < 128)
                ++transparent;
        }
        return static_cast<double>(transparent) / static_cast<double>(count) > static_cast<double>(ratioThreshold);
    }

    //----------------------------------------------------------------------------
    //! 画像を加工して KTX で書き出します。
    //----------------------------------------------------------------------------
    bool WriteTextureKtx(const Tsukino::Core::Path& path, const Rgba8Image& image, const ImageEncodeOptions& options) {
        if(image.width == 0 || image.height == 0)
            return false;

        // ブロック圧縮するなら、最上段を4の倍数の大きさにそろえる
        const Rgba8Image* source = &image;
        Rgba8Image        padded;
        if(IsBcFormat(options.compressedFormat) && (image.width % 4 != 0 || image.height % 4 != 0)) {
            padded = ResizeImage(image, RoundUpTo4(image.width), RoundUpTo4(image.height), options.srgb);
            source = &padded;
        }

        ContainerPtr container = BuildContainer({source}, options);
        if(!container) {
            Tsukino::Core::Log::Error("ImageCodec: Failed to process the image: " + path.string());
            return false;
        }
        return WriteKtx(path, *container);
    }

    //----------------------------------------------------------------------------
    //! 6面をキューブマップにまとめ、加工して KTX で書き出します。
    //----------------------------------------------------------------------------
    bool WriteCubemapKtx(const Tsukino::Core::Path& path, const std::array<Rgba8Image, 6>& faces, const ImageEncodeOptions& options) {
        std::vector<const Rgba8Image*> pointers;
        for(const Rgba8Image& face : faces) {
            if(face.width != faces[0].width || face.height != faces[0].height || face.width == 0)
                return false;
            pointers.push_back(&face);
        }

        ContainerPtr container = BuildContainer(pointers, options);
        if(!container) {
            Tsukino::Core::Log::Error("ImageCodec: Failed to process the cubemap: " + path.string());
            return false;
        }
        return WriteKtx(path, *container);
    }

    //----------------------------------------------------------------------------
    //! テクスチャのファイルを読みます。
    //----------------------------------------------------------------------------
    bool ReadTextureFile(const void* data, std::size_t size, TextureFileData& out) {
        if(!data || size == 0)
            return false;

        bx::Error    error;
        ContainerPtr container(bimg::imageParse(Allocator(), data, static_cast<u32>(size), bimg::TextureFormat::Count, &error));
        if(!container || !error.isOk())
            return false;

        TextureFormat format = FromBimg(container->m_format, container->m_srgb);
        if(format == TextureFormat::Unknown) {
            // エンジンが扱わない並び（B5G6R5 など）は RGBA8 に直す。色空間は引き継ぐ
            ContainerPtr converted(bimg::imageConvert(Allocator(), bimg::TextureFormat::RGBA8, *container, true));
            if(!converted)
                return false;
            converted->m_srgb = container->m_srgb;
            container         = std::move(converted);
            format            = container->m_srgb ? TextureFormat::RGBA8_sRGB : TextureFormat::RGBA8;
        }

        out = TextureFileData{};
        out.format = format;

        const bimg::ImageBlockInfo& block = bimg::getBlockInfo(container->m_format);
        std::size_t                 total = 0;
        for(u8 lod = 0; lod < container->m_numMips; ++lod) {
            bimg::ImageMip mip{};
            if(!bimg::imageGetRawData(*container, 0, lod, container->m_data, container->m_size, mip))
                break;

            const u32 blocksX = std::max<u32>(block.minBlockX, (mip.m_width + block.blockWidth - 1) / block.blockWidth);
            TextureFileData::Mip entry;
            entry.offset   = total;
            entry.size     = mip.m_size;
            entry.width    = mip.m_width;
            entry.height   = mip.m_height;
            entry.rowPitch = blocksX * block.blockSize;
            out.mips.push_back(entry);
            total += mip.m_size;
        }
        if(out.mips.empty())
            return false;

        out.pixels.resize(total);
        for(size_t lod = 0; lod < out.mips.size(); ++lod) {
            bimg::ImageMip mip{};
            bimg::imageGetRawData(*container, 0, static_cast<u8>(lod), container->m_data, container->m_size, mip);
            std::memcpy(out.pixels.data() + out.mips[lod].offset, mip.m_data, mip.m_size);
        }
        return true;
    }

    //----------------------------------------------------------------------------
    //! テクスチャのファイルの先頭から、形式と段の数だけを読みます。
    //----------------------------------------------------------------------------
    bool ReadTextureFileInfo(const void* data, std::size_t size, TextureFormat& format, u32& mipCount) {
        bimg::ImageContainer container{};
        bx::Error            error;
        if(!data || size == 0 || !bimg::imageParse(container, data, static_cast<u32>(size), &error))
            return false;
        format   = FromBimg(container.m_format, container.m_srgb);
        mipCount = container.m_numMips;
        return true;
    }

}    // namespace Tsukino::Asset::Detail
