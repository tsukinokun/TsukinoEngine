//----------------------------------------------------------------------------
//! @file   FullscreenPass.hpp
//! @brief  bgfx 版のフルスクリーン三角形の描画（Renderer の内部用）
//! @detail ライティング・トーンマップ・後処理が共用します。DX11 版は頂点シェーダーが SV_VertexID から
//!         三角形を作っていましたが、bgfx 版は3つの頂点を一時的な頂点バッファで渡します。
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/Core/typedef.hpp>

#include <bgfx/bgfx.h>

// 前方宣言
namespace Tsukino::Asset {
    class ShaderAsset;
}

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {

    //------------------------------------------------------------------------
    //! フルスクリーン三角形を描くクラスです（bgfx 版）。
    //------------------------------------------------------------------------
    class FullscreenPass {
    public:
        //! 頂点シェーダーと頂点レイアウトを用意します。
        //! @param  [in] vs フルスクリーン三角形の頂点シェーダー（Tonemap.vs。bgfx の形式）
        //! @return true: 用意できた
        [[nodiscard]]
        bool Initialize(const Tsukino::Asset::ShaderAsset* vs);

        //! 使える状態かを返します。
        //! @return 頂点シェーダーが bgfx の形式なら true
        [[nodiscard]]
        bool IsValid() const noexcept {
            return m_valid;
        }

        //! この頂点シェーダーとピクセルシェーダーを組にしたプログラムを作ります。
        //! @param  [in] ps ピクセルシェーダー（bgfx の形式）
        //! @return プログラム（作れなければ無効なハンドル。呼び出し側が破棄する）
        [[nodiscard]]
        bgfx::ProgramHandle CreateProgram(const Tsukino::Asset::ShaderAsset* ps) const;

        //! フルスクリーン三角形を1枚描きます（テクスチャとユニフォームは呼び出し側が先に設定しておく）。
        //! @param  [in] view    描く先のビュー
        //! @param  [in] program 使うプログラム
        //! @param  [in] state   描画ステート（BGFX_STATE_*）
        void Submit(bgfx::ViewId view, bgfx::ProgramHandle program, u64 state) const;

    private:
        const Tsukino::Asset::ShaderAsset* m_vs    = nullptr;    // 頂点シェーダー（アセットは AssetManager が持つ）
        bool                               m_valid = false;      // 使える状態か
        bgfx::VertexLayout                 m_layout;             // 頂点（位置・UV）
    };

}    // namespace Tsukino::Renderer
