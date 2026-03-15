//------------------------------------------------------------
//! @file   SpriteRenderer.cpp
//! @brief  スプライト描画クラスの実装
//! @author 山﨑愛
//------------------------------------------------------------
#include <Tsukino/Renderer/SpriteRenderer.hpp>
#include <Tsukino/Renderer/DX11/GraphicsContext.hpp>
#include <Tsukino/Renderer/DX11/Material.hpp>
#include <Tsukino/Renderer/DX11/MeshBuffer.hpp>
// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {
    //------------------------------------------------------------
    //! @brief sprite描画を行う関数
    //------------------------------------------------------------
    void SpriteRenderer::Draw(GraphicsContext* gfx, Material* material, MeshBuffer* mesh) {
        gfx->SetMaterial(*material);    // マテリアルのセット

        // 頂点バッファとインデックスバッファのセット
        UINT          stride = mesh->stride;
        UINT          offset = 0;
        ID3D11Buffer* vb     = mesh->vertexBuffer.Get();

        gfx->GetContext()->IASetVertexBuffers(0, 1, &vb, &stride, &offset);

        gfx->GetContext()->IASetIndexBuffer(mesh->indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

        gfx->GetContext()->DrawIndexed(mesh->indexCount, 0, 0);
    }

}    // namespace Tsukino::Renderer
