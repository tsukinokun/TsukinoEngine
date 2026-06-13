//--------------------------------------------------------------
//! @file   PipelineFactory.cpp
//! @brief  パイプラインファクトリークラスの実装
//! @author 山﨑愛
//--------------------------------------------------------------
#include <Tsukino/Renderer/DX11/PipelineFactory.hpp>
#include <Tsukino/Engine/Asset/Shader/ShaderAsset.hpp>
// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {
    //--------------------------------------------------------------
    //! @brief  パイプラインステートを作成する関数
    //--------------------------------------------------------------
    std::shared_ptr<PipelineState> PipelineFactory::Create(const Tsukino::Asset::ShaderAsset& vs,
                                                           const Tsukino::Asset::ShaderAsset& ps,
                                                           const D3D11_INPUT_ELEMENT_DESC*    layout,
                                                           UINT                               layoutCount,
                                                           DepthMode                          depthMode) {
        //--------------------------------------------------------------
        // シェーダーのハンドル値を取り出してキーを作成
        //--------------------------------------------------------------
        PipelineKey key = {vs.GetHandle().Value(), ps.GetHandle().Value(), depthMode};

        //--------------------------------------------------------------
        // キャッシュに存在する場合はそれを即座に返す（これによって毎フレームの生成コストをゼロ化）
        //--------------------------------------------------------------
        if(m_cache.find(key) != m_cache.end()) {
            return m_cache[key];
        }

        //--------------------------------------------------------------
        // 存在しない場合は新規作成
        //--------------------------------------------------------------
        std::shared_ptr<PipelineState> p = std::make_shared<PipelineState>();

        // VS
        m_device->CreateVertexShader(vs.binary.data(), vs.binary.size(), nullptr, p->vs.GetAddressOf());

        // PS
        m_device->CreatePixelShader(ps.binary.data(), ps.binary.size(), nullptr, p->ps.GetAddressOf());

        // InputLayout
        m_device->CreateInputLayout(layout, layoutCount, vs.binary.data(), vs.binary.size(), p->inputLayout.GetAddressOf());

        //--------------------------------------------------------------
        // デプスステンシルステートの設定
        //--------------------------------------------------------------
        D3D11_DEPTH_STENCIL_DESC desc = {};    // デフォルト値で初期化
        // DepthMode に応じて設定を変更
        switch(depthMode) {
        case DepthMode::None:
            desc.DepthEnable    = FALSE;
            desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
            desc.DepthFunc      = D3D11_COMPARISON_ALWAYS;
            break;

        case DepthMode::ReadWrite:
            desc.DepthEnable    = TRUE;
            desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
            desc.DepthFunc      = D3D11_COMPARISON_GREATER_EQUAL;
            break;

        case DepthMode::ReadOnly:
            desc.DepthEnable    = TRUE;
            desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
            desc.DepthFunc      = D3D11_COMPARISON_GREATER_EQUAL;
            break;
        }
        // デプスステンシルステートの作成
        m_device->CreateDepthStencilState(&desc, p->depth.GetAddressOf());
        D3D11_RASTERIZER_DESC rasterDesc = {};
        rasterDesc.FillMode              = D3D11_FILL_SOLID;
        rasterDesc.CullMode              = D3D11_CULL_NONE;
        rasterDesc.DepthClipEnable       = TRUE;
        m_device->CreateRasterizerState(&rasterDesc, p->rasterizer.GetAddressOf());

        p->topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

        // 4. キャッシュに保存してから返す
        m_cache[key] = p;
        return p;
    }

}    // namespace Tsukino::Renderer
