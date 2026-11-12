//----------------------------------------------------------------------------
//! @file   DrawCommandExecutor.cpp
//! @brief  描画コマンド1本を GPU へ流す処理の実装
//----------------------------------------------------------------------------
#include "DrawCommandExecutor.hpp"
#include <Tsukino/Renderer/DX11/PipelineState.hpp>
#include <Tsukino/Renderer/DX11/MeshBufferGpu.hpp>
#include <Tsukino/Renderer/DX11/GraphicsContext.hpp>
#include <Tsukino/Renderer/DX11/FrameConstantsGpu.hpp>
#include <Tsukino/Renderer/DX11/RenderResourcesGpu.hpp>

#include <Tsukino/Renderer/ShaderSlots.hpp>

#include <Tsukino/Engine/Asset/Shader/ShaderAsset.hpp>

#include <Tsukino/Core/Log.hpp>

#include <CommonStates.h>

#include <algorithm>
#include <cstddef>
#include <cstring>

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {
    //------------------------------------------------------------------------
    //! 定数バッファと、スプライト・線を描く資源を作成します。
    //------------------------------------------------------------------------
    bool DrawCommandExecutor::Initialize(GraphicsContext&                   graphicsContext,
                                         RenderResources&                   resources,
                                         FrameConstants&                    frameConstants,
                                         const DrawCommandQueue&            drawQueue,
                                         const Tsukino::Asset::ShaderAsset* lineVS,
                                         const Tsukino::Asset::ShaderAsset* linePS) {
        m_graphicsContext = &graphicsContext;
        m_resources       = &resources;
        m_frameConstants  = &frameConstants;
        m_drawQueue       = &drawQueue;

        ID3D11Device* device = graphicsContext.GetDevice();

        D3D11_BUFFER_DESC desc = {};
        desc.Usage             = D3D11_USAGE_DEFAULT;
        desc.BindFlags         = D3D11_BIND_CONSTANT_BUFFER;

        // Transform (b1)
        desc.ByteWidth = sizeof(CBufferTransform);
        if(FAILED(device->CreateBuffer(&desc, nullptr, m_objectBuffer.GetAddressOf()))) {
            Tsukino::Core::Log::Error("Failed to create object constant buffer.");
            return false;
        }

        // マテリアル (b2)
        desc.ByteWidth = sizeof(CBufferMaterial);
        if(FAILED(device->CreateBuffer(&desc, nullptr, m_materialBuffer.GetAddressOf()))) {
            Tsukino::Core::Log::Error("Failed to create material constant buffer.");
            return false;
        }

        //--------------------------------------------------------------------
        // ボーン行列 (b3) と前フレームのボーン行列 (b7)。
        // ボーン行列は1ドローごとに書き換えるうえ1本あたり8KBと大きいため、
        // DEFAULT+UpdateSubresourceではなくDYNAMIC+Map(WRITE_DISCARD)で更新する。
        // 実ボーン数ぶんだけ書けるようになり、転送量とCPU側のゼロ初期化が消える
        //--------------------------------------------------------------------
        desc.Usage          = D3D11_USAGE_DYNAMIC;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        desc.ByteWidth = sizeof(CBufferSkinning);
        if(FAILED(device->CreateBuffer(&desc, nullptr, m_skinningBuffer.GetAddressOf()))) {
            Tsukino::Core::Log::Error("Failed to create skinning constant buffer.");
            return false;
        }

        desc.ByteWidth = sizeof(CBufferSkinningPrev);
        if(FAILED(device->CreateBuffer(&desc, nullptr, m_prevSkinningBuffer.GetAddressOf()))) {
            Tsukino::Core::Log::Error("Failed to create previous frame skinning constant buffer.");
            return false;
        }

        //--------------------------------------------------------------------
        // スプライト（DrawKind::Sprites）を描く SpriteBatch
        //--------------------------------------------------------------------
        m_spriteBatch = resources.GetGpu().CreateSpriteBatch();

        //--------------------------------------------------------------------
        // 線（DrawKind::Lines）を描くシェーダー・入力レイアウト・動的頂点バッファ
        //--------------------------------------------------------------------
        if(!lineVS || !linePS) {
            Tsukino::Core::Log::Error("Line shader assets are null.");
            return false;
        }

        if(FAILED(device->CreateVertexShader(lineVS->binary.data(), lineVS->binary.size(), nullptr, m_lineVS.GetAddressOf()))) {
            Tsukino::Core::Log::Error("Failed to create line vertex shader.");
            return false;
        }

        if(FAILED(device->CreatePixelShader(linePS->binary.data(), linePS->binary.size(), nullptr, m_linePS.GetAddressOf()))) {
            Tsukino::Core::Log::Error("Failed to create line pixel shader.");
            return false;
        }

        using Tsukino::GraphicsCommon::DebugVertex;
        const D3D11_INPUT_ELEMENT_DESC layout[] = {
            {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, offsetof(DebugVertex, position), D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offsetof(DebugVertex, color),    D3D11_INPUT_PER_VERTEX_DATA, 0},
        };
        if(FAILED(device->CreateInputLayout(layout, ARRAYSIZE(layout), lineVS->binary.data(), lineVS->binary.size(),
                                            m_lineInputLayout.GetAddressOf()))) {
            Tsukino::Core::Log::Error("Failed to create line input layout.");
            return false;
        }

        D3D11_BUFFER_DESC lineDesc{};
        lineDesc.Usage          = D3D11_USAGE_DYNAMIC;
        lineDesc.ByteWidth      = static_cast<UINT>(sizeof(DebugVertex) * kMaxLineVertices);
        lineDesc.BindFlags      = D3D11_BIND_VERTEX_BUFFER;
        lineDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        if(FAILED(device->CreateBuffer(&lineDesc, nullptr, m_lineVB.GetAddressOf()))) {
            Tsukino::Core::Log::Error("Failed to create line vertex buffer.");
            return false;
        }

        return true;
    }

    //------------------------------------------------------------------------
    //! 通常のパスの描画コマンドを実行します。
    //------------------------------------------------------------------------
    void DrawCommandExecutor::Execute(const DrawCommand& cmd, bool motionBlurEnabled, Renderer::FrameStats& stats) {
        ID3D11DeviceContext* context = m_graphicsContext->GetContext();

        //--------------------------------------------------------------------
        // メッシュ以外（スプライトの並び・線の頂点の列）
        //--------------------------------------------------------------------
        switch(cmd.kind) {
        case DrawKind::Sprites: ExecuteSprites(cmd, stats); return;
        case DrawKind::Lines:   ExecuteLines(cmd, stats); return;
        case DrawKind::Mesh:    break;
        }

        // 無効なコマンドは何もしない
        if(!cmd.material || !cmd.mesh)
            return;

        // Scene (b0) を毎回再バインド（ステート汚染対策）
        context->VSSetConstantBuffers(static_cast<UINT>(CBSlot::Scene), 1, m_frameConstants->GetGpu().GetSceneBufferAddress());

        //--------------------------------------------------------------------
        // Transform を定数バッファに書き込む。
        // モーションブラーが有効で、かつこのオブジェクトに前フレームの
        // データがあるときだけ速度を出す。それ以外は motionFlags.x = 0 に
        // して、VS側で prevClip = curClip（＝速度ゼロ）へ短絡させる
        //--------------------------------------------------------------------
        const bool writeVelocity = motionBlurEnabled && cmd.hasPrevFrame;

        CBufferTransform cb{};
        cb.world       = cmd.transform;
        cb.prevWorld   = writeVelocity ? cmd.prevTransform : cmd.transform;
        cb.motionFlags = hlslpp::float4(writeVelocity ? 1.0f : 0.0f, cmd.ditherFade, cmd.occlusionCutout ? 1.0f : 0.0f, 0.0f);
        context->UpdateSubresource(m_objectBuffer.Get(), 0, nullptr, &cb, 0, 0);
        context->VSSetConstantBuffers(static_cast<UINT>(CBSlot::Transform), 1, m_objectBuffer.GetAddressOf());
        // 網点フェード（motionFlags.y）はピクセルシェーダーが読むので、PS にも同じものを束ねる
        context->PSSetConstantBuffers(static_cast<UINT>(CBSlot::Transform), 1, m_objectBuffer.GetAddressOf());

        //--------------------------------------------------------------------
        // ボーン行列 (b3) の適用
        //--------------------------------------------------------------------
        const bool isSkeletal     = cmd.boneMatrices && cmd.boneCount > 0;
        u32        boneBytesThis  = 0;

        if(isSkeletal) {
            boneBytesThis = UploadBoneMatrices(m_skinningBuffer.Get(), cmd.boneMatrices, cmd.boneCount);
            context->VSSetConstantBuffers(static_cast<UINT>(CBSlot::Skinning), 1, m_skinningBuffer.GetAddressOf());

            //----------------------------------------------------------------
            // 前フレームのボーン行列 (b7) の適用。
            // スキン1体あたり8KBの転送になるため、速度を出さないときは
            // 転送もバインドも行わない
            //----------------------------------------------------------------
            if(writeVelocity && cmd.prevBoneMatrices) {
                constexpr UINT prevSkinSlot = static_cast<UINT>(CBSlot::SkinningPrev);
                boneBytesThis += UploadBoneMatrices(m_prevSkinningBuffer.Get(), cmd.prevBoneMatrices, cmd.boneCount);
                context->VSSetConstantBuffers(prevSkinSlot, 1, m_prevSkinningBuffer.GetAddressOf());
            }
        } else {
            // スキニングを使わないオブジェクトは、スロット3を nullptr でクリアして
            // 前のオブジェクトのボーン行列が残らないようにする
            ID3D11Buffer* nullBuffer = nullptr;
            context->VSSetConstantBuffers(static_cast<UINT>(CBSlot::Skinning), 1, &nullBuffer);
        }

        //--------------------------------------------------------------------
        // Material を適用
        //--------------------------------------------------------------------
        m_graphicsContext->SetMaterial(*cmd.material, *m_resources);

        //--------------------------------------------------------------------
        // 切り取り。SetMaterial がパイプラインのラスタライザを毎回設定し直すので、
        // ここで差し替えても次のコマンドへは持ち越されない
        //--------------------------------------------------------------------
        if(cmd.hasClipRect) {
            context->RSSetState(ApplyClipRect(cmd, nullptr));
        }

        if(cmd.materialData) {
            context->UpdateSubresource(m_materialBuffer.Get(), 0, nullptr, cmd.materialData, 0, 0);
            context->PSSetConstantBuffers(static_cast<UINT>(CBSlot::Material), 1, m_materialBuffer.GetAddressOf());
        }

        BindMesh(*cmd.mesh, isSkeletal);

        //--------------------------------------------------------------------
        // インスタンスごとのデータを頂点シェーダーへバインドする。
        // 使わないコマンドが直前のSRVを引き継がないよう、無いときは明示的に外す
        //--------------------------------------------------------------------
        BindInstanceData(m_resources->GetGpu().ResolveInstanceBuffer(cmd.instanceData));

        // ゲームが自前のシェーダーで描くときのパラメータ。使わないコマンドでも
        // 明示的に空を書き、直前のコマンドのバッファを引き継がせない
        BindUserConstantBuffer(m_resources->GetGpu().ResolveUserConstantBuffer(cmd.userConstantBuffer), cmd.userConstantSlot);

        Draw(cmd);

        //--------------------------------------------------------------------
        // 統計の加算（負荷調査用）
        //--------------------------------------------------------------------
        CountDrawCall(cmd.pass, stats);
        stats.triangleCount += (cmd.mesh->indexCount * cmd.instanceCount) / 3;

        if(isSkeletal) {
            ++stats.skinnedDrawCalls;
            stats.boneBytesUploaded += boneBytesThis;
        }
    }

    //------------------------------------------------------------------------
    //! DrawKind::Sprites のコマンドを描きます。
    //------------------------------------------------------------------------
    void DrawCommandExecutor::ExecuteSprites(const DrawCommand& cmd, Renderer::FrameStats& stats) {
        const std::vector<SpriteQuad>& sprites = m_drawQueue->GetSprites();
        if(!m_spriteBatch || cmd.batchCount == 0 || static_cast<size_t>(cmd.batchFirst) + cmd.batchCount > sprites.size())
            return;

        ID3D11DeviceContext*   context = m_graphicsContext->GetContext();
        DirectX::CommonStates* states  = m_resources->GetGpu().GetCommonStatesTK();

        // 直前のメッシュが張った頂点バッファ（スロット1のボーンウェイトなど）を外す
        ID3D11Buffer* nullBuffers[] = {nullptr, nullptr};
        UINT          strides[]     = {0, 0};
        UINT          offsets[]     = {0, 0};
        context->IASetVertexBuffers(0, 2, nullBuffers, strides, offsets);

        //--------------------------------------------------------------------
        // 切り取りがあればシザー有効のラスタライザで描く。シザー矩形は SpriteBatch が
        // 触らないので、Begin の前に設定しておけば End の描画に効く。
        // ラスタライザが nullptr なら SpriteBatch の既定（カリングなし）
        //--------------------------------------------------------------------
        ID3D11RasterizerState* rasterizer = ApplyClipRect(cmd, nullptr);

        m_spriteBatch->Begin(DirectX::SpriteSortMode_Deferred,    // 積んだ順に描く
                             states->NonPremultiplied(),          // straight alpha のブレンド
                             nullptr,                             // サンプラーは SpriteBatch の既定（LinearClamp）
                             states->DepthRead(),                 // 奥行きは読むだけで書かない
                             rasterizer);

        const u32 end = cmd.batchFirst + cmd.batchCount;
        for(u32 index = cmd.batchFirst; index < end; ++index) {
            const SpriteQuad&         sprite  = sprites[index];
            ID3D11ShaderResourceView* texture = m_resources->GetGpu().ResolveTexture(sprite.texture);
            if(!texture)
                continue;

            const RECT              source = {sprite.source.left, sprite.source.top, sprite.source.right, sprite.source.bottom};
            const DirectX::XMVECTOR color  = DirectX::XMVectorSet(sprite.color[0], sprite.color[1], sprite.color[2], sprite.color[3]);
            m_spriteBatch->Draw(texture,
                                DirectX::XMFLOAT2(sprite.x, sprite.y),
                                &source,
                                color,
                                sprite.rotation,
                                DirectX::XMFLOAT2(sprite.originX, sprite.originY),
                                DirectX::XMFLOAT2(sprite.scaleX, sprite.scaleY));
        }

        m_spriteBatch->End();

        RestoreStateAfterBatch();

        // 統計の加算（負荷調査用）。SpriteBatch はまとめて描くので、1コマンドを1回と数える
        CountDrawCall(cmd.pass, stats);
        stats.triangleCount += cmd.batchCount * 2;
    }

    //------------------------------------------------------------------------
    //! DrawKind::Lines のコマンドを描きます。
    //------------------------------------------------------------------------
    void DrawCommandExecutor::ExecuteLines(const DrawCommand& cmd, Renderer::FrameStats& stats) {
        using Tsukino::GraphicsCommon::DebugVertex;

        const std::vector<DebugVertex>& vertices = m_drawQueue->GetLineVertices();
        if(!m_lineVB || cmd.batchCount < 2 || static_cast<size_t>(cmd.batchFirst) + cmd.batchCount > vertices.size())
            return;

        ID3D11DeviceContext*   context = m_graphicsContext->GetContext();
        DirectX::CommonStates* states  = m_resources->GetGpu().GetCommonStatesTK();

        // 線のシェーダーはカメラ (b0) を読む。パスが転送したシーン定数をそのまま使う
        context->VSSetConstantBuffers(static_cast<UINT>(CBSlot::Scene), 1, m_frameConstants->GetGpu().GetSceneBufferAddress());

        context->VSSetShader(m_lineVS.Get(), nullptr, 0);
        context->PSSetShader(m_linePS.Get(), nullptr, 0);
        context->IASetInputLayout(m_lineInputLayout.Get());

        // 不透明・深度テストと書き込みあり（リバースZ）・カリングなし
        context->OMSetBlendState(states->Opaque(), nullptr, 0xFFFFFFFF);
        context->OMSetDepthStencilState(states->DepthReverseZ(), 0);
        context->RSSetState(ApplyClipRect(cmd, states->CullNone()));
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

        //--------------------------------------------------------------------
        // 頂点バッファに入りきらない分は、何回かに分けて描く
        //--------------------------------------------------------------------
        constexpr UINT stride    = sizeof(DebugVertex);
        u32            remaining = cmd.batchCount & ~1u;
        u32            first     = cmd.batchFirst;
        while(remaining > 0) {
            const u32 count = std::min(remaining, kMaxLineVertices);

            D3D11_MAPPED_SUBRESOURCE mapped{};
            if(FAILED(context->Map(m_lineVB.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
                break;
            std::memcpy(mapped.pData, vertices.data() + first, static_cast<size_t>(count) * stride);
            context->Unmap(m_lineVB.Get(), 0);

            // スロット1は直前のメッシュのボーンウェイトが残らないよう空にする
            ID3D11Buffer* vbs[]     = {m_lineVB.Get(), nullptr};
            UINT          strides[] = {stride, 0};
            UINT          offsets[] = {0, 0};
            context->IASetVertexBuffers(0, 2, vbs, strides, offsets);
            context->Draw(count, 0);

            CountDrawCall(cmd.pass, stats);
            first += count;
            remaining -= count;
        }

        RestoreStateAfterBatch();
    }

    //------------------------------------------------------------------------
    //! コマンドの切り取りを設定し、使うラスタライザを返します。
    //------------------------------------------------------------------------
    ID3D11RasterizerState* DrawCommandExecutor::ApplyClipRect(const DrawCommand& cmd, ID3D11RasterizerState* fallback) {
        if(!cmd.hasClipRect)
            return fallback;

        const D3D11_RECT scissor = {cmd.clipRect.left, cmd.clipRect.top, cmd.clipRect.right, cmd.clipRect.bottom};
        m_graphicsContext->GetContext()->RSSetScissorRects(1, &scissor);
        return m_resources->GetGpu().GetScissorRasterizerState();
    }

    //------------------------------------------------------------------------
    //! スプライト・線を描いた後に、メッシュの描画やパスが前提とするステートへ戻します。
    //------------------------------------------------------------------------
    void DrawCommandExecutor::RestoreStateAfterBatch() {
        ID3D11DeviceContext*   context = m_graphicsContext->GetContext();
        DirectX::CommonStates* states  = m_resources->GetGpu().GetCommonStatesTK();

        context->OMSetBlendState(states->Opaque(), nullptr, 0xFFFFFFFF);
        context->OMSetDepthStencilState(states->DepthDefault(), 0);
        context->RSSetState(states->CullNone());

        // s0 を LinearWrap に戻す（SpriteBatch が LinearClamp を張ったまま返るため）
        ID3D11SamplerState* linearWrap = m_resources->GetGpu().GetSampler(Tsukino::GraphicsCommon::SamplerType::LinearWrap);
        context->PSSetSamplers(static_cast<UINT>(SamplerSlot::Material), 1, &linearWrap);
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

    //------------------------------------------------------------------------
    //! シャドウマップへ深度を書き込む描画コマンドを実行します。
    //------------------------------------------------------------------------
    void DrawCommandExecutor::ExecuteShadow(const DrawCommand&    cmd,
                                            const PipelineState*  staticPipeline,
                                            const PipelineState*  skeletalPipeline,
                                            Renderer::FrameStats& stats) {
        // 影を落とすのはメッシュだけ（スプライト・線は形を持たない）
        if(cmd.kind != DrawKind::Mesh || !cmd.mesh)
            return;

        ID3D11DeviceContext* context = m_graphicsContext->GetContext();

        const bool isSkeletal = cmd.boneMatrices && cmd.boneCount > 0;

        // シャドウ用パイプラインをセット。コマンドが自前の影用（頂点を動かす物）を持っていればそれを使う
        const PipelineState* pipeline = cmd.shadowPipeline ? cmd.shadowPipeline : (isSkeletal ? skeletalPipeline : staticPipeline);
        if(!pipeline)
            return;

        m_graphicsContext->SetPipelineState(*pipeline);

        // Scene (b0) を再バインド
        context->VSSetConstantBuffers(static_cast<UINT>(CBSlot::Scene), 1, m_frameConstants->GetGpu().GetSceneBufferAddress());

        // Transform (b1)。網点フェード（motionFlags.y）はピクセルシェーダーが読むので PS にも束ねる
        CBufferTransform cb{};
        cb.world       = cmd.transform;
        cb.motionFlags = hlslpp::float4(0.0f, cmd.ditherFade, 0.0f, 0.0f);
        context->UpdateSubresource(m_objectBuffer.Get(), 0, nullptr, &cb, 0, 0);
        context->VSSetConstantBuffers(static_cast<UINT>(CBSlot::Transform), 1, m_objectBuffer.GetAddressOf());
        context->PSSetConstantBuffers(static_cast<UINT>(CBSlot::Transform), 1, m_objectBuffer.GetAddressOf());

        //--------------------------------------------------------------------
        // くり抜き（アルファテスト）の影。葉のカードのように、アルベドのアルファで
        // 形を抜いたものが四角い影を落とさないよう、影のピクセルシェーダーにも
        // アルベド (t0)・サンプラー (s0)・マテリアル定数 (b2) を渡す。
        // マテリアル定数の無いコマンドは alphaCutoff = 0（くり抜かない）を渡して、
        // 直前のコマンドの値が残らないようにする
        //--------------------------------------------------------------------
        ID3D11ShaderResourceView* albedo  = cmd.material ? m_resources->GetGpu().ResolveTexture(cmd.material->GetTexture()) : nullptr;
        ID3D11SamplerState*       sampler = nullptr;
        if(cmd.material && cmd.material->GetSampler())
            sampler = m_resources->GetGpu().GetSampler(*cmd.material->GetSampler());
        if(!albedo)
            albedo = m_resources->GetGpu().ResolveTexture(m_resources->GetWhiteTexture());
        if(!sampler)
            sampler = m_resources->GetGpu().GetSampler(Tsukino::GraphicsCommon::SamplerType::LinearWrap);
        context->PSSetShaderResources(static_cast<UINT>(SRVSlot::Albedo), 1, &albedo);
        context->PSSetSamplers(static_cast<UINT>(SamplerSlot::Material), 1, &sampler);

        static const CBufferMaterial kNoCutoutMaterial{};
        context->UpdateSubresource(m_materialBuffer.Get(), 0, nullptr, cmd.materialData ? cmd.materialData : &kNoCutoutMaterial, 0, 0);
        context->PSSetConstantBuffers(static_cast<UINT>(CBSlot::Material), 1, m_materialBuffer.GetAddressOf());

        // ボーン行列 (b3)
        u32 boneBytesThis = 0;
        if(isSkeletal) {
            boneBytesThis = UploadBoneMatrices(m_skinningBuffer.Get(), cmd.boneMatrices, cmd.boneCount);
            context->VSSetConstantBuffers(static_cast<UINT>(CBSlot::Skinning), 1, m_skinningBuffer.GetAddressOf());
        } else {
            ID3D11Buffer* nullBuffer = nullptr;
            context->VSSetConstantBuffers(static_cast<UINT>(CBSlot::Skinning), 1, &nullBuffer);
        }

        BindMesh(*cmd.mesh, isSkeletal);

        // インスタンスごとのデータのバインド。これが無いとインスタンス描画したオブジェクトが影を落とさない
        BindInstanceData(m_resources->GetGpu().ResolveInstanceBuffer(cmd.instanceData));

        // ゲームが自前のシェーダーで描くときのパラメータ。使わないコマンドでも
        // 明示的に空を書き、直前のコマンドのバッファを引き継がせない
        BindUserConstantBuffer(m_resources->GetGpu().ResolveUserConstantBuffer(cmd.userConstantBuffer), cmd.userConstantSlot);

        Draw(cmd);

        //--------------------------------------------------------------------
        // 統計の加算（負荷調査用）
        // GBufferと同じ形状をもう一度描いているので、ドロー数もボーン転送量も
        // ここで二重に計上されるのが実態どおり
        //--------------------------------------------------------------------
        ++stats.shadowDrawCalls;
        stats.triangleCount += (cmd.mesh->indexCount * cmd.instanceCount) / 3;
        if(isSkeletal) {
            ++stats.skinnedDrawCalls;
            stats.boneBytesUploaded += boneBytesThis;
        }
    }

    //------------------------------------------------------------------------
    //! ボーン行列を定数バッファへ転送します。
    //------------------------------------------------------------------------
    u32 DrawCommandExecutor::UploadBoneMatrices(ID3D11Buffer* buffer, const void* boneMatrices, u32 boneCount) {
        if(!buffer || !boneMatrices || boneCount == 0)
            return 0;

        // シェーダー側の宣言（float4x4 bones[128]）を超えて書かない
        const u32 copyCount = (boneCount < kMaxBoneCount) ? boneCount : kMaxBoneCount;
        const u32 byteCount = copyCount * static_cast<u32>(sizeof(hlslpp::float4x4));

        //--------------------------------------------------------------------
        // WRITE_DISCARDで新しい領域をもらい、実ボーン数ぶんだけ書く。
        //
        // 以前は8KBの構造体をスタックにゼロ初期化で作り、実ボーン数ぶんmemcpyしてから
        // UpdateSubresourceで8KB全体を転送していた。Mixamoのリグは65本程度なので、
        // ゼロ初期化と転送の半分以上が捨てられていたことになる。
        // スキンメッシュはShadowとGBufferの2パスで描かれるため、この無駄は2倍で効く
        //--------------------------------------------------------------------
        ID3D11DeviceContext*     context = m_graphicsContext->GetContext();
        D3D11_MAPPED_SUBRESOURCE mapped{};

        if(FAILED(context->Map(buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
            return 0;

        std::memcpy(mapped.pData, boneMatrices, byteCount);
        context->Unmap(buffer, 0);

        return byteCount;
    }

    //------------------------------------------------------------------------
    //! インスタンスごとのデータを頂点シェーダーへバインドします。
    //------------------------------------------------------------------------
    void DrawCommandExecutor::BindInstanceData(ID3D11ShaderResourceView* srv) {
        constexpr UINT slot = static_cast<UINT>(SRVSlot::InstanceData);

        // nullptrのときも「空を書き込む」のが肝。ここを素通りさせると、
        // 直前のインスタンス描画が残したSRVを次のコマンドが読んでしまう
        ID3D11ShaderResourceView* views[] = {srv};
        m_graphicsContext->GetContext()->VSSetShaderResources(slot, 1, views);
    }

    //------------------------------------------------------------------------
    //! ゲーム定義の定数バッファをバインドします。
    //------------------------------------------------------------------------
    void DrawCommandExecutor::BindUserConstantBuffer(ID3D11Buffer* buffer, CBSlot slot) {
        //--------------------------------------------------------------------
        // ゲーム予約枠（User0 / User1）以外を指定されたら何もしない。
        // エンジンが使う b0〜b9 を上書きされると描画が壊れるため、ここで弾いておく
        //--------------------------------------------------------------------
        if(slot != CBSlot::User0 && slot != CBSlot::User1)
            return;

        // BindInstanceDataと同じ理由で、nullptrのときも明示的に空を書く
        ID3D11Buffer* buffers[] = {buffer};

        ID3D11DeviceContext* context   = m_graphicsContext->GetContext();
        const UINT           slotIndex = static_cast<UINT>(slot);
        context->VSSetConstantBuffers(slotIndex, 1, buffers);
        context->PSSetConstantBuffers(slotIndex, 1, buffers);
    }

    //------------------------------------------------------------------------
    //! 頂点バッファとインデックスバッファをセットします。
    //------------------------------------------------------------------------
    void DrawCommandExecutor::BindMesh(const MeshBuffer& mesh, bool isSkeletal) {
        ID3D11DeviceContext* context = m_graphicsContext->GetContext();

        if(isSkeletal && mesh.gpu->boneWeightBuffer.Get() != nullptr) {
            // ボーンあり：スロット0と1をバインド
            ID3D11Buffer* vbs[]     = {mesh.gpu->vertexBuffer.Get(), mesh.gpu->boneWeightBuffer.Get()};
            UINT          strides[] = {mesh.stride, sizeof(Tsukino::GraphicsCommon::BoneWeight)};
            UINT          offsets[] = {0, 0};
            context->IASetVertexBuffers(0, 2, vbs, strides, offsets);
        } else {
            // ボーンなし：スロット0のみバインドし、スロット1は必ず明示的にクリアする。
            // クリアしないと直前に描いたスキンメッシュのボーンウェイトが残る
            ID3D11Buffer* vbs[]     = {mesh.gpu->vertexBuffer.Get(), nullptr};
            UINT          strides[] = {mesh.stride, 0};
            UINT          offsets[] = {0, 0};
            context->IASetVertexBuffers(0, 2, vbs, strides, offsets);
        }

        context->IASetIndexBuffer(mesh.gpu->indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    }

    //------------------------------------------------------------------------
    //! 描画します（インスタンス数に応じて DrawIndexed と DrawIndexedInstanced を切り替えます）。
    //------------------------------------------------------------------------
    void DrawCommandExecutor::Draw(const DrawCommand& cmd) {
        ID3D11DeviceContext* context = m_graphicsContext->GetContext();

        if(cmd.instanceCount > 1) {
            context->DrawIndexedInstanced(cmd.mesh->indexCount, cmd.instanceCount, 0, 0, 0);
        } else {
            // 既定値が1なので、既存の描画はすべてこちらを通り続ける
            context->DrawIndexed(cmd.mesh->indexCount, 0, 0);
        }
    }
}    // namespace Tsukino::Renderer
