//--------------------------------------------------------------
//! @file   DX11TextureCube.hpp
//! @brief  DirectX11用のキューブマップレンダーターゲットクラスの宣言
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
// windows.h の min/max マクロを避けるため、DX11 / Effekseer より先に通す
#include <Tsukino/Core/WindowsLean.hpp>
#include <Tsukino/Core/typedef.hpp>

#include <d3d11.h>
#include <wrl/client.h>
#include <dxgiformat.h>
#include <array>
#include <vector>
// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {
    //--------------------------------------------------------------
    //! @class  DX11TextureCube
    //! @brief  6面×mipへ個別に描画でき、まとめてシェーダーから読めるキューブマップ
    //! @note   DX11Texture2Dは単一mip・単一スライスの読み取り専用テクスチャのため、
    //!         「面ごと・mipごとにRTVを持つ」キューブマップの概念を表現できない。
    //!         IBL（スカイのキャプチャ・irradiance畳み込み・プレフィルタ済み
    //!         スペキュラミップチェーン）専用に、別型として用意する。
    //--------------------------------------------------------------
    class DX11TextureCube {
    public:
        //--------------------------------------------------------------
        //! @brief コンストラクタ
        //! @param baseSize [in] mip0の一辺のピクセル数（正方形）
        //! @param mipLevels [in] ミップレベル数（1以上。プレフィルタ済みスペキュラ用は複数）
        //! @param format   [in] テクスチャフォーマット（HDR前提で R16G16B16A16_FLOAT を想定）
        //! @param device   [in] DirectX11のデバイスオブジェクト
        //--------------------------------------------------------------
        DX11TextureCube(u32 baseSize, u32 mipLevels, DXGI_FORMAT format, ID3D11Device* device);

        //--------------------------------------------------------------
        //! @brief  シェーダー読み取り用SRV（TextureCubeとして1枚にまとまったもの）を取得
        //--------------------------------------------------------------
        [[nodiscard]]
        ID3D11ShaderResourceView* GetSRV() const {
            return m_srv.Get();
        }

        //--------------------------------------------------------------
        //! @brief  指定した面・mipへ書き込むためのRTVを取得
        //! @param  face [in] キューブの面（0〜5。D3D11_TEXTURECUBE_FACEの並びと同じ：+X,-X,+Y,-Y,+Z,-Z）
        //! @param  mip  [in] ミップレベル（0〜GetMipLevels()-1）
        //! @return ID3D11RenderTargetViewへのポインタ（範囲外はnullptr）
        //--------------------------------------------------------------
        [[nodiscard]]
        ID3D11RenderTargetView* GetFaceRTV(u32 face, u32 mip) const {
            if(face >= 6 || mip >= m_mipLevels)
                return nullptr;
            return m_faceRTV[static_cast<size_t>(mip) * 6 + face].Get();
        }

        //--------------------------------------------------------------
        //! @brief  mip0の一辺のピクセル数を取得
        //--------------------------------------------------------------
        [[nodiscard]]
        u32 GetBaseSize() const {
            return m_baseSize;
        }

        //--------------------------------------------------------------
        //! @brief  ミップレベル数を取得
        //--------------------------------------------------------------
        [[nodiscard]]
        u32 GetMipLevels() const {
            return m_mipLevels;
        }

        //--------------------------------------------------------------
        //! @brief  指定したmipの一辺のピクセル数を取得（mip0からの半分ずつ、最小1）
        //--------------------------------------------------------------
        [[nodiscard]]
        u32 GetMipSize(u32 mip) const {
            u32 size = m_baseSize;
            for(u32 i = 0; i < mip; ++i)
                size = (size > 1) ? (size / 2) : 1;
            return size;
        }

        //--------------------------------------------------------------
        //! @brief  作成に成功したか
        //--------------------------------------------------------------
        [[nodiscard]]
        bool IsValid() const {
            return m_srv != nullptr;
        }

    private:
        u32 m_baseSize   = 0;    //!< mip0の一辺のピクセル数
        u32 m_mipLevels  = 1;    //!< ミップレベル数

        Microsoft::WRL::ComPtr<ID3D11Texture2D>          m_texture;    //!< ArraySize=6, MiscFlags=TEXTURECUBEの実体
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_srv;        //!< TextureCubeとして読むための1枚のSRV

        //! 面×mipごとのRTV。添字は mip * 6 + face（GetFaceRTVの計算と対応させること）
        std::vector<Microsoft::WRL::ComPtr<ID3D11RenderTargetView>> m_faceRTV;
    };

}    // namespace Tsukino::Renderer
