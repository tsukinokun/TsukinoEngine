//--------------------------------------------------------------
//! @file   TextureAsset.hpp
//! @brief  テクスチャアセットクラスの宣言
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
#include <Tsukino/Engine/Asset/IAsset.hpp>

#include <dxgiformat.h>

#include <vector>
#include <cstdint>
// 名前空間 Tsukino::Asset
namespace Tsukino::Asset {
    //--------------------------------------------------------------
    //! @class  TextureAsset
    //! @brief  テクスチャアセット
    //--------------------------------------------------------------
    class TextureAsset : public IAsset {
    public:
        //--------------------------------------------------------------
        //! @brief ハンドルを取得する関数
        //! @return アセットのハンドル
        //--------------------------------------------------------------
        AssetHandle GetHandle() const override { return m_handle; }

        //--------------------------------------------------------------
        //! @brief アセットの種類を取得する関数
        //! @return アセットの種類
        //--------------------------------------------------------------
        AssetType GetType() const override { return AssetType::Texture; }

        //--------------------------------------------------------------
        //! @brief  ローダー側から設定されるハンドル
        //! @param handle [in] 設定するハンドル
        //--------------------------------------------------------------
        void SetHandle(const AssetHandle& h) { m_handle = h; }

        //--------------------------------------------------------------
        //! ピクセルデータを手放します。
        //! @note  GPU へ送ったあとは読まないので、Renderer が転送の直後に呼ぶ。
        //!        CPU 側に残し続けると、GPU と同じ量のメモリを二重に使うことになる
        //--------------------------------------------------------------
        void ReleasePixels() {
            std::vector<uint8_t>().swap(pixels);
        }

        uint32_t             width  = 0;                      // テクスチャの幅
        uint32_t             height = 0;                      // テクスチャの高さ
        DXGI_FORMAT          format = DXGI_FORMAT_UNKNOWN;    // テクスチャのフォーマット
        std::vector<uint8_t> pixels;                          // テクスチャのピクセルデータ。最上段から順に全段を詰めて持つ

        //! ミップマップ1段ぶんの、pixels内の位置と1行のバイト数
        struct MipLevel {
            size_t   offset   = 0;    // pixelsの先頭からのバイト位置
            uint32_t rowPitch = 0;    // 1行のバイト数（ブロック圧縮はブロック1行ぶん）
        };

        //! ファイルが持っていたミップマップの段。最上段だけのときは1要素。
        //! 2段以上あるときはそのままGPUへ送り、generateMipsは使わない
        std::vector<MipLevel> mips;

        //! GPUへ送るときにミップマップ（1/2ずつ縮めた絵の段）を作るか。
        //! pixelsは最上段だけを持つ形のまま、縮小はRendererがGPUで作る。
        //! GPUが縮められるフォーマット（非圧縮のRGBA8系など）のときだけ効く（ブロック圧縮は1段のまま）。
        //! 細かい線や模様を、縮めて描いたときにちらつかせないために使う（TextureLoaderは1段だけのDDSで立てる）
        bool generateMips = false;

    private:
        AssetHandle m_handle = AssetHandle::Invalid();    // アセットのハンドル
    };

}    // namespace Tsukino::Asset
