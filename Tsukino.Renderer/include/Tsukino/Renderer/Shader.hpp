//------------------------------------------------------------
//! @file    Shader.hpp
//! @brief   DirectX11シェーダ（VS/PS + InputLayout）を保持するクラス
//! @details Rendererからは Bind() だけを呼べばよい。
//!          読み込み・コンパイルは ShaderLoader が担当する。
//! @author  山﨑愛
//------------------------------------------------------------
#pragma once
// windows.h の min/max マクロを避けるため、DX11 / Effekseer より先に通す
#include <Tsukino/Core/WindowsLean.hpp>
#include <wrl/client.h>
#include <d3d11.h>
// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {
    // ComPtr の using 宣言。公開ヘッダなのでグローバルではなく名前空間の内側に置く
    using Microsoft::WRL::ComPtr;

    //------------------------------------------------------------
    //! @class Shader
    //! @brief 1つの頂点シェーダ + ピクセルシェーダ + 入力レイアウトを表す
    //------------------------------------------------------------
    class Shader {
    public:
        //------------------------------------------------------------
        //! @brief コンストラクタ
        //------------------------------------------------------------
        Shader() = default;

        //------------------------------------------------------------
        //! @brief デストラクタ
        //------------------------------------------------------------
        ~Shader() = default;

        //------------------------------------------------------------
        // シェーダをパイプラインにバインドする
        //! @param context [in] デバイスコンテキスト
        //------------------------------------------------------------
        void Bind(ID3D11DeviceContext* context);

    private:
        // ShaderLoader が内部メンバへアクセスできるようにする
        friend class ShaderLoader;

        // GPU リソース
        ComPtr<ID3D11VertexShader> m_vertexShader;    // 頂点シェーダ
        ComPtr<ID3D11PixelShader>  m_pixelShader;     // ピクセルシェーダ
        ComPtr<ID3D11InputLayout>  m_inputLayout;     // 入力レイアウト
    };

}    // namespace Tsukino::Renderer
