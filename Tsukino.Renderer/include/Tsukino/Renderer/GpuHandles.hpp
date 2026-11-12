//----------------------------------------------------------------------------
//! @file   GpuHandles.hpp
//! @brief  GPU の資源（テクスチャ・バッファ）を指すハンドル
//! @detail 描画 API の型（ID3D11ShaderResourceView など）の代わりに、ゲームと組み込みシステムが
//!         持ち回る小さな番号です。実体は RenderResources が持ち、描くときに番号から引きます。
//!         番号は 16bit で、bgfx のハンドルと1対1に対応させられる大きさにしてあります。
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/Core/typedef.hpp>

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {

    //------------------------------------------------------------------------
    //! GPU の資源を指すハンドルです。
    //! @tparam Tag 資源の種類を区別するための型（テクスチャとバッファを取り違えないため）
    //! @note  既定で作ったものは無効（IsValid() が false）
    //------------------------------------------------------------------------
    template<typename Tag>
    struct GpuHandle {
        static constexpr u16 kInvalidIndex = 0xFFFF;    //!< 無効を表す番号

        u16 index = kInvalidIndex;    // RenderResources の表の中の番号

        //! ハンドルが資源を指しているかを返します。
        //! @return 有効なら true
        [[nodiscard]]
        constexpr bool IsValid() const noexcept {
            return index != kInvalidIndex;
        }

        //! 2つのハンドルが同じ資源を指しているかを比べます。
        //! @param  [in] other 比べる相手
        //! @return 同じ番号なら true
        [[nodiscard]]
        constexpr bool operator==(const GpuHandle& other) const noexcept = default;
    };

    struct TextureHandleTag;    // テクスチャのハンドルを区別する型
    struct BufferHandleTag;     // バッファのハンドルを区別する型

    using TextureHandle = GpuHandle<TextureHandleTag>;    //!< テクスチャを指すハンドル
    using BufferHandle  = GpuHandle<BufferHandleTag>;     //!< バッファ（インスタンス・ゲーム定義の定数）を指すハンドル

}    // namespace Tsukino::Renderer
