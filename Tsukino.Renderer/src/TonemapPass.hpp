//----------------------------------------------------------------------------
//! @file   TonemapPass.hpp
//! @brief  トーンマップパスの宣言
//! @detail Tsukino.Renderer の内部専用ヘッダです。HDR のシーンカラーを LDR へ変換して
//!         バックバッファへ書きます。外から触る設定が無いので公開していません。
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/Core/WindowsLean.hpp>

#include <wrl/client.h>
#include <d3d11.h>

namespace Tsukino::Asset {
    class ShaderAsset;    // 前方宣言
}

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {
    class GraphicsContext;    // 前方宣言
    class RenderResources;    // 前方宣言
    class FullscreenPass;     // 前方宣言

    //------------------------------------------------------------------------
    //! トーンマップパスのクラスです。
    //------------------------------------------------------------------------
    class TonemapPass {
    public:

        //! ピクセルシェーダーを作成します。
        //! @param  [in] graphicsContext デバイスとバックバッファの取得元
        //! @param  [in] resources       共通ステートとサンプラーの取得元
        //! @param  [in] fullscreenPass  フルスクリーン三角形の描画
        //! @param  [in] ps              トーンマップ用のピクセルシェーダー
        //! @return true: 作成成功, false: 作成失敗（パスは何もしなくなる）
        [[nodiscard]]
        bool Initialize(GraphicsContext& graphicsContext, RenderResources& resources, const FullscreenPass& fullscreenPass,
                        const Tsukino::Asset::ShaderAsset* ps);

        //! トーンマップを実行してバックバッファへ書きます。
        //! @param  [in] source 入力となるシーンカラーの SRV（モーションブラーが走ったかで切り替わる）
        void Execute(ID3D11ShaderResourceView* source);

    private:
        GraphicsContext*      m_graphicsContext = nullptr;    // デバイスとバックバッファの取得元（借りている）
        RenderResources*      m_resources       = nullptr;    // 共通ステートとサンプラーの取得元（借りている）
        const FullscreenPass* m_fullscreenPass  = nullptr;    // フルスクリーン三角形の描画（借りている）

        Microsoft::WRL::ComPtr<ID3D11PixelShader> m_ps;    // トーンマップ用ピクセルシェーダー
    };
}    // namespace Tsukino::Renderer
