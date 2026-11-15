//----------------------------------------------------------------------------
//! @file   EfkGraphicsDevice.cpp
//! @brief  Effekseer の描画デバイスの bgfx 版の実装
//----------------------------------------------------------------------------
#include "EfkGraphicsDevice.hpp"

#include "../MipChain.hpp"

#include <Tsukino/Renderer/Bgfx/GraphicsContext.hpp>

#include <Tsukino/Core/Log.hpp>

#include <algorithm>
#include <cstring>
#include <string>

// 名前空間 : Tsukino::Renderer::EffekseerBgfx
namespace Tsukino::Renderer::EffekseerBgfx {

    namespace {
        using Effekseer::Backend::TextureFormatType;

        //! Effekseer のテクスチャの形式に当たる bgfx の形式です。
        struct BgfxTextureFormat {
            bgfx::TextureFormat::Enum format   = bgfx::TextureFormat::Unknown;    // 形式
            bool                      srgb     = false;                           // sRGB として読むか
            u32                       channels = 0;                               // 8bit の成分の数（圧縮や浮動小数の形式は 0）
        };

        //! Effekseer のテクスチャの形式を bgfx の形式へ直します。
        //! @param  [in] format Effekseer の形式
        //! @return bgfx の形式（直せなければ Unknown）
        BgfxTextureFormat ToBgfxTextureFormat(TextureFormatType format) {
            switch(format) {
                case TextureFormatType::R8G8B8A8_UNORM:      return {bgfx::TextureFormat::RGBA8, false, 4};
                case TextureFormatType::R8G8B8A8_UNORM_SRGB: return {bgfx::TextureFormat::RGBA8, true, 4};
                case TextureFormatType::B8G8R8A8_UNORM:      return {bgfx::TextureFormat::BGRA8, false, 4};
                case TextureFormatType::B8G8R8A8_UNORM_SRGB: return {bgfx::TextureFormat::BGRA8, true, 4};
                case TextureFormatType::R8_UNORM:            return {bgfx::TextureFormat::R8, false, 1};
                case TextureFormatType::R16_FLOAT:           return {bgfx::TextureFormat::R16F};
                case TextureFormatType::R32_FLOAT:           return {bgfx::TextureFormat::R32F};
                case TextureFormatType::R16G16_FLOAT:        return {bgfx::TextureFormat::RG16F};
                case TextureFormatType::R16G16B16A16_FLOAT:  return {bgfx::TextureFormat::RGBA16F};
                case TextureFormatType::R32G32B32A32_FLOAT:  return {bgfx::TextureFormat::RGBA32F};
                case TextureFormatType::RG11B10_UFLOAT:      return {bgfx::TextureFormat::RG11B10F};
                case TextureFormatType::BC1:                 return {bgfx::TextureFormat::BC1};
                case TextureFormatType::BC1_SRGB:            return {bgfx::TextureFormat::BC1, true};
                case TextureFormatType::BC2:                 return {bgfx::TextureFormat::BC2};
                case TextureFormatType::BC2_SRGB:            return {bgfx::TextureFormat::BC2, true};
                case TextureFormatType::BC3:                 return {bgfx::TextureFormat::BC3};
                case TextureFormatType::BC3_SRGB:            return {bgfx::TextureFormat::BC3, true};
                case TextureFormatType::BC7:                 return {bgfx::TextureFormat::BC7};
                case TextureFormatType::BC7_SRGB:            return {bgfx::TextureFormat::BC7, true};
                default:                                     return {};
            }
        }

        //! 1×1 までの段の数を返します。
        //! @param  [in] width  幅
        //! @param  [in] height 高さ
        //! @return 段の数
        u32 FullMipCount(u32 width, u32 height) {
            u32 count = 1;
            for(u32 size = std::max(width, height); size > 1; size /= 2)
                ++count;
            return count;
        }

