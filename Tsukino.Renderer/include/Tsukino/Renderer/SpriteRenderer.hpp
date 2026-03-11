//------------------------------------------------------------
//! @file   SpriteRenderer.hpp
//! @brief  スプライト描画クラス
//! @author 山﨑愛
//------------------------------------------------------------
#pragma once

#include <Tsukino/Core/typedef.hpp>
#include <Tsukino/Renderer/DX11/Texture/DX11Texture2D.hpp>

#include <d3d11.h>
#include <wrl/client.h>

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {

    //------------------------------------------------------------
    //! @class  SpriteRenderer
    //! @brief  2Dスプライト描画クラス
    //------------------------------------------------------------
    class SpriteRenderer {
    public:
        //------------------------------------------------------------
        // 初期化
        //! @param device [in] DirectX11 Device
        //------------------------------------------------------------
        bool Initialize(ID3D11Device* device);

        //------------------------------------------------------------
        // スプライト描画
        //! @param context [in] DeviceContext
        //! @param texture [in] 描画するテクスチャ
        //------------------------------------------------------------
        void Draw(ID3D11DeviceContext* context, DX11Texture2D* texture);

    private:
        //------------------------------------------------------------
        //! 頂点構造体
        //------------------------------------------------------------
        struct Vertex {
            float position[3];
            float uv[2];
        };

        Microsoft::WRL::ComPtr<ID3D11Buffer> m_vertexBuffer;
        Microsoft::WRL::ComPtr<ID3D11Buffer> m_indexBuffer;

        Microsoft::WRL::ComPtr<ID3D11VertexShader> m_vertexShader;
        Microsoft::WRL::ComPtr<ID3D11PixelShader>  m_pixelShader;

        Microsoft::WRL::ComPtr<ID3D11InputLayout> m_inputLayout;

        Microsoft::WRL::ComPtr<ID3D11SamplerState> m_sampler;
    };

}    // namespace Tsukino::Renderer
