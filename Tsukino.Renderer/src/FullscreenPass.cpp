//----------------------------------------------------------------------------
//! @file   FullscreenPass.cpp
//! @brief  画面全体を覆う三角形の描画の実装
//----------------------------------------------------------------------------
#include "FullscreenPass.hpp"

#include <Tsukino/Engine/Asset/Shader/ShaderAsset.hpp>

#include <Tsukino/Core/Log.hpp>

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {
    //------------------------------------------------------------------------
    //! 頂点シェーダーを作成します。
    //------------------------------------------------------------------------
    bool FullscreenPass::Initialize(ID3D11Device* device, const Tsukino::Asset::ShaderAsset* vs) {
        if(!vs) {
            Tsukino::Core::Log::Error("FullscreenPass::Initialize - shader is null.");
            return false;
        }

        if(FAILED(device->CreateVertexShader(vs->binary.data(), vs->binary.size(), nullptr, m_vs.GetAddressOf()))) {
            Tsukino::Core::Log::Error("Failed to create fullscreen triangle vertex shader.");
            m_vs.Reset();
            return false;
        }

        return true;
    }

    //------------------------------------------------------------------------
    //! 頂点バッファ・インデックスバッファを外し、トポロジーを三角形リストにします。
    //------------------------------------------------------------------------
    void FullscreenPass::BindGeometry(ID3D11DeviceContext* context) {
        context->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
        context->IASetIndexBuffer(nullptr, DXGI_FORMAT_UNKNOWN, 0);
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    }

    //------------------------------------------------------------------------
    //! フルスクリーン三角形を1枚描きます（BindGeometry の後に Draw(3, 0) します）。
    //------------------------------------------------------------------------
    void FullscreenPass::Draw(ID3D11DeviceContext* context) {
        BindGeometry(context);
        context->Draw(3, 0);
    }
}    // namespace Tsukino::Renderer
