//--------------------------------------------------------------
//! @file   BoneLookup.cpp
//! @brief  ボーン名からノード番号を引く関数の実装
//--------------------------------------------------------------
#include <Tsukino/EngineIntegration/ECS/BoneLookup.hpp>
#include <Tsukino/EngineIntegration/EngineContext.hpp>

#include <Tsukino/BuiltIn/ECS/Component/ModelComponent.hpp>
#include <Tsukino/Engine/Asset/AssetManager.hpp>
#include <Tsukino/Engine/Asset/Model/ModelAsset.hpp>
#include <Tsukino/GraphicsCommon/Model/ModelData.hpp>

#include <memory>
// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {
    //--------------------------------------------------------------
    //! @brief ボーン名からnodeIndexを解決する
    //--------------------------------------------------------------
    void ResolveBoneNodeIndex(Tsukino::EngineIntegration::EngineContext* ctx, Tsukino::ECS::Registry& registry, Tsukino::ECS::Entity modelEntity,
                              const std::string& boneName, Tsukino::Asset::AssetHandle& resolvedAgainstModel, Tsukino::u32& outNodeIndex) {
        if(!ctx || !ctx->assetManager || !registry.HasComponent<ModelComponent>(modelEntity))
            return;

        auto& model = registry.GetComponent<ModelComponent>(modelEntity);
        if(resolvedAgainstModel == model.modelHandle)
            return;    // 既に同じモデルに対して解決済み

        auto asset = ctx->assetManager->Get(model.modelHandle);
        if(!asset || asset->GetType() != Tsukino::Asset::AssetType::Model)
            return;

        auto modelAsset = std::static_pointer_cast<Tsukino::Asset::ModelAsset>(asset);
        outNodeIndex    = UINT32_MAX;
        for(Tsukino::u32 i = 0; i < modelAsset->modelData.nodes.size(); ++i) {
            if(modelAsset->modelData.nodes[i].name == boneName) {
                outNodeIndex = i;
                break;
            }
        }
        resolvedAgainstModel = model.modelHandle;    // アセットが読めた時点で確定（見つからなければUINT32_MAXのまま）
    }
}    // namespace Tsukino::BuiltIn::ECS
