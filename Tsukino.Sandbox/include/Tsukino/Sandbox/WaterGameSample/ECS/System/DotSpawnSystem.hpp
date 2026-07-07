//-------------------------------------------------------------
//! @file   DotSpawnSystem.hpp
//! @brief  DotSpawnSystemクラスの宣言
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once

#include <Tsukino/Core/ECS/System/ISystem.hpp>

namespace Tsukino::ECS {
    class Registry;
}

// 名前空間 : WaterGame::ECS
namespace WaterGame::ECS {

    //-------------------------------------------------------------
    //! @class  DotSpawnSystem
    //! @brief  地形の物理Bodyが準備できたタイミングで、地形の高さに沿ってドットを生成する
    //-------------------------------------------------------------
    class DotSpawnSystem : public Tsukino::ECS::ISystem {
    public:
        void Update(Tsukino::ECS::Registry& registry, float deltaTime) override;
    };

}    // namespace WaterGame::ECS
