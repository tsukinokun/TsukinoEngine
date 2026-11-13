//----------------------------------------------------------------------------
//! @file   DrawCommandExecutor.cpp
//! @brief  bgfx 版の、描画コマンド1本を GPU へ流す処理の実装
//! @detail メッシュはパイプライン（PipelineFactory が作る bgfx のプログラムとステート）で描きます。
//!         スプライトの並びは、DX11 版の DirectXTK SpriteBatch と同じ置き方で四角の頂点を CPU で作り、
//!         同じテクスチャが続くところをまとめて一時的な頂点バッファで描きます。
//!         線は一時的な頂点バッファへ写して、線のリストとして描きます。
//----------------------------------------------------------------------------
#include "DrawCommandExecutor.hpp"

#include <Tsukino/Renderer/Material.hpp>
#include <Tsukino/Renderer/MeshBuffer.hpp>
#include <Tsukino/Renderer/Bgfx/FrameConstantsGpu.hpp>
#include <Tsukino/Renderer/Bgfx/GraphicsContext.hpp>
#include <Tsukino/Renderer/Bgfx/IBLBakerGpu.hpp>
#include <Tsukino/Renderer/Bgfx/ShaderPrograms.hpp>
#include <Tsukino/Renderer/Bgfx/RenderResourcesGpu.hpp>

#include <Tsukino/Engine/Asset/Shader/ShaderAsset.hpp>

#include <Tsukino/GraphicsCommon/Vertex/DebugVertex.hpp>

