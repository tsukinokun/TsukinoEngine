//----------------------------------------------------------------------------
//! @file   EfkModelRenderer.cpp
//! @brief  Effekseer のモデルのレンダラーの bgfx 版の実装（Effekseer の DX11 版の ModelRenderer と同じ作り）
//----------------------------------------------------------------------------
#include "EfkRenderer.hpp"

#include <Tsukino/Core/Log.hpp>

#include <EffekseerRendererCommon/EffekseerRenderer.ModelRendererBase.h>
#include <EffekseerRendererCommon/EffekseerRenderer.Renderer_Impl.h>

// 名前空間 : Tsukino::Renderer::EffekseerBgfx
namespace Tsukino::Renderer::EffekseerBgfx {

    namespace {
        constexpr int32_t kCount = ModelRenderer::kModelInstanceCount;

        //! モデルの頂点の定数の vec4 の数（Shaders/bgfx/Effekseer/EfkModelVS.sh と揃える）
        constexpr u16 kModelVSVec4Count   = static_cast<u16>(8 + 6 * kCount);
        constexpr u16 kModelAdVSVec4Count = static_cast<u16>(10 + 13 * kCount);
        //! ピクセルの定数の vec4 の数（スプライトと共用。EfkPS.sh・EfkDistortionPS.sh と揃える）
        constexpr u16 kPSVec4Count           = 18;
        constexpr u16 kDistortionPSVec4Count = 8;

        static_assert(sizeof(EffekseerRenderer::ModelRendererVertexConstantBuffer<kCount>) == kModelVSVec4Count * 16,
                      "ModelRendererVertexConstantBuffer must match u_efkModelVS in EfkModelVS.sh.");
        static_assert(sizeof(EffekseerRenderer::ModelRendererAdvancedVertexConstantBuffer<kCount>) == kModelAdVSVec4Count * 16,
                      "ModelRendererAdvancedVertexConstantBuffer must match u_efkModelAdVS in EfkModelVS.sh.");
    }    // namespace

    //------------------------------------------------------------------------
    //! モデルのレンダラーを作ります。
    //------------------------------------------------------------------------
    Effekseer::RefPtr<ModelRenderer> ModelRenderer::Create(const RendererImplementedRef& renderer) {
        const Effekseer::Backend::VertexLayoutRef layout = EffekseerRenderer::GetModelRendererVertexLayout(renderer->GetGraphicsDevice());

        // 並びは advanced の lit・unlit・歪み、通常の lit・unlit・歪み（EndRendering_ に渡す順）
        std::array<std::unique_ptr<Shader>, 6> shaders = {
            std::make_unique<Shader>("Effekseer/ModelAdLit.vs", "Effekseer/AdLit.ps", layout, "u_efkModelAdVS", kModelAdVSVec4Count, "u_efkPS", kPSVec4Count),
            std::make_unique<Shader>("Effekseer/ModelAdUnlit.vs", "Effekseer/AdUnlit.ps", layout, "u_efkModelAdVS", kModelAdVSVec4Count, "u_efkPS", kPSVec4Count),
            std::make_unique<Shader>("Effekseer/ModelAdDistortion.vs", "Effekseer/AdDistortion.ps", layout, "u_efkModelAdVS", kModelAdVSVec4Count, "u_efkDistPS",
                                     kDistortionPSVec4Count),
            std::make_unique<Shader>("Effekseer/ModelLit.vs", "Effekseer/Lit.ps", layout, "u_efkModelVS", kModelVSVec4Count, "u_efkPS", kPSVec4Count),
            std::make_unique<Shader>("Effekseer/ModelUnlit.vs", "Effekseer/Unlit.ps", layout, "u_efkModelVS", kModelVSVec4Count, "u_efkPS", kPSVec4Count),
            std::make_unique<Shader>("Effekseer/ModelDistortion.vs", "Effekseer/Distortion.ps", layout, "u_efkModelVS", kModelVSVec4Count, "u_efkDistPS",
                                     kDistortionPSVec4Count),
        };
        for(const auto& shader : shaders) {
            if(!shader->IsValid()) {
                Tsukino::Core::Log::Error("Effekseer (bgfx): The model shaders could not be loaded; model nodes are not drawn.");
                return nullptr;
            }
        }
        return Effekseer::RefPtr<ModelRenderer>(new ModelRenderer(renderer, std::move(shaders)));
    }

    //------------------------------------------------------------------------
    //! モデルのレンダラーを作ります（Create() から呼ぶ）。
    //------------------------------------------------------------------------
    ModelRenderer::ModelRenderer(const RendererImplementedRef& renderer, std::array<std::unique_ptr<Shader>, 6> shaders)
        : m_renderer(renderer)
        , m_shaders(std::move(shaders)) {
        // 共通部分が書く定数の大きさを確かめる（入れ物はシェーダーの配列の長さで作ってある）
        for(int i = 0; i < 3; ++i) {
            m_shaders[i]->SetVertexConstantBufferSize(sizeof(EffekseerRenderer::ModelRendererAdvancedVertexConstantBuffer<kCount>));
            m_shaders[i + 3]->SetVertexConstantBufferSize(sizeof(EffekseerRenderer::ModelRendererVertexConstantBuffer<kCount>));
        }
        VertexType = EffekseerRenderer::ModelRendererVertexType::Instancing;
    }

    //------------------------------------------------------------------------
    //! 描き始めます。
    //------------------------------------------------------------------------
    void ModelRenderer::BeginRendering(const NodeParameter& parameter, int32_t count, void* userData) {
        BeginRendering_(m_renderer.Get(), parameter, count, userData);
    }

    //------------------------------------------------------------------------
    //! 1つ分を積みます。
    //------------------------------------------------------------------------
    void ModelRenderer::Rendering(const NodeParameter& parameter, const InstanceParameter& instanceParameter, void* userData) {
        Rendering_<RendererImplemented>(m_renderer.Get(), parameter, instanceParameter, userData);
    }

    //------------------------------------------------------------------------
    //! 積んだ分を描きます。
    //------------------------------------------------------------------------
    void ModelRenderer::EndRendering(const NodeParameter& parameter, void* userData) {
        if(!parameter.IsExternalMode && parameter.ModelIndex < 0)
            return;

        Effekseer::ModelRef model = nullptr;
        if(parameter.IsExternalMode)
            model = parameter.ExternalModel;
        else if(parameter.IsProceduralMode)
            model = parameter.EffectPointer->GetProceduralModel(parameter.ModelIndex);
        else
            model = parameter.EffectPointer->GetModel(parameter.ModelIndex);
        if(model == nullptr)
            return;

        // モデルの頂点とインデックスを、描画デバイスのバッファへ移す（最初の1回だけ）
        model->StoreBufferToGPU(m_renderer->GetGraphicsDevice().Get());
        if(!model->GetIsBufferStoredOnGPU())
            return;

        if(m_renderer->GetRenderMode() == Effekseer::RenderMode::Wireframe) {
            model->GenerateWireIndexBuffer(m_renderer->GetGraphicsDevice().Get());
            if(!model->GetIsWireIndexBufferGenerated())
                return;
        }

        EndRendering_<RendererImplemented, Shader, Effekseer::Model, true, kCount>(m_renderer.Get(), m_shaders[0].get(), m_shaders[1].get(),
                                                                                   m_shaders[2].get(), m_shaders[3].get(), m_shaders[4].get(),
                                                                                   m_shaders[5].get(), parameter, userData);
    }

}    // namespace Tsukino::Renderer::EffekseerBgfx
