//----------------------------------------------------------------------------
//! @file   FullscreenPass.cpp
//! @brief  bgfx 版のフルスクリーン三角形の描画の実装
//----------------------------------------------------------------------------
#include "FullscreenPass.hpp"

#include <Tsukino/Renderer/Bgfx/ShaderPrograms.hpp>

#include <Tsukino/Engine/Asset/Shader/ShaderAsset.hpp>

#include <Tsukino/Core/Log.hpp>

#include <cstring>

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {

    namespace {
        //--------------------------------------------------------------------
        //! フルスクリーン三角形の頂点です。
        //--------------------------------------------------------------------
        struct FullscreenVertex {
            float x, y, z;    // 位置（クリップ空間）
            float u, v;       // UV（左下が (0, 0)。HLSL 版の SV_VertexID から作る UV と同じ）
        };

        //! 画面を覆う大きな三角形（DX11 版の Tonemap.vs.hlsl の id = 0, 1, 2 と同じ頂点）
        constexpr FullscreenVertex kVertices[3] = {
            {-1.0f, -1.0f, 0.0f, 0.0f, 0.0f},
            { 3.0f, -1.0f, 0.0f, 2.0f, 0.0f},
            {-1.0f,  3.0f, 0.0f, 0.0f, 2.0f},
        };
    }    // namespace

    //------------------------------------------------------------------------
    //! 頂点シェーダーと頂点レイアウトを用意します。
    //------------------------------------------------------------------------
    bool FullscreenPass::Initialize(const Tsukino::Asset::ShaderAsset* vs) {
        m_vs    = vs;
        m_valid = vs && IsBgfxShaderBinary(vs->binary);
        m_layout.begin()
            .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
            .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
            .end();
        if(!m_valid)
            Tsukino::Core::Log::Error("FullscreenPass: The fullscreen vertex shader is not a bgfx shader.");
        return m_valid;
    }

    //------------------------------------------------------------------------
    //! この頂点シェーダーとピクセルシェーダーを組にしたプログラムを作ります。
    //------------------------------------------------------------------------
    bgfx::ProgramHandle FullscreenPass::CreateProgram(const Tsukino::Asset::ShaderAsset* ps) const {
        return m_valid ? CreateBgfxProgram(m_vs, ps) : bgfx::ProgramHandle BGFX_INVALID_HANDLE;
    }

    //------------------------------------------------------------------------
    //! フルスクリーン三角形を1枚描きます。
    //------------------------------------------------------------------------
    void FullscreenPass::Submit(bgfx::ViewId view, bgfx::ProgramHandle program, u64 state) const {
        if(!m_valid || !bgfx::isValid(program) || bgfx::getAvailTransientVertexBuffer(3, m_layout) < 3) {
            bgfx::discard();    // 先に設定したテクスチャ・ユニフォームを次の描画へ持ち越さない
            return;
        }

        bgfx::TransientVertexBuffer vertexBuffer;
        bgfx::allocTransientVertexBuffer(&vertexBuffer, 3, m_layout);
        std::memcpy(vertexBuffer.data, kVertices, sizeof(kVertices));

        bgfx::setVertexBuffer(0, &vertexBuffer);
        bgfx::setState(state);
        bgfx::submit(view, program);
    }

}    // namespace Tsukino::Renderer
