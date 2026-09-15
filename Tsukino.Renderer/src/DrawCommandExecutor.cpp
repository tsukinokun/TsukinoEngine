//----------------------------------------------------------------------------
//! @file   DrawCommandExecutor.cpp
//! @brief  描画コマンド1本を GPU へ流す処理の実装
//----------------------------------------------------------------------------
#include "DrawCommandExecutor.hpp"

#include <Tsukino/Renderer/ShaderSlots.hpp>

#include <Tsukino/Core/Log.hpp>

#include <cstring>

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {
    //------------------------------------------------------------------------
    //! 定数バッファを作成します。
    //------------------------------------------------------------------------
    bool DrawCommandExecutor::Initialize(GraphicsContext& graphicsContext, RenderResources& resources, FrameConstants& frameConstants) {
        m_graphicsContext = &graphicsContext;
        m_resources       = &resources;
        m_frameConstants  = &frameConstants;

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

        return true;
    }

    //------------------------------------------------------------------------
    //! 通常のパスの描画コマンドを実行します。
    //------------------------------------------------------------------------
    void DrawCommandExecutor::Execute(const DrawCommand& cmd, bool motionBlurEnabled, Renderer::FrameStats& stats) {
        ID3D11DeviceContext* context = m_graphicsContext->GetContext();

        //--------------------------------------------------------------------
        // カスタム描画（フォント等）がある場合
        //--------------------------------------------------------------------
        if(cmd.customDraw) {
            // スロットをクリア
            ID3D11Buffer* nullBuffers[] = {nullptr, nullptr};
            UINT          strides[]     = {0, 0};
            UINT          offsets[]     = {0, 0};
            context->IASetVertexBuffers(0, 2, nullBuffers, strides, offsets);

            // カスタム描画実行
            cmd.customDraw(context);

            // SpriteBatchで汚されたステートをリセット
            // これを入れないとSpriteの後の描画が真っ暗になったり崩れる
            DirectX::CommonStates* states = m_resources->GetCommonStatesTK();
            context->OMSetBlendState(states->Opaque(), nullptr, 0xFFFFFFFF);
            context->OMSetDepthStencilState(states->DepthDefault(), 0);
            context->RSSetState(states->CullNone());

            // s0をLinearWrapに戻す（SpriteBatch汚染対策）
            ID3D11SamplerState* linearWrap = m_resources->GetSampler(Tsukino::GraphicsCommon::SamplerType::LinearWrap);
            context->PSSetSamplers(static_cast<UINT>(SamplerSlot::Material), 1, &linearWrap);
            return;
        }

        // 無効なコマンドは何もしない
        if(!cmd.material || !cmd.mesh)
            return;

        // Scene (b0) を毎回再バインド（ステート汚染対策）
        context->VSSetConstantBuffers(static_cast<UINT>(CBSlot::Scene), 1, m_frameConstants->GetSceneBufferAddress());

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
        cb.motionFlags = hlslpp::float4(writeVelocity ? 1.0f : 0.0f, 0.0f, 0.0f, 0.0f);
        context->UpdateSubresource(m_objectBuffer.Get(), 0, nullptr, &cb, 0, 0);
        context->VSSetConstantBuffers(static_cast<UINT>(CBSlot::Transform), 1, m_objectBuffer.GetAddressOf());

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
        m_graphicsContext->SetMaterial(*cmd.material);

        if(cmd.materialData) {
            context->UpdateSubresource(m_materialBuffer.Get(), 0, nullptr, cmd.materialData, 0, 0);
            context->PSSetConstantBuffers(static_cast<UINT>(CBSlot::Material), 1, m_materialBuffer.GetAddressOf());
        }

        BindMesh(*cmd.mesh, isSkeletal);

        //--------------------------------------------------------------------
        // インスタンスごとのデータを頂点シェーダーへバインドする。
        // 使わないコマンドが直前のSRVを引き継がないよう、無いときは明示的に外す
        //--------------------------------------------------------------------
        BindInstanceData(cmd.instanceData);

        // ゲームが自前のシェーダーで描くときのパラメータ。使わないコマンドでも
        // 明示的に空を書き、直前のコマンドのバッファを引き継がせない
        BindUserConstantBuffer(cmd.userConstantBuffer, cmd.userConstantSlot);

        Draw(cmd);

        //--------------------------------------------------------------------
        // 統計の加算（負荷調査用）
        //--------------------------------------------------------------------
        switch(cmd.pass) {
        case RenderPass::GBuffer:          ++stats.gbufferDrawCalls; break;
        case RenderPass::World:            ++stats.worldDrawCalls; break;
        case RenderPass::TransparentDepth: ++stats.transparentDrawCalls; break;
        case RenderPass::Transparent:      ++stats.transparentDrawCalls; break;
        case RenderPass::Overlay:          ++stats.overlayDrawCalls; break;
        }
        stats.triangleCount += (cmd.mesh->indexCount * cmd.instanceCount) / 3;

        if(isSkeletal) {
            ++stats.skinnedDrawCalls;
            stats.boneBytesUploaded += boneBytesThis;
        }
    }

    //------------------------------------------------------------------------
    //! シャドウマップへ深度を書き込む描画コマンドを実行します。
    //------------------------------------------------------------------------
    void DrawCommandExecutor::ExecuteShadow(const DrawCommand&    cmd,
                                            const PipelineState*  staticPipeline,
                                            const PipelineState*  skeletalPipeline,
                                            Renderer::FrameStats& stats) {
        if(!cmd.mesh)
            return;

        ID3D11DeviceContext* context = m_graphicsContext->GetContext();

        const bool isSkeletal = cmd.boneMatrices && cmd.boneCount > 0;

        // シャドウ用パイプラインをセット
        const PipelineState* pipeline = isSkeletal ? skeletalPipeline : staticPipeline;
        if(!pipeline)
            return;

        m_graphicsContext->SetPipelineState(*pipeline);

        // Scene (b0) を再バインド
        context->VSSetConstantBuffers(static_cast<UINT>(CBSlot::Scene), 1, m_frameConstants->GetSceneBufferAddress());

        // Transform (b1)
        CBufferTransform cb{};
        cb.world = cmd.transform;
        context->UpdateSubresource(m_objectBuffer.Get(), 0, nullptr, &cb, 0, 0);
        context->VSSetConstantBuffers(static_cast<UINT>(CBSlot::Transform), 1, m_objectBuffer.GetAddressOf());

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
        BindInstanceData(cmd.instanceData);

        // ゲームが自前のシェーダーで描くときのパラメータ。使わないコマンドでも
        // 明示的に空を書き、直前のコマンドのバッファを引き継がせない
        BindUserConstantBuffer(cmd.userConstantBuffer, cmd.userConstantSlot);

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

        if(isSkeletal && mesh.boneWeightBuffer.Get() != nullptr) {
            // ボーンあり：スロット0と1をバインド
            ID3D11Buffer* vbs[]     = {mesh.vertexBuffer.Get(), mesh.boneWeightBuffer.Get()};
            UINT          strides[] = {mesh.stride, sizeof(Tsukino::GraphicsCommon::BoneWeight)};
            UINT          offsets[] = {0, 0};
            context->IASetVertexBuffers(0, 2, vbs, strides, offsets);
        } else {
            // ボーンなし：スロット0のみバインドし、スロット1は必ず明示的にクリアする。
            // クリアしないと直前に描いたスキンメッシュのボーンウェイトが残る
            ID3D11Buffer* vbs[]     = {mesh.vertexBuffer.Get(), nullptr};
            UINT          strides[] = {mesh.stride, 0};
            UINT          offsets[] = {0, 0};
            context->IASetVertexBuffers(0, 2, vbs, strides, offsets);
        }

        context->IASetIndexBuffer(mesh.indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
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
