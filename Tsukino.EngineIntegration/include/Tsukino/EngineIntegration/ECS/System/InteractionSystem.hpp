//-------------------------------------------------------------
//! @file   InteractionSystem.hpp
//! @brief  InteractionSystemクラスの宣言
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once
#include <Tsukino/Core/ECS/System/ISystem.hpp>
// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {
    struct TransformComponent;    // 前方宣言

    //-------------------------------------------------------------
    //! @class  InteractionSystem
    //! @brief  インタラクションを管理するシステム
    //-------------------------------------------------------------
    class InteractionSystem final : public Tsukino::ECS::ISystem {
    public:
        //-------------------------------------------------------------
        // システムの更新
        //! @param  registry    [in] ECS レジストリ
        //! @param  deltaTime   [in] 前フレームからの経過時間
        //-------------------------------------------------------------
        void Update(Tsukino::ECS::Registry& registry, float deltaTime) override;
    };

}    // namespace Tsukino::BuiltIn::ECS
