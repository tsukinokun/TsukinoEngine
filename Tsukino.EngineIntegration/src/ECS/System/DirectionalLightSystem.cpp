//--------------------------------------------------------------
//! @file   DirectionalLightSystem.cpp
//! @brief  ディレクショナルライトシステムの実装
//! @author 山﨑愛
//--------------------------------------------------------------
#include <Tsukino/EngineIntegration/ECS/System/DirectionalLightSystem.hpp>
#include <Tsukino/EngineIntegration/EngineContext.hpp>
#include <Tsukino/BuiltIn/BuiltInAssets.hpp>
#include <Tsukino/BuiltIn/ECS/Component/DirectionalLightComponent.hpp>
#include <Tsukino/Engine/Asset/AssetManager.hpp>
#include <Tsukino/Engine/Asset/Shader/ShaderAsset.hpp>
#include <Tsukino/Renderer/Renderer.hpp>

#include <entt/entt.hpp>

// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {
    //--------------------------------------------------------------
    //! @brief システムの更新
    //--------------------------------------------------------------
    void DirectionalLightSystem::Update(Tsukino::ECS::Registry& registry, float deltaTime) {
        Tsukino::EngineIntegration::EngineContext* ctx = registry.GetContext<Tsukino::EngineIntegration::EngineContext*>();
        if(!ctx || !ctx->renderer)
            return;

        //--------------------------------------------------------------
        // 初回のみシャドウ用パイプラインを構築
        //--------------------------------------------------------------
        if(!m_pipelineInitialized) {
            auto vsStaticAsset   = std::static_pointer_cast<Tsukino::Asset::ShaderAsset>(ctx->assetManager->Get(ctx->builtinAssets->shaders.shadowStaticVS));
            auto vsSkeletalAsset = std::static_pointer_cast<Tsukino::Asset::ShaderAsset>(ctx->assetManager->Get(ctx->builtinAssets->shaders.shadowVS));
            auto psAsset         = std::static_pointer_cast<Tsukino::Asset::ShaderAsset>(ctx->assetManager->Get(ctx->builtinAssets->shaders.shadowPS));

            if(vsStaticAsset && vsSkeletalAsset && psAsset) {
                //--------------------------------------------------------------
                // スタティック用入力レイアウト
                //--------------------------------------------------------------
                D3D11_INPUT_ELEMENT_DESC staticLayout[] = {
                    {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
                    {"NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
                    {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
                };
                auto staticPipeline = ctx->renderer->GetPipelineFactory()->Create(
                    *vsStaticAsset, *psAsset, staticLayout, ARRAYSIZE(staticLayout), Tsukino::Renderer::DepthMode::ReadWrite);

                //--------------------------------------------------------------
                // スケルタル用入力レイアウト
                //--------------------------------------------------------------
                D3D11_INPUT_ELEMENT_DESC skeletalLayout[] = {
                    {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
                    {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
                    {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
                    {"BONE_INDICES",
                     0, DXGI_FORMAT_R32G32B32A32_UINT,
                     1, offsetof(Tsukino::GraphicsCommon::BoneWeight, boneIndices),
                     D3D11_INPUT_PER_VERTEX_DATA, 0},
                    {"BONE_WEIGHTS",
                     0, DXGI_FORMAT_R32G32B32A32_FLOAT,
                     1, offsetof(Tsukino::GraphicsCommon::BoneWeight, weights),
                     D3D11_INPUT_PER_VERTEX_DATA, 0},
                };
                auto skeletalPipeline = ctx->renderer->GetPipelineFactory()->Create(
                    *vsSkeletalAsset, *psAsset, skeletalLayout, ARRAYSIZE(skeletalLayout), Tsukino::Renderer::DepthMode::ReadWrite);

                ctx->renderer->SetShadowPipeline(staticPipeline, skeletalPipeline);
                m_pipelineInitialized = true;
            }
        }

        //--------------------------------------------------------------
        // DirectionalLightComponentを持つエンティティを探す
        // 複数あった場合は最初の1つだけ使用する
        //--------------------------------------------------------------
        auto view = registry.View<DirectionalLightComponent>();
        view.each([&](entt::entity entity, const DirectionalLightComponent& light) {
            if(!light.castShadow)
                return;

            ctx->renderer->SetDirectionalLight(light.direction, light.color, light.intensity);
        });
    }

}    // namespace Tsukino::BuiltIn::ECS
