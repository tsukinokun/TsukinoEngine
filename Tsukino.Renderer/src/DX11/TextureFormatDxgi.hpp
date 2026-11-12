//----------------------------------------------------------------------------
//! @file   TextureFormatDxgi.hpp
//! @brief  TextureFormat から DXGI_FORMAT への変換（Renderer の DX11 版の内部用）
//! @detail 値の対応は Engine のインポーターとローダーが使う変換
//!         （Tsukino.Engine/src/Asset/Texture/TextureFormatDxgi.hpp）と揃えています。
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/GraphicsCommon/Texture/TextureFormat.hpp>

#include <dxgiformat.h>

// 名前空間 : Tsukino::Renderer::Detail
namespace Tsukino::Renderer::Detail {

    //! TextureFormat を DXGI_FORMAT に変換します。
    //! @param  [in] format エンジンのフォーマット
    //! @return 対応する DXGI_FORMAT（Unknown は DXGI_FORMAT_UNKNOWN）
    inline DXGI_FORMAT ToDxgiFormat(Tsukino::GraphicsCommon::TextureFormat format) {
        using Tsukino::GraphicsCommon::TextureFormat;
        switch(format) {
            case TextureFormat::R8:         return DXGI_FORMAT_R8_UNORM;
            case TextureFormat::RG8:        return DXGI_FORMAT_R8G8_UNORM;
            case TextureFormat::RGBA8:      return DXGI_FORMAT_R8G8B8A8_UNORM;
            case TextureFormat::RGBA8_sRGB: return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
            case TextureFormat::BGRA8:      return DXGI_FORMAT_B8G8R8A8_UNORM;
            case TextureFormat::BGRA8_sRGB: return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
            case TextureFormat::R16:        return DXGI_FORMAT_R16_UNORM;
            case TextureFormat::RGBA16:     return DXGI_FORMAT_R16G16B16A16_UNORM;
            case TextureFormat::R16F:       return DXGI_FORMAT_R16_FLOAT;
            case TextureFormat::RGBA16F:    return DXGI_FORMAT_R16G16B16A16_FLOAT;
            case TextureFormat::R32F:       return DXGI_FORMAT_R32_FLOAT;
            case TextureFormat::RGBA32F:    return DXGI_FORMAT_R32G32B32A32_FLOAT;
            case TextureFormat::RGB10A2:    return DXGI_FORMAT_R10G10B10A2_UNORM;
            case TextureFormat::BC1:        return DXGI_FORMAT_BC1_UNORM;
            case TextureFormat::BC1_sRGB:   return DXGI_FORMAT_BC1_UNORM_SRGB;
            case TextureFormat::BC3:        return DXGI_FORMAT_BC3_UNORM;
            case TextureFormat::BC3_sRGB:   return DXGI_FORMAT_BC3_UNORM_SRGB;
            case TextureFormat::BC4:        return DXGI_FORMAT_BC4_UNORM;
            case TextureFormat::BC5:        return DXGI_FORMAT_BC5_UNORM;
            case TextureFormat::BC7:        return DXGI_FORMAT_BC7_UNORM;
            case TextureFormat::BC7_sRGB:   return DXGI_FORMAT_BC7_UNORM_SRGB;
            default:                        return DXGI_FORMAT_UNKNOWN;
        }
    }

    //! 1ピクセルあたりのバイト数を返します。
    //! @param  [in] format エンジンのフォーマット
    //! @return バイト数（ブロック圧縮と Unknown は 0）
    inline u32 BytesPerPixel(Tsukino::GraphicsCommon::TextureFormat format) {
        using Tsukino::GraphicsCommon::TextureFormat;
        switch(format) {
            case TextureFormat::R8:         return 1;
            case TextureFormat::RG8:        return 2;
            case TextureFormat::RGBA8:
            case TextureFormat::RGBA8_sRGB:
            case TextureFormat::BGRA8:
            case TextureFormat::BGRA8_sRGB: return 4;
            case TextureFormat::R16:
            case TextureFormat::R16F:       return 2;
            case TextureFormat::RGBA16:
            case TextureFormat::RGBA16F:    return 8;
            case TextureFormat::R32F:       return 4;
            case TextureFormat::RGBA32F:    return 16;
            case TextureFormat::RGB10A2:    return 4;
            default:                        return 0;
        }
    }

}    // namespace Tsukino::Renderer::Detail
