//------------------------------------------------------------
//! @file   Shader.cpp
//! @brief  Shader クラスの実装（VS/PS + InputLayout のバインド）
//! @author 山﨑愛
//------------------------------------------------------------
#include "Tsukino/Renderer/Shader.h"
// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {

    //------------------------------------------------------------
    //! @brief シェーダをパイプラインにバインドする
    //------------------------------------------------------------
    void Shader::Bind(ID3D11DeviceContext* context) {
        // 入力レイアウトをセット
        context->IASetInputLayout(m_inputLayout.Get());

        // 頂点シェーダーをセット
        context->VSSetShader(m_vertexShader.Get(), nullptr, 0);

        // ピクセルシェーダーをセット
        context->PSSetShader(m_pixelShader.Get(), nullptr, 0);
    }

}    // namespace Tsukino::Renderer