        //! テクスチャの中身の大きさ（バイト）を返します。
        //! @param  [in] width   幅
        //! @param  [in] height  高さ
        //! @param  [in] hasMips 1×1 までの段を持つか
        //! @param  [in] format  形式
        //! @return バイト数
        u32 TextureStorageSize(u32 width, u32 height, bool hasMips, bgfx::TextureFormat::Enum format) {
            bgfx::TextureInfo info;
            bgfx::calcTextureSize(info, static_cast<u16>(width), static_cast<u16>(height), 1, false, hasMips, 1, format);
            return info.storageSize;
        }
    }    // namespace

    //------------------------------------------------------------------------
    //! 要素の並びから頂点形式を作ります。
    //------------------------------------------------------------------------
    VertexLayout::VertexLayout(const Effekseer::Backend::VertexLayoutElement* elements, int32_t count) {
        using Effekseer::Backend::VertexLayoutFormat;

        m_valid = true;
        m_layout.begin();
        for(int32_t i = 0; i < count; ++i) {
            const Effekseer::Backend::VertexLayoutElement& element = elements[i];

            //----------------------------------------------------------------
            // セマンティクス → bgfx の頂点の入力
            //----------------------------------------------------------------
            const std::string semantic(element.SemanticName.c_str());
            bgfx::Attrib::Enum attrib = bgfx::Attrib::Count;
            if(semantic == "POSITION" && element.SemanticIndex == 0)
                attrib = bgfx::Attrib::Position;
            else if(semantic == "NORMAL" && element.SemanticIndex >= 0 && element.SemanticIndex < 4)
                attrib = static_cast<bgfx::Attrib::Enum>(bgfx::Attrib::Color0 + element.SemanticIndex);
            else if(semantic == "TEXCOORD" && element.SemanticIndex >= 0 && element.SemanticIndex < 8)
                attrib = static_cast<bgfx::Attrib::Enum>(bgfx::Attrib::TexCoord0 + element.SemanticIndex);

            //----------------------------------------------------------------
            // 形式 → 成分の数と型
            //----------------------------------------------------------------
            u8                     num        = 0;
            bgfx::AttribType::Enum type       = bgfx::AttribType::Float;
            bool                   normalized = false;
            bool                   asInt      = false;
            switch(element.Format) {
                case VertexLayoutFormat::R32_FLOAT:          num = 1; break;
                case VertexLayoutFormat::R32G32_FLOAT:       num = 2; break;
                case VertexLayoutFormat::R32G32B32_FLOAT:    num = 3; break;
                case VertexLayoutFormat::R32G32B32A32_FLOAT: num = 4; break;
                case VertexLayoutFormat::R8G8B8A8_UNORM:     num = 4; type = bgfx::AttribType::Uint8; normalized = true; break;
                case VertexLayoutFormat::R8G8B8A8_UINT:      num = 4; type = bgfx::AttribType::Uint8; asInt = true; break;
            }

            if(attrib == bgfx::Attrib::Count || num == 0) {
                Tsukino::Core::Log::Error("Effekseer (bgfx): Unsupported vertex element " + semantic + std::to_string(element.SemanticIndex));
                m_valid = false;
                continue;
            }
            m_layout.add(attrib, num, type, normalized, asInt);
        }
        m_layout.end();
    }

    //------------------------------------------------------------------------
    //! 頂点バッファを作ります。
    //------------------------------------------------------------------------
    VertexBuffer::VertexBuffer(int32_t size, const void* initialData, bool isDynamic)
        : m_data(static_cast<size_t>(std::max(size, 0)), 0)
        , m_dynamic(isDynamic) {
        if(initialData && !m_data.empty())
            std::memcpy(m_data.data(), initialData, m_data.size());
    }

    //------------------------------------------------------------------------
    //! bgfx のバッファを破棄します。
    //------------------------------------------------------------------------
    VertexBuffer::~VertexBuffer() {
        DestroyHandle();
    }

    //------------------------------------------------------------------------
    //! 中身を書き換えます。
    //------------------------------------------------------------------------
    void VertexBuffer::UpdateData(const void* src, int32_t size, int32_t offset) {
        if(!src || size <= 0 || offset < 0 || static_cast<size_t>(offset) + static_cast<size_t>(size) > m_data.size())
            return;
        std::memcpy(m_data.data() + offset, src, static_cast<size_t>(size));

        // 静的なバッファは中身が古くなるので、次に描くときに作り直す
        DestroyHandle();
    }

