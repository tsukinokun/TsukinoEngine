//----------------------------------------------------------------------------
//! @file   ShaderPrograms.hpp
//! @brief  bgfx のシェーダーとプログラムを作る関数（Renderer の内部用）
//! @detail シェーダーアセット（BuiltInShaders が読んだ .sc.bin）からプログラムを作る関数と、
//!         RendererShaderSet に無い組み込みのシェーダーをファイルから直接読む関数です。
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/Core/typedef.hpp>

#include <bgfx/bgfx.h>

#include <vector>

// 前方宣言
namespace Tsukino::Asset {
    class ShaderAsset;
}

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {

    //! 変換済みのシェーダーの形式のフォルダ（BuiltInShaders と揃える。今は Windows の s_5_0 だけ）
    inline constexpr const char* kBgfxShaderProfileDir = "dxbc";

    //! シェーダーのバイナリが bgfx の形式（shaderc の出力）かを確かめます。
    //! @param  [in] binary シェーダーのバイナリ
    //! @return bgfx の形式なら true（頭が "VSH" / "FSH" / "CSH"）
    [[nodiscard]]
    bool IsBgfxShaderBinary(const std::vector<u8>& binary);

    //! シェーダーアセット2本（bgfx の形式）から、プログラムを作ります。
    //! @param  [in] vs 頂点シェーダー
    //! @param  [in] ps ピクセルシェーダー
    //! @return プログラム（どちらかが無いか bgfx の形式でなければ無効なハンドル）
    [[nodiscard]]
    bgfx::ProgramHandle CreateBgfxProgram(const Tsukino::Asset::ShaderAsset* vs, const Tsukino::Asset::ShaderAsset* ps);

    //! 組み込みの変換済みシェーダーを2本ファイルから読み込んで、プログラムを作ります。
    //! @param  [in] vsName 頂点シェーダーの名前（"SpriteBatch.vs" のように拡張子を除いた名前）
    //! @param  [in] psName ピクセルシェーダーの名前
    //! @return プログラム（読めなければ無効なハンドル）
    [[nodiscard]]
    bgfx::ProgramHandle LoadBuiltInBgfxProgram(const char* vsName, const char* psName);

}    // namespace Tsukino::Renderer
