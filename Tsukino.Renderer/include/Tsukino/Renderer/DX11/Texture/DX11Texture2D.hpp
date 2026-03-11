//--------------------------------------------------------------
//! @file   DX11Texture2D.hpp
//! @brief  DirectX11用の2Dテクスチャクラスの実装
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
#include <Tsukino/Core/typedef.hpp>   

#include <d3d11.h>
#include <wrl/client.h>
#include <dxgiformat.h>
// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {
    //--------------------------------------------------------------
    //! @class  DX11Texture2D
    //! @brief  DirectX11用の2Dテクスチャクラス
    //--------------------------------------------------------------
    class DX11Texture2D  {
    public:
        //--------------------------------------------------------------
        //! @brief コンストラクタ
        //! @param width  [in] テクスチャの幅（ピクセル単位）
        //! @param height [in] テクスチャの高さ（ピクセル単
        //! @param format [in] テクスチャのフォーマット（DXGI_FORMAT 列挙体）
        //! @param data   [in] テクスチャの初期データ
        //! @param device [in] DirectX11のデバイスオブジェクト（テクスチャの作成に必要）
        //--------------------------------------------------------------
        DX11Texture2D(u32 width, u32 height, DXGI_FORMAT format, const void* data, ID3D11Device* device);

        //--------------------------------------------------------------
        //! @brief バインドを行う関数
        //! @param slot [in] バインドスロット（0～15）
        //--------------------------------------------------------------
        void Bind(ID3D11DeviceContext* context, u32 slot) const;

        //--------------------------------------------------------------
        //! @brief  幅を取得
        //! @return テクスチャの幅（ピクセル単位）
        //--------------------------------------------------------------
        [[nodiscard]]
        u32 GetWidth() const {
            return m_width;
        }

        //--------------------------------------------------------------
        //! @brief  高さを取得
        //! @return テクスチャの高さ（ピクセル単位）
        //--------------------------------------------------------------
        [[nodiscard]]
        u32 GetHeight() const {
            return m_height;
        }

    private:
        u32 m_width  = 0;    // テクスチャの幅（ピクセル単位）
        u32 m_height = 0;    // テクスチャの高さ（ピクセル単位）

        Microsoft::WRL::ComPtr<ID3D11Texture2D>          m_texture;    // DirectX11の2Dテクスチャオブジェクト
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_srv;        // DirectX11のシェーダーリソースビュー（テクスチャをシェーダーで使用するためのビュー）
    };

}    // namespace Tsukino::Renderer
