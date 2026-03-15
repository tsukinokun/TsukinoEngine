//--------------------------------------------------------------
//! @file   PipelineState.hpp
//! @brief  パイプラインステート構造体の宣言
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
#include <d3d11.h>
#include <wrl/client.h>
// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {
    //--------------------------------------------------------------
    //! @struct PipelineState
    //! @brief  パイプラインステート構造体
    //--------------------------------------------------------------
    struct PipelineState {
        Microsoft::WRL::ComPtr<ID3D11VertexShader>      vs;             // 頂点シェーダー
        Microsoft::WRL::ComPtr<ID3D11PixelShader>       ps;             // ピクセルシェーダー
        Microsoft::WRL::ComPtr<ID3D11InputLayout>       inputLayout;    // 入力レイアウト
        Microsoft::WRL::ComPtr<ID3D11RasterizerState>   rasterizer;     // ラスタライザーステート
        Microsoft::WRL::ComPtr<ID3D11BlendState>        blend;          // ブレンドステート
        Microsoft::WRL::ComPtr<ID3D11DepthStencilState> depth;          // デプスステンシルステート
        D3D11_PRIMITIVE_TOPOLOGY                        topology;       // プリミティブトポロジー
    };
}    // namespace Tsukino::Renderer
