//----------------------------------------------------------------------------
//! @file   TextureFormatDxgi.hpp
//! @brief  TextureFormat と DXGI_FORMAT の相互変換（Engine の内部用）
//! @detail DirectXTex で DDS を読み書きするインポーターとローダーが使います。
//!         Renderer の DX11 版にも同じ向きの変換があり、値の対応はそちらと揃えています。
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/GraphicsCommon/Texture/TextureFormat.hpp>

#include <dxgiformat.h>

// 名前空間 : Tsukino::Asset::Detail
namespace Tsukino::Asset::Detail {

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

    //! DXGI_FORMAT を TextureFormat に変換します。
    //! @param  [in] format DDS が持っていたフォーマット
    //! @return 対応する TextureFormat（エンジンが扱わないフォーマットは Unknown）
    inline Tsukino::GraphicsCommon::TextureFormat FromDxgiFormat(DXGI_FORMAT format) {
        using Tsukino::GraphicsCommon::TextureFormat;
        switch(format) {
            case DXGI_FORMAT_R8_UNORM:             return TextureFormat::R8;
            case DXGI_FORMAT_R8G8_UNORM:           return TextureFormat::RG8;
            case DXGI_FORMAT_R8G8B8A8_UNORM:       return TextureFormat::RGBA8;
            case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:  return TextureFormat::RGBA8_sRGB;
            case DXGI_FORMAT_B8G8R8A8_UNORM:       return TextureFormat::BGRA8;
            case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:  return TextureFormat::BGRA8_sRGB;
            case DXGI_FORMAT_R16_UNORM:            return TextureFormat::R16;
            case DXGI_FORMAT_R16G16B16A16_UNORM:   return TextureFormat::RGBA16;
            case DXGI_FORMAT_R16_FLOAT:            return TextureFormat::R16F;
            case DXGI_FORMAT_R16G16B16A16_FLOAT:   return TextureFormat::RGBA16F;
            case DXGI_FORMAT_R32_FLOAT:            return TextureFormat::R32F;
            case DXGI_FORMAT_R32G32B32A32_FLOAT:   return TextureFormat::RGBA32F;
            case DXGI_FORMAT_R10G10B10A2_UNORM:    return TextureFormat::RGB10A2;
            case DXGI_FORMAT_BC1_UNORM:            return TextureFormat::BC1;
            case DXGI_FORMAT_BC1_UNORM_SRGB:       return TextureFormat::BC1_sRGB;
            case DXGI_FORMAT_BC3_UNORM:            return TextureFormat::BC3;
            case DXGI_FORMAT_BC3_UNORM_SRGB:       return TextureFormat::BC3_sRGB;
            case DXGI_FORMAT_BC4_UNORM:            return TextureFormat::BC4;
            case DXGI_FORMAT_BC5_UNORM:            return TextureFormat::BC5;
            case DXGI_FORMAT_BC7_UNORM:            return TextureFormat::BC7;
            case DXGI_FORMAT_BC7_UNORM_SRGB:       return TextureFormat::BC7_sRGB;
            default:                               return TextureFormat::Unknown;
        }
    }

}    // namespace Tsukino::Asset::Detail
