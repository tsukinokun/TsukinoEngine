//----------------------------------------------------------------------------
//! @file   MipChain.hpp
//! @brief  1段だけの画像から、1×1 までのミップを CPU で作る関数の宣言
//! @detail Tsukino.Renderer の内部専用ヘッダです。
//!         bgfx には普通のテクスチャのミップを GPU で作る機能が無いので、送る前に CPU で作ります。
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/Core/typedef.hpp>

#include <vector>

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {
    //------------------------------------------------------------------------
    //! 1画素 8bit × channels の画像から、1×1 までのミップを作ります。
    //! @param  [in] channels 1画素の成分の数（1〜4）
    //! @param  [in] srgb     true なら色の成分（4成分のときは最後の1つを除く）を線形に直してから平均する
    //! @param  [in] width    いちばん上の段の幅
    //! @param  [in] height   いちばん上の段の高さ
    //! @param  [in] top      いちばん上の段の画素
    //! @param  [in] rowPitch いちばん上の段の1行のバイト数
    //! @return いちばん上の段から 1×1 までを、行のすき間なく順に詰めた画素
    //! @note   縦横が奇数の段は、端の画素を繰り返して 2×2 の平均を取る
    //------------------------------------------------------------------------
    [[nodiscard]]
    std::vector<u8> BuildMipChain(u32 channels, bool srgb, u32 width, u32 height, const u8* top, u32 rowPitch);
}    // namespace Tsukino::Renderer
