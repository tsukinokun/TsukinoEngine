//-------------------------------------------------------------
//! @file   HeightmapGenerationSystem.hpp
//! @brief  HeightmapGenerationSystemクラスの宣言
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once
#include <Tsukino/Core/ECS/System/ISystem.hpp>
// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {
    //-------------------------------------------------------------------------
    // @class  HeightmapGenerationSystem
    // @brief  TerrainGenerationRequestComponentを持つエンティティを検出し、
    //         ノイズから高さデータを生成してCollisionComponent(Heightfield)へ書き込むシステム。
    //         実際のJolt Body生成はPhysicsSystemが行うため、
    //         必ずPhysicsSystem::Updateより前に実行すること。
    //-------------------------------------------------------------------------
    class HeightmapGenerationSystem : public Tsukino::ECS::ISystem {
    public:
        void Update(Tsukino::ECS::Registry& registry, float deltaTime) override;
    };
}    // namespace Tsukino::BuiltIn::ECS