    //------------------------------------------------------------------------
    //! 次の描画の頂点として bgfx へ渡します。
    //------------------------------------------------------------------------
    bool VertexBuffer::Bind(u32 firstVertex, u32 vertexCount, const bgfx::VertexLayout& layout) {
        const u32 stride = layout.getStride();
        if(stride == 0 || vertexCount == 0)
            return false;

        const size_t begin = static_cast<size_t>(firstVertex) * stride;
        const size_t bytes = static_cast<size_t>(vertexCount) * stride;
        if(begin + bytes > m_data.size())
            return false;

        //--------------------------------------------------------------------
        // 毎フレーム書き換わる頂点は、使う範囲だけを一時的な頂点バッファへ写す
        //--------------------------------------------------------------------
        if(m_dynamic) {
            if(bgfx::getAvailTransientVertexBuffer(vertexCount, layout) < vertexCount)
                return false;
            bgfx::TransientVertexBuffer transient;
            bgfx::allocTransientVertexBuffer(&transient, vertexCount, layout);
            std::memcpy(transient.data, m_data.data() + begin, bytes);
            bgfx::setVertexBuffer(0, &transient);
            return true;
        }

        //--------------------------------------------------------------------
        // 書き換わらない頂点（モデル）は、最初に描くときに静的なバッファを作る
        //--------------------------------------------------------------------
        if(!bgfx::isValid(m_handle) || m_handleLayoutHash != layout.m_hash) {
            DestroyHandle();
            const u32 totalVertices = static_cast<u32>(m_data.size() / stride);
            if(totalVertices == 0)
                return false;
            m_handle           = bgfx::createVertexBuffer(bgfx::copy(m_data.data(), totalVertices * stride), layout);
            m_handleLayoutHash = layout.m_hash;
            if(!bgfx::isValid(m_handle))
                return false;
        }
        bgfx::setVertexBuffer(0, m_handle, firstVertex, vertexCount);
        return true;
    }

    //------------------------------------------------------------------------
    //! 静的なバッファを破棄します。
    //------------------------------------------------------------------------
    void VertexBuffer::DestroyHandle() {
        if(bgfx::isValid(m_handle) && GraphicsContext::IsBgfxAlive())
            bgfx::destroy(m_handle);
        m_handle = BGFX_INVALID_HANDLE;
    }

    //------------------------------------------------------------------------
    //! インデックスバッファを作ります。
    //------------------------------------------------------------------------
    IndexBuffer::IndexBuffer(int32_t elementCount, const void* initialData, Effekseer::Backend::IndexBufferStrideType stride) {
        strideType_   = stride;
        elementCount_ = std::max(elementCount, 0);

        const size_t indexSize = stride == Effekseer::Backend::IndexBufferStrideType::Stride4 ? 4 : 2;
        m_data.assign(static_cast<size_t>(elementCount_) * indexSize, 0);
        if(initialData && !m_data.empty())
            std::memcpy(m_data.data(), initialData, m_data.size());
    }

    //------------------------------------------------------------------------
    //! bgfx のバッファを破棄します。
    //------------------------------------------------------------------------
    IndexBuffer::~IndexBuffer() {
        DestroyHandle();
    }

    //------------------------------------------------------------------------
    //! 中身を書き換えます。
    //------------------------------------------------------------------------
    void IndexBuffer::UpdateData(const void* src, int32_t size, int32_t offset) {
        if(!src || size <= 0 || offset < 0 || static_cast<size_t>(offset) + static_cast<size_t>(size) > m_data.size())
            return;
        std::memcpy(m_data.data() + offset, src, static_cast<size_t>(size));
        DestroyHandle();
    }

