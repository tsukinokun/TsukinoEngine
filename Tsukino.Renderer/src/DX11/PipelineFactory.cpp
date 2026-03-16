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
                                           UINT                               layoutCount) {
        // 新しいパイプラインステートを作成
        std::shared_ptr<PipelineState> p = std::make_shared<PipelineState>();

        // VS
        m_device->CreateVertexShader(vs.binary.data(), vs.binary.size(), nullptr, p->vs.GetAddressOf());

        // PS
        m_device->CreatePixelShader(ps.binary.data(), ps.binary.size(), nullptr, p->ps.GetAddressOf());

        // InputLayout
        m_device->CreateInputLayout(layout, layoutCount, vs.binary.data(), vs.binary.size(), p->inputLayout.GetAddressOf());

        return p;
    }

}    // namespace Tsukino::Renderer
