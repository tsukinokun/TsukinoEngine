//--------------------------------------------------------------
//! @file   PipelineFactory.cpp
//! @brief  パイプラインファクトリークラスの実装
//! @author 山﨑愛
//--------------------------------------------------------------
#include <Tsukino/Renderer/DX11/PipelineFactory.hpp>
#include <Tsukino/Engine/Asset/Shader/ShaderAsset.hpp>
#include <Tsukino/GraphicsCommon/Mesh/MeshData.hpp>
// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {
    //--------------------------------------------------------------
    //! @brief  パイプラインステートを作成する関数
    //--------------------------------------------------------------
    std::shared_ptr<PipelineState> PipelineFactory::Create(const Tsukino::Asset::ShaderAsset&    vs,
                                                           const Tsukino::Asset::ShaderAsset&    ps,
                                                           Tsukino::GraphicsCommon::VertexFormat format,
                                                           DepthMode                             depthMode,
                                                           BlendMode                             blendMode) {
        //--------------------------------------------------------------
        // シェーダーのハンドル値とフォーマット、デプスモードからキーを作成
        // 4つの要素を正しく波括弧初期化
        //--------------------------------------------------------------
        PipelineKey key = {vs.GetHandle().Value(), ps.GetHandle().Value(), format, depthMode, blendMode};

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

        //--------------------------------------------------------------
        // 頂点フォーマットに応じてインプットレイアウトを内部で切り替える
        //--------------------------------------------------------------
        const D3D11_INPUT_ELEMENT_DESC* finalLayout      = nullptr;
        UINT                            finalLayoutCount = 0;

        // 静的メッシュ用レイアウト (Slot 0)
        D3D11_INPUT_ELEMENT_DESC staticLayout[] = {
            {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0},
        };

        // スキンメッシュ用レイアウト (頂点は Slot 0 / ボーン情報は Slot 1 から跨いでマルチスロットで読み込む)
        D3D11_INPUT_ELEMENT_DESC skinnedLayout[] = {
            {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"BONE_INDICES", 0, DXGI_FORMAT_R32G32B32A32_UINT, 1, offsetof(Tsukino::GraphicsCommon::BoneWeight, boneIndices), D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"BONE_WEIGHTS", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, offsetof(Tsukino::GraphicsCommon::BoneWeight, weights), D3D11_INPUT_PER_VERTEX_DATA, 0},
        };

        // Sprite用
        D3D11_INPUT_ELEMENT_DESC spriteLayout[] = {
            {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
        };

        switch(format) {
        case Tsukino::GraphicsCommon::VertexFormat::PositionNormalUV:
            finalLayout      = staticLayout;
            finalLayoutCount = ARRAYSIZE(staticLayout);
            break;

        case Tsukino::GraphicsCommon::VertexFormat::Skinned:
            finalLayout      = skinnedLayout;
            finalLayoutCount = ARRAYSIZE(skinnedLayout);
            break;

        case Tsukino::GraphicsCommon::VertexFormat::Sprite:
            finalLayout      = spriteLayout;
            finalLayoutCount = ARRAYSIZE(spriteLayout);
            break;

        default:
            return nullptr;
        }

        // InputLayout 生成
        m_device->CreateInputLayout(finalLayout, finalLayoutCount, vs.binary.data(), vs.binary.size(), p->inputLayout.GetAddressOf());

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

        case DepthMode::EqualReadOnly:
            // 半透明フォワードの色パス用。直前のTransparentDepthパスが埋めた深度と
            // 一致する画素だけを通す（自己重なりのある1メッシュを1回だけシェーディングするため）
            desc.DepthEnable    = TRUE;
            desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
            desc.DepthFunc      = D3D11_COMPARISON_EQUAL;
            break;
        }
        // デプスステンシルステートの作成
        m_device->CreateDepthStencilState(&desc, p->depth.GetAddressOf());

        //--------------------------------------------------------------
        // ラスタライザーステートの設定
        //--------------------------------------------------------------
        D3D11_RASTERIZER_DESC rasterDesc = {};
        rasterDesc.FillMode              = D3D11_FILL_SOLID;
        rasterDesc.CullMode              = D3D11_CULL_NONE;
        rasterDesc.DepthClipEnable       = TRUE;
        m_device->CreateRasterizerState(&rasterDesc, p->rasterizer.GetAddressOf());

        //--------------------------------------------------------------
        // ブレンドステートの設定
        //--------------------------------------------------------------
        D3D11_BLEND_DESC blendDesc                      = {};
        blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

        switch(blendMode) {
        case BlendMode::Opaque:
            blendDesc.RenderTarget[0].BlendEnable = FALSE;
            break;

        case BlendMode::DepthOnly:
            // 色を一切書かない（深度事前パス専用）。ブレンド自体は無効のままで良い
            blendDesc.RenderTarget[0].BlendEnable           = FALSE;
            blendDesc.RenderTarget[0].RenderTargetWriteMask = 0;
            break;

        case BlendMode::Alpha:
            blendDesc.RenderTarget[0].BlendEnable    = TRUE;
            blendDesc.RenderTarget[0].SrcBlend       = D3D11_BLEND_SRC_ALPHA;
            blendDesc.RenderTarget[0].DestBlend      = D3D11_BLEND_INV_SRC_ALPHA;
            blendDesc.RenderTarget[0].BlendOp        = D3D11_BLEND_OP_ADD;
            blendDesc.RenderTarget[0].SrcBlendAlpha  = D3D11_BLEND_SRC_ALPHA;        
            blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;    
            blendDesc.RenderTarget[0].BlendOpAlpha   = D3D11_BLEND_OP_ADD;
            break;

        case BlendMode::Additive:
            blendDesc.RenderTarget[0].BlendEnable    = TRUE;
            blendDesc.RenderTarget[0].SrcBlend       = D3D11_BLEND_SRC_ALPHA;
            blendDesc.RenderTarget[0].DestBlend      = D3D11_BLEND_ONE;
            blendDesc.RenderTarget[0].BlendOp        = D3D11_BLEND_OP_ADD;
            // アルファチャンネルは書き換えない（DestBlendAlpha=ONEで既存値を保持）。
            // HDRバッファのアルファはTonemap.ps.hlslが最終色への事前乗算（Premultiplied Alpha）に
            // 使っており、以前はSrcBlendAlpha=ONE/DestBlendAlpha=ZEROでスプライト自身のアルファに
            // 上書きしていたため、加算合成スプライトの透明部分（アルファ0）を描いた画素のHDRアルファが
            // 0になり、トーンマップで強制的に黒く潰れてしまっていた（ExpOrb.pngの透明部分が黒く
            // 見える不具合の原因）
            blendDesc.RenderTarget[0].SrcBlendAlpha  = D3D11_BLEND_ZERO;
            blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
            blendDesc.RenderTarget[0].BlendOpAlpha   = D3D11_BLEND_OP_ADD;
            break;
        }
        m_device->CreateBlendState(&blendDesc, p->blend.GetAddressOf());

        p->topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

        m_cache[key] = p;
        return p;
    }

}    // namespace Tsukino::Renderer