    //------------------------------------------------------------------------
    //! 次の描画のインデックスとして bgfx へ渡します。
    //------------------------------------------------------------------------
    bool IndexBuffer::Bind(u32 indexCount) {
        if(indexCount == 0 || indexCount > static_cast<u32>(elementCount_))
            return false;

        if(!bgfx::isValid(m_handle)) {
            const u16 flags = strideType_ == Effekseer::Backend::IndexBufferStrideType::Stride4 ? BGFX_BUFFER_INDEX32 : BGFX_BUFFER_NONE;
            m_handle        = bgfx::createIndexBuffer(bgfx::copy(m_data.data(), static_cast<u32>(m_data.size())), flags);
            if(!bgfx::isValid(m_handle))
                return false;
        }
        bgfx::setIndexBuffer(m_handle, 0, indexCount);
        return true;
    }

    //------------------------------------------------------------------------
    //! bgfx のバッファを破棄します。
    //------------------------------------------------------------------------
    void IndexBuffer::DestroyHandle() {
        if(bgfx::isValid(m_handle) && GraphicsContext::IsBgfxAlive())
            bgfx::destroy(m_handle);
        m_handle = BGFX_INVALID_HANDLE;
    }

    //------------------------------------------------------------------------
    //! bgfx のテクスチャを持たせて作ります。
    //------------------------------------------------------------------------
    Texture::Texture(bgfx::TextureHandle handle, const Effekseer::Backend::TextureParameter& param, bool owned)
        : m_handle(handle)
        , m_owned(owned) {
        param_ = param;
    }

    //------------------------------------------------------------------------
    //! 持っているテクスチャを破棄します。
    //------------------------------------------------------------------------
    Texture::~Texture() {
        if(m_owned && bgfx::isValid(m_handle) && GraphicsContext::IsBgfxAlive())
            bgfx::destroy(m_handle);
    }

    //------------------------------------------------------------------------
    //! 借りているテクスチャを差し替えます。
    //------------------------------------------------------------------------
    void Texture::SetBorrowedHandle(bgfx::TextureHandle handle, u32 width, u32 height) {
        if(m_owned)
            return;
        m_handle     = handle;
        param_.Size = {static_cast<int32_t>(width), static_cast<int32_t>(height), 1};
    }

    //------------------------------------------------------------------------
    //! 頂点バッファを作ります。
    //------------------------------------------------------------------------
    Effekseer::Backend::VertexBufferRef GraphicsDevice::CreateVertexBuffer(int32_t size, const void* initialData, bool isDynamic) {
        return Effekseer::MakeRefPtr<VertexBuffer>(size, initialData, isDynamic);
    }

    //------------------------------------------------------------------------
    //! インデックスバッファを作ります。
    //------------------------------------------------------------------------
    Effekseer::Backend::IndexBufferRef GraphicsDevice::CreateIndexBuffer(int32_t elementCount, const void* initialData,
                                                                         Effekseer::Backend::IndexBufferStrideType stride) {
        return Effekseer::MakeRefPtr<IndexBuffer>(elementCount, initialData, stride);
    }

    //------------------------------------------------------------------------
    //! 頂点バッファの中身を書き換えます。
    //------------------------------------------------------------------------
    bool GraphicsDevice::UpdateVertexBuffer(Effekseer::Backend::VertexBufferRef& buffer, int32_t size, int32_t offset, const void* data) {
        if(buffer == nullptr)
            return false;
        buffer->UpdateData(data, size, offset);
        return true;
    }

    //------------------------------------------------------------------------
    //! インデックスバッファの中身を書き換えます。
    //------------------------------------------------------------------------
    bool GraphicsDevice::UpdateIndexBuffer(Effekseer::Backend::IndexBufferRef& buffer, int32_t size, int32_t offset, const void* data) {
        if(buffer == nullptr)
            return false;
        buffer->UpdateData(data, size, offset);
        return true;
    }

    //------------------------------------------------------------------------
    //! 頂点形式を作ります。
    //------------------------------------------------------------------------
    Effekseer::Backend::VertexLayoutRef GraphicsDevice::CreateVertexLayout(const Effekseer::Backend::VertexLayoutElement* elements,
                                                                           int32_t                                        elementCount) {
        auto layout = Effekseer::MakeRefPtr<VertexLayout>(elements, elementCount);
        if(!layout->IsValid())
            return nullptr;
        return layout;
    }

