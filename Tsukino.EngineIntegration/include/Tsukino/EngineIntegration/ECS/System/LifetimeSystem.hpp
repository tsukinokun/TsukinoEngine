//----------------------------------------------------------------------------
//! @file   LifetimeSystem.hpp
//! @brief  時間が来たエンティティを子ごと片付けるシステム
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/Core/ECS/System/ISystem.hpp>

// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {

    //! 時間が来たエンティティを子ごと片付けるシステムです。
    //! @note   LifetimeComponent の残り時間を deltaTime だけ減らし、0 以下になったものを
    //!         Transform の子孫ごと破棄予約する（実際の破棄はフレームの終わり）
    class LifetimeSystem : public Tsukino::ECS::ISystem {
    public:
        //! 残り時間を減らして片付けます。
        //! @param  [in] registry  ECS レジストリ
        //! @param  [in] deltaTime 前フレームからの経過時間（秒）
        void Update(Tsukino::ECS::Registry& registry, float deltaTime) override;
    };
}    // namespace Tsukino::BuiltIn::ECS
