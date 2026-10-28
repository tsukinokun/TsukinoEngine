//----------------------------------------------------------------------------
//! @file   UISliderSystem.hpp
//! @brief  画面UIのスライダーをマウスで動かすシステム
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/Core/ECS/System/ISystem.hpp>

// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {

    //! 画面UIのスライダーをマウスで動かすシステムです。
    //! @note   UISliderComponent の当たり（PointerTargetComponent::clicked）でつかみ、
    //!         左ボタンを離すまで、マウスの位置を track のローカル座標へ戻して値を決める。
    //!         InteractionSystem の後に置くこと（同じフレームのクリックを読む）
    class UISliderSystem : public Tsukino::ECS::ISystem {
    public:
        //! スライダーの値を更新します。
        //! @param  [in] registry  ECS レジストリ
        //! @param  [in] deltaTime 前フレームからの経過時間（秒）
        void Update(Tsukino::ECS::Registry& registry, float deltaTime) override;
    };
}    // namespace Tsukino::BuiltIn::ECS