#include <Tsukino/Core/Log.hpp>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <vector>

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {

    namespace {
        //--------------------------------------------------------------------
        //! スプライトの並びの頂点（位置はクリップ空間、色、UV）です。
        //--------------------------------------------------------------------
        struct SpriteVertex {
            float x, y, z;         // 位置（クリップ空間。z は使わない）
            float r, g, b, a;      // テクスチャに掛ける色
            float u, v;            // UV
        };

        //! 1回の Submit で描くスプライトの枚数の上限（16bit のインデックスで足りる数）
        constexpr u32 kMaxSpritesPerSubmit = 0xFFFF / 4;

        //! マテリアルの既定値（materialData を持たないコマンドが最初に来たとき用。色は白）
        const CBufferMaterial& DefaultMaterial() {
            static const CBufferMaterial material = [] {
                CBufferMaterial m{};
                m.baseColor = hlslpp::float4(1.0f, 1.0f, 1.0f, 1.0f);
                m.emissive  = hlslpp::float3(0.0f, 0.0f, 0.0f);
                m.metallic  = 0.0f;
                m.roughness = 1.0f;
                m.specular  = 0.5f;
                m.alphaCutoff = 0.0f;
                m.rimColor  = hlslpp::float4(0.0f, 0.0f, 0.0f, 0.0f);
                m.rimParams = hlslpp::float4(0.0f, 0.0f, 0.0f, 0.0f);
                return m;
            }();
            return material;
        }
    }    // namespace

    //------------------------------------------------------------------------
    //! ユニフォームとシェーダーを破棄します。
    //------------------------------------------------------------------------
    DrawCommandExecutor::~DrawCommandExecutor() {
        if(!GraphicsContext::IsBgfxAlive())
            return;
        for(bgfx::UniformHandle& sampler : m_materialSamplers) {
            if(bgfx::isValid(sampler))
                bgfx::destroy(sampler);
        }
        for(bgfx::UniformHandle uniform : {m_spriteSampler, m_bonesUniform, m_bonesPrevUniform}) {
            if(bgfx::isValid(uniform))
                bgfx::destroy(uniform);
        }
        if(bgfx::isValid(m_spriteProgram))
            bgfx::destroy(m_spriteProgram);
        if(bgfx::isValid(m_lineProgram))
            bgfx::destroy(m_lineProgram);
    }

    //------------------------------------------------------------------------
    //! ユニフォームと、スプライト・線を描くシェーダーを作成します。
    //------------------------------------------------------------------------
    bool DrawCommandExecutor::Initialize(const GraphicsContext&             graphicsContext,
                                         RenderResources&                   resources,
                                         FrameConstants&                    frameConstants,
                                         const DrawCommandQueue&            drawQueue,
                                         const IBLBaker&                    iblBaker,
                                         const Tsukino::Asset::ShaderAsset* lineVS,
                                         const Tsukino::Asset::ShaderAsset* linePS) {
        m_graphicsContext = &graphicsContext;
        m_resources       = &resources;
        m_frameConstants  = &frameConstants;
        m_drawQueue       = &drawQueue;
        m_iblBaker        = &iblBaker;
        m_lastMaterial    = DefaultMaterial();

        //--------------------------------------------------------------------
        // 定数バッファ（b1・b2）の代わりのユニフォームと、マテリアルのテクスチャのサンプラー。
        // 名前は Shaders/bgfx の .sh・.sc と揃える
        //--------------------------------------------------------------------
        if(!m_transformUniform.Create("u_transform") || !m_materialUniform.Create("u_material")) {
            Tsukino::Core::Log::Error("DrawCommandExecutor: Failed to create uniforms.");
            return false;
        }
        constexpr const char* kSamplerNames[] = {"s_albedo", "s_normalMap", "s_metallicRoughness", "s_emissiveMap", "s_aoMap"};
        for(size_t i = 0; i < m_materialSamplers.size(); ++i)
            m_materialSamplers[i] = bgfx::createUniform(kSamplerNames[i], bgfx::UniformType::Sampler);
        m_spriteSampler = bgfx::createUniform("s_texture", bgfx::UniformType::Sampler);

        // ボーン行列（Model.vs.sc の u_bones / u_bonesPrev。1本4行で128本ぶん）
        constexpr u16 kBoneVec4Count = static_cast<u16>(sizeof(CBufferSkinning) / 16);
        m_bonesUniform     = bgfx::createUniform("u_bones", bgfx::UniformType::Vec4, kBoneVec4Count);
        m_bonesPrevUniform = bgfx::createUniform("u_bonesPrev", bgfx::UniformType::Vec4, kBoneVec4Count);

        //--------------------------------------------------------------------
        // スプライトの並び（DrawKind::Sprites）。DX11 版は DirectXTK の SpriteBatch が
        // シェーダーを持っていたので、RendererShaderSet には無い。組み込みのシェーダーから直接読む
        //--------------------------------------------------------------------
        m_spriteLayout.begin()
            .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
            .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Float)
            .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
            .end();
        m_spriteProgram = LoadBuiltInBgfxProgram("SpriteBatch.vs", "SpriteBatch.ps");
        if(!bgfx::isValid(m_spriteProgram))
            Tsukino::Core::Log::Error("DrawCommandExecutor: Failed to load the sprite batch shaders; text is not drawn.");

        //--------------------------------------------------------------------
        // 線（DrawKind::Lines）
        //--------------------------------------------------------------------
        m_lineLayout.begin()
            .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
            .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Float)
            .end();
        m_lineProgram = CreateBgfxProgram(lineVS, linePS);
        if(!bgfx::isValid(m_lineProgram))
            Tsukino::Core::Log::Error("DrawCommandExecutor: The line shaders are not bgfx shaders; debug lines are not drawn.");

        return true;
    }

    //------------------------------------------------------------------------
    //! 描画コマンドを実行します。
    //------------------------------------------------------------------------
    void DrawCommandExecutor::Execute(const DrawCommand& cmd, bgfx::ViewId view, Renderer::FrameStats& stats) {
        switch(cmd.kind) {
        case DrawKind::Sprites: ExecuteSprites(cmd, view, stats); return;
        case DrawKind::Lines:   ExecuteLines(cmd, view, stats); return;
        case DrawKind::Mesh:    ExecuteMesh(cmd, view, stats); return;
        }
    }

    //------------------------------------------------------------------------
    //! DrawKind::Mesh のコマンドを描きます。
    //------------------------------------------------------------------------
    void DrawCommandExecutor::ExecuteMesh(const DrawCommand& cmd, bgfx::ViewId view, Renderer::FrameStats& stats) {
        if(!cmd.material || !cmd.mesh || !cmd.mesh->gpu)
            return;

        //--------------------------------------------------------------------
        // まだ bgfx 版へ移していないシェーダー・頂点の形式（スキニングなど）は描かない
        //--------------------------------------------------------------------
        const PipelineState*   pipeline = cmd.material->GetPipeline();
        const MeshBuffer::Gpu& mesh     = *cmd.mesh->gpu;
        if(!pipeline || !bgfx::isValid(pipeline->program) || !bgfx::isValid(mesh.vertexBuffer))
            return;

        // スキニングのシェーダーは2本目の頂点バッファ（ボーン）を読むので、それが無いメッシュは描かない
        const bool isSkeletal = cmd.boneMatrices && cmd.boneCount > 0;
        if(pipeline->format == Tsukino::GraphicsCommon::VertexFormat::Skinned && (!isSkeletal || !bgfx::isValid(mesh.boneBuffer)))
            return;

        //--------------------------------------------------------------------
        // 定数（シーン・物ごと・マテリアル）。bgfx のユニフォームは描画1回ごとの値なので毎回送る。
        // マテリアルの定数を持たないコマンドは、DX11 版と同じく直前の値を引き継ぐ
        //--------------------------------------------------------------------
        m_frameConstants->GetGpu().Apply();

        CBufferTransform transform{};
        transform.world       = cmd.transform;
        transform.prevWorld   = cmd.transform;    // 速度（モーションブラー）は段階 1-4 d で移す
        transform.motionFlags = hlslpp::float4(0.0f, cmd.ditherFade, cmd.occlusionCutout ? 1.0f : 0.0f, 0.0f);
        m_transformUniform.Set(transform);

        if(cmd.materialData)
            m_lastMaterial = *cmd.materialData;
        m_materialUniform.Set(m_lastMaterial);

        //--------------------------------------------------------------------
        // テクスチャ（t0〜t4）。設定されていない枠は、色は白、法線は平らな法線で埋める
        //--------------------------------------------------------------------
        const RenderResources::Gpu& resources = m_resources->GetGpu();
        const u32 samplerFlags = cmd.material->GetSampler() ? RenderResources::Gpu::GetSamplerFlags(*cmd.material->GetSampler())
                                                            : BGFX_SAMPLER_NONE;    // 既定は DX11 版と同じ LinearWrap
        const auto& textures = cmd.material->GetTextures();
        for(size_t slot = 0; slot < textures.size(); ++slot) {
            const RenderResources::Gpu::TextureEntry* entry = resources.FindTexture(textures[slot]);
            if(!entry)
                entry = resources.FindTexture(slot == static_cast<size_t>(SRVSlot::Normal) ? resources.flatNormal : resources.white);
            if(entry)
                bgfx::setTexture(static_cast<u8>(slot), m_materialSamplers[slot], entry->texture, samplerFlags);
        }

        //--------------------------------------------------------------------
        // ボーン行列。シェーダーの配列は 128 本ぶんだが、使う本数だけ送る。
        // 前フレームの行列は、速度（モーションブラー。段階 1-4 d）を出すまで送らない
        //--------------------------------------------------------------------
        if(isSkeletal && bgfx::isValid(mesh.boneBuffer)) {
            constexpr u32 kMaxBoneCount = sizeof(CBufferSkinning) / sizeof(CBufferSkinning::bones[0]);
            const u16     vec4Count     = static_cast<u16>(std::min(cmd.boneCount, kMaxBoneCount) * 4);
            bgfx::setUniform(m_bonesUniform, cmd.boneMatrices, vec4Count);
            bgfx::setUniform(m_bonesPrevUniform, cmd.boneMatrices, vec4Count);
            bgfx::setVertexBuffer(1, mesh.boneBuffer);
            ++stats.skinnedDrawCalls;
            stats.boneBytesUploaded += static_cast<u64>(vec4Count) * 16;
        }

        // 環境光（フォワードで照らすモデルが読む。読まないシェーダーには何も起きない）
        if(m_iblBaker)
            m_iblBaker->GetGpu().Bind();

        bgfx::setVertexBuffer(0, mesh.vertexBuffer);
        if(bgfx::isValid(mesh.indexBuffer))
            bgfx::setIndexBuffer(mesh.indexBuffer);
        if(cmd.instanceCount > 1)
            bgfx::setInstanceCount(cmd.instanceCount);

        ApplyClipRect(cmd);
        bgfx::setState(pipeline->state);
        bgfx::submit(view, pipeline->program);

        CountDrawCall(cmd.pass, stats);
        stats.triangleCount += (static_cast<u64>(cmd.mesh->indexCount) * cmd.instanceCount) / 3;
    }

    //------------------------------------------------------------------------
    //! DrawKind::Sprites のコマンドを描きます。
    //------------------------------------------------------------------------
    void DrawCommandExecutor::ExecuteSprites(const DrawCommand& cmd, bgfx::ViewId view, Renderer::FrameStats& stats) {
        const std::vector<SpriteQuad>& sprites = m_drawQueue->GetSprites();
        if(!bgfx::isValid(m_spriteProgram) || cmd.batchCount == 0 || static_cast<size_t>(cmd.batchFirst) + cmd.batchCount > sprites.size())
            return;

        const float screenWidth  = static_cast<float>(m_graphicsContext->GetWidth());
        const float screenHeight = static_cast<float>(m_graphicsContext->GetHeight());
        if(screenWidth <= 0.0f || screenHeight <= 0.0f)
            return;

        // ピクセル（左上原点・下向きが +Y）からクリップ空間（中央原点・上向きが +Y）への換算
        const float toClipX = 2.0f / screenWidth;
        const float toClipY = -2.0f / screenHeight;

        // straight alpha のブレンド・深度は使わない・カリングなし（DirectXTK の NonPremultiplied と同じ）
        constexpr u64 kState = BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A
                               | BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_SRC_ALPHA, BGFX_STATE_BLEND_INV_SRC_ALPHA);
        // サンプラーは SpriteBatch の既定と同じ LinearClamp
        constexpr u32 kSampler = BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP;

        const RenderResources::Gpu& resources = m_resources->GetGpu();
        const u32                   end       = cmd.batchFirst + cmd.batchCount;

        //--------------------------------------------------------------------
        // 同じテクスチャが続く範囲ごとに、1回の Submit で描く（積んだ順は保つ）
        //--------------------------------------------------------------------
        u32 index = cmd.batchFirst;
        while(index < end) {
            const RenderResources::Gpu::TextureEntry* texture = resources.FindTexture(sprites[index].texture);
            if(!texture) {
                ++index;    // テクスチャの無いスプライトは描かない（DX11 版と同じ）
                continue;
            }

            u32 runEnd = index + 1;
            while(runEnd < end && runEnd - index < kMaxSpritesPerSubmit && sprites[runEnd].texture == sprites[index].texture)
                ++runEnd;

            u32 count = runEnd - index;
            count     = std::min(count, bgfx::getAvailTransientVertexBuffer(count * 4, m_spriteLayout) / 4);
            count     = std::min(count, bgfx::getAvailTransientIndexBuffer(count * 6) / 6);
            if(count == 0) {
                Tsukino::Core::Log::Warn("DrawCommandExecutor: Out of transient buffer space; some sprites are not drawn this frame.");
                return;
            }

            bgfx::TransientVertexBuffer vertexBuffer;
            bgfx::TransientIndexBuffer  indexBuffer;
            bgfx::allocTransientBuffers(&vertexBuffer, m_spriteLayout, count * 4, &indexBuffer, count * 6);
            auto* vertices = reinterpret_cast<SpriteVertex*>(vertexBuffer.data);
            auto* indices  = reinterpret_cast<u16*>(indexBuffer.data);

            const float invTexWidth  = 1.0f / static_cast<float>(texture->width);
            const float invTexHeight = 1.0f / static_cast<float>(texture->height);

            for(u32 i = 0; i < count; ++i) {
                const SpriteQuad& sprite = sprites[index + i];

                //------------------------------------------------------------
                // 置き方は SpriteBatch と同じ。切り出した矩形の origin の点が (x, y) に来るように置き、
                // origin を軸に scale で拡大し、rotation（画面上で時計回りが正）で回す
                //------------------------------------------------------------
                const float sourceWidth  = static_cast<float>(sprite.source.right - sprite.source.left);
                const float sourceHeight = static_cast<float>(sprite.source.bottom - sprite.source.top);
                const float cosR         = std::cos(sprite.rotation);
                const float sinR         = std::sin(sprite.rotation);

                const float u0 = static_cast<float>(sprite.source.left) * invTexWidth;
                const float v0 = static_cast<float>(sprite.source.top) * invTexHeight;
                const float u1 = static_cast<float>(sprite.source.right) * invTexWidth;
                const float v1 = static_cast<float>(sprite.source.bottom) * invTexHeight;

                // 角の並び：左上・右上・左下・右下
                constexpr float kCornerX[4] = {0.0f, 1.0f, 0.0f, 1.0f};
                constexpr float kCornerY[4] = {0.0f, 0.0f, 1.0f, 1.0f};
                for(int corner = 0; corner < 4; ++corner) {
                    const float localX = (kCornerX[corner] * sourceWidth - sprite.originX) * sprite.scaleX;
                    const float localY = (kCornerY[corner] * sourceHeight - sprite.originY) * sprite.scaleY;
                    const float pixelX = sprite.x + localX * cosR - localY * sinR;
                    const float pixelY = sprite.y + localX * sinR + localY * cosR;

                    SpriteVertex& vertex = vertices[i * 4 + corner];
                    vertex.x = pixelX * toClipX - 1.0f;
                    vertex.y = pixelY * toClipY + 1.0f;
                    vertex.z = 0.0f;
                    vertex.r = sprite.color[0];
                    vertex.g = sprite.color[1];
                    vertex.b = sprite.color[2];
                    vertex.a = sprite.color[3];
                    vertex.u = kCornerX[corner] > 0.0f ? u1 : u0;
                    vertex.v = kCornerY[corner] > 0.0f ? v1 : v0;
                }

                const u16 base     = static_cast<u16>(i * 4);
                indices[i * 6 + 0] = base + 0;
                indices[i * 6 + 1] = base + 1;
                indices[i * 6 + 2] = base + 2;
                indices[i * 6 + 3] = base + 1;
                indices[i * 6 + 4] = base + 3;
                indices[i * 6 + 5] = base + 2;
            }

            bgfx::setVertexBuffer(0, &vertexBuffer);
            bgfx::setIndexBuffer(&indexBuffer);
            bgfx::setTexture(0, m_spriteSampler, texture->texture, kSampler);
            ApplyClipRect(cmd);
            bgfx::setState(kState);
            bgfx::submit(view, m_spriteProgram);

            stats.triangleCount += static_cast<u64>(count) * 2;
            index += count;
        }

        // DX11 版と同じく、1コマンドを1回と数える
        CountDrawCall(cmd.pass, stats);
    }

    //------------------------------------------------------------------------
    //! DrawKind::Lines のコマンドを描きます。
    //------------------------------------------------------------------------
    void DrawCommandExecutor::ExecuteLines(const DrawCommand& cmd, bgfx::ViewId view, Renderer::FrameStats& stats) {
        using Tsukino::GraphicsCommon::DebugVertex;
        static_assert(sizeof(DebugVertex) == sizeof(float) * 7, "DebugVertex must be position(3) + color(4) floats to match the line layout.");

        const std::vector<DebugVertex>& vertices = m_drawQueue->GetLineVertices();
        if(!bgfx::isValid(m_lineProgram) || cmd.batchCount < 2 || static_cast<size_t>(cmd.batchFirst) + cmd.batchCount > vertices.size())
            return;

        // 不透明・深度テストと書き込みあり（リバース Z）・カリングなし・線のリスト（DX11 版と同じ）
        constexpr u64 kState = BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_WRITE_Z | BGFX_STATE_DEPTH_TEST_GEQUAL
                               | BGFX_STATE_PT_LINES;

        //--------------------------------------------------------------------
        // 一時的な頂点バッファに入りきらない分は、何回かに分けて描く
        //--------------------------------------------------------------------
        u32 remaining = cmd.batchCount & ~1u;
        u32 first     = cmd.batchFirst;
        while(remaining > 0) {
            const u32 count = std::min(remaining, bgfx::getAvailTransientVertexBuffer(remaining, m_lineLayout)) & ~1u;
            if(count == 0) {
                Tsukino::Core::Log::Warn("DrawCommandExecutor: Out of transient buffer space; some lines are not drawn this frame.");
                return;
            }

            bgfx::TransientVertexBuffer vertexBuffer;
            bgfx::allocTransientVertexBuffer(&vertexBuffer, count, m_lineLayout);
            std::memcpy(vertexBuffer.data, vertices.data() + first, static_cast<size_t>(count) * sizeof(DebugVertex));

            m_frameConstants->GetGpu().Apply();    // 線のシェーダーはカメラ（u_scene）を読む
            bgfx::setVertexBuffer(0, &vertexBuffer);
            ApplyClipRect(cmd);
            bgfx::setState(kState);
            bgfx::submit(view, m_lineProgram);

            CountDrawCall(cmd.pass, stats);
            first += count;
            remaining -= count;
        }
    }

    //------------------------------------------------------------------------
    //! コマンドに切り取りがあれば、次の Submit にシザーを設定します。
    //------------------------------------------------------------------------
    void DrawCommandExecutor::ApplyClipRect(const DrawCommand& cmd) {
        if(!cmd.hasClipRect)
            return;

        const i32 left   = std::max(cmd.clipRect.left, 0);
        const i32 top    = std::max(cmd.clipRect.top, 0);
        const i32 width  = std::max(cmd.clipRect.right - left, 0);
        const i32 height = std::max(cmd.clipRect.bottom - top, 0);
        bgfx::setScissor(static_cast<u16>(left), static_cast<u16>(top), static_cast<u16>(width), static_cast<u16>(height));
    }

    //------------------------------------------------------------------------
    //! パスごとの描画数を統計へ加えます。
    //------------------------------------------------------------------------
    void DrawCommandExecutor::CountDrawCall(RenderPass pass, Renderer::FrameStats& stats) {
        switch(pass) {
        case RenderPass::GBuffer:          ++stats.gbufferDrawCalls; break;
        case RenderPass::World:            ++stats.worldDrawCalls; break;
        case RenderPass::TransparentDepth: ++stats.transparentDrawCalls; break;
        case RenderPass::Transparent:      ++stats.transparentDrawCalls; break;
        case RenderPass::Overlay:          ++stats.overlayDrawCalls; break;
        }
    }

}    // namespace Tsukino::Renderer
