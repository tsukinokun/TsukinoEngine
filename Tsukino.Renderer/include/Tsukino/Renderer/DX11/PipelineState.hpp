//--------------------------------------------------------------
//! @file   PipelineState.hpp
//! @brief  パイプラインステート構造体の宣言
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
#include <d3d11.h>
// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {
    //--------------------------------------------------------------
    //! @struct PipelineState
    //! @brief  パイプラインステート構造体
    //--------------------------------------------------------------
    struct PipelineState {
        ID3D11VertexShader*      vs;             // 頂点シェーダー
        ID3D11PixelShader*       ps;             // ピクセルシェーダー
        ID3D11InputLayout*       inputLayout;    // 入力レイアウト
        ID3D11RasterizerState*   rasterizer;     // ラスタライザーステート
        ID3D11BlendState*        blend;          // ブレンドステート
        ID3D11DepthStencilState* depth;          // デプスステンシルステート
        D3D11_PRIMITIVE_TOPOLOGY topology;       // プリミティブトポロジー
    };
}    // namespace Tsukino::Renderer
