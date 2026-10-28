//----------------------------------------------------------------------------
//! @file   UIButtonSoundSystem.hpp
//! @brief  画面UIのボタンの音を鳴らすシステム
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/Core/ECS/System/ISystem.hpp>

// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {

    //! 画面UIのボタンの音を鳴らすシステムです。
    //! @note   PointerTargetComponent と UIButtonSoundComponent を持つエンティティについて、
    //!         hoverEntered なら hoverSound を、clicked なら clickSound を鳴らす（同じフレームに両方なら clickSound だけ）。
    //!         InteractionSystem の後に置くこと（同じフレームのホバー・クリックを読む）
    class UIButtonSoundSystem : public Tsukino::ECS::ISystem {
    public:
        //! ボタンの音を鳴らします。
        //! @param  [in] registry  ECS レジストリ
        //! @param  [in] deltaTime 前フレームからの経過時間（秒）
        void Update(Tsukino::ECS::Registry& registry, float deltaTime) override;
    };
}    // namespace Tsukino::BuiltIn::ECS
