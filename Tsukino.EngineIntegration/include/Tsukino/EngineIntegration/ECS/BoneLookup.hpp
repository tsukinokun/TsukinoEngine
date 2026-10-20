//--------------------------------------------------------------
//! @file   BoneLookup.hpp
//! @brief  ボーン名からノード番号を引く関数の宣言
//--------------------------------------------------------------
#pragma once
#include <Tsukino/Core/ECS/Entity/Entity.hpp>
#include <Tsukino/Core/ECS/Registry/Registry.hpp>
#include <Tsukino/Core/typedef.hpp>
#include <Tsukino/Engine/Asset/AssetHandle.hpp>

#include <string>

// 前方宣言
namespace Tsukino::EngineIntegration {
    struct EngineContext;
}

// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {
    //--------------------------------------------------------------
    //! @brief  エンティティのModelComponentが指すモデルに対し、名前でボーンのnodeIndexを解決する。
    //!         resolvedAgainstModelが対象モデルと一致していれば何もしない（毎フレームの名前検索を避ける）
    //! @param  ctx                  [in]     アセットマネージャを引くためのエンジンコンテキスト
    //! @param  registry             [in]     ECSレジストリ
    //! @param  modelEntity          [in]     ModelComponentを持つエンティティ（ボーンの持ち主）
    //! @param  boneName             [in]     解決したいボーン名
    //! @param  resolvedAgainstModel [in,out] 最後に解決した時点のモデルハンドル（キャッシュキー）
    //! @param  outNodeIndex         [in,out] 解決結果のnodeIndex（見つからなければUINT32_MAX）
    //! @note   NodeWorldMatrixComponent/NodeWorldPoseComponentはModelComponent側のノード並びで書かれるので、
    //!         再生中のクリップが切り替わっても再解決は要らない。モデルがまだ読めていなければ何もせず、次の呼び出しで解決する
    //--------------------------------------------------------------
    void ResolveBoneNodeIndex(Tsukino::EngineIntegration::EngineContext* ctx, Tsukino::ECS::Registry& registry, Tsukino::ECS::Entity modelEntity,
                              const std::string& boneName, Tsukino::Asset::AssetHandle& resolvedAgainstModel, Tsukino::u32& outNodeIndex);
}    // namespace Tsukino::BuiltIn::ECS
