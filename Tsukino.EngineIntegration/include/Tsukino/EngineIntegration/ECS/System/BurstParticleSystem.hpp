//----------------------------------------------------------------------------
//! @file   BurstParticleSystem.hpp
//! @brief  一度に撒いた粒子を動かし、寿命が来たら破棄するシステム
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/Core/ECS/System/ISystem.hpp>

// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {

    //! 一度に撒いた粒子を動かし、寿命が来たら破棄するシステムです。
    //! @note   BurstParticleComponent の速度に重力と減衰を掛けて Transform を動かし、
    //!         大きさと SpriteComponent::tintColor を寿命に沿って変える。出番待ち（age が負）の間は透明にする。
    //!         TransformSystem の前に置くこと（同じフレームの worldMatrix に反映する）
    class BurstParticleSystem : public Tsukino::ECS::ISystem {
    public:
        //! 粒子を更新します。
        //! @param  [in] registry  ECS レジストリ
        //! @param  [in] deltaTime 前フレームからの経過時間（秒）
        void Update(Tsukino::ECS::Registry& registry, float deltaTime) override;
    };
}    // namespace Tsukino::BuiltIn::ECS