    //------------------------------------------------------------------------
    //! テクスチャを作ります。
    //------------------------------------------------------------------------
    Effekseer::Backend::TextureRef GraphicsDevice::CreateTexture(const Effekseer::Backend::TextureParameter& param,
                                                                 const Effekseer::CustomVector<uint8_t>&    initialData) {
        //--------------------------------------------------------------------
        // 作れるのは 2D の画像だけ（配列・3D は GPU パーティクルが使うもので、エンジンは使わない）
        //--------------------------------------------------------------------
        if(param.Dimension != 2 || (param.Usage & Effekseer::Backend::TextureUsageType::Array) != Effekseer::Backend::TextureUsageType::None) {
            Tsukino::Core::Log::Warn("Effekseer (bgfx): Only 2D textures are supported; a texture was not created.");
            return nullptr;
        }

        const BgfxTextureFormat format = ToBgfxTextureFormat(param.Format);
        if(format.format == bgfx::TextureFormat::Unknown) {
            Tsukino::Core::Log::Warn("Effekseer (bgfx): Unsupported texture format " + std::to_string(static_cast<int>(param.Format)) + ".");
            return nullptr;
        }

        const u32 width  = static_cast<u32>(std::max(param.Size[0], 1));
        const u32 height = static_cast<u32>(std::max(param.Size[1], 1));
        const u64 flags  = format.srgb ? BGFX_TEXTURE_SRGB : BGFX_TEXTURE_NONE;

        //--------------------------------------------------------------------
        // 中身の段。bgfx は「1段だけ」か「1×1 までの全段」しか受け付けない。
        //   段の数が 0（作ってほしい）: 8bit の形式なら CPU で全段を作る。作れない形式は1段のまま
        //   全段がそろっている: そのまま渡す
        //   それ以外（途中までしか無い）: いちばん上の段だけを使う
        //--------------------------------------------------------------------
        const u32 baseSize = TextureStorageSize(width, height, false, format.format);
        const u32 fullSize = TextureStorageSize(width, height, true, format.format);

        const bgfx::Memory* memory  = nullptr;
        bool                hasMips = false;
        if(!initialData.empty()) {
            if(param.MipLevelCount < 1 && format.channels > 0 && initialData.size() >= baseSize) {
                const std::vector<u8> chain = BuildMipChain(format.channels, format.srgb, width, height, initialData.data(), width * format.channels);
                memory                      = bgfx::copy(chain.data(), static_cast<u32>(chain.size()));
                hasMips                     = true;
            } else if(static_cast<u32>(param.MipLevelCount) == FullMipCount(width, height) && param.MipLevelCount > 1
                      && initialData.size() >= fullSize) {
                memory  = bgfx::copy(initialData.data(), fullSize);
                hasMips = true;
            } else if(initialData.size() >= baseSize) {
                memory = bgfx::copy(initialData.data(), baseSize);
            } else {
                Tsukino::Core::Log::Warn("Effekseer (bgfx): The texture data is smaller than the texture; a texture was not created.");
                return nullptr;
            }
        }

        const bgfx::TextureHandle handle =
            bgfx::createTexture2D(static_cast<u16>(width), static_cast<u16>(height), hasMips, 1, format.format, flags, memory);
        if(!bgfx::isValid(handle))
            return nullptr;
        return Effekseer::MakeRefPtr<Texture>(handle, param, true);
    }

    //------------------------------------------------------------------------
    //! Renderer が持つテクスチャを借りて見せるテクスチャを作ります。
    //------------------------------------------------------------------------
    Effekseer::RefPtr<Texture> GraphicsDevice::CreateBorrowedTexture(bgfx::TextureHandle handle, u32 width, u32 height) {
        Effekseer::Backend::TextureParameter param;
        param.Size = {static_cast<int32_t>(width), static_cast<int32_t>(height), 1};
        return Effekseer::MakeRefPtr<Texture>(handle, param, false);
    }

}    // namespace Tsukino::Renderer::EffekseerBgfx
