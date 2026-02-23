//-------------------------------------------------------------
//! @file   ISystem.hpp
//! @brief  システムインターフェースの宣言
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once
#include <Tsukino/Core/ECS/Registry.hpp>
// 名前空間 : Tsukino::Engine::ECS
namespace Tsukino::Engine::ECS {
    //-------------------------------------------------------------
    //! @class  ISystem
    //! @brief  システムインターフェース
    //-------------------------------------------------------------
    class ISystem {
    public:
        //-------------------------------------------------------------
        //! @brief  仮想デストラクタ
        //-------------------------------------------------------------
        virtual ~ISystem() = default;

        //-------------------------------------------------------------
        //! @brief  システムの更新
        //! @param  registry    [in] ECS レジストリ
        //! @param  deltaTime   [in] 前フレームからの経過時間
        //-------------------------------------------------------------
        virtual void Update(Tsukino::ECS::Registry& registry, float deltaTime) = 0;
    };

}    // namespace Tsukino::Engine::ECS
