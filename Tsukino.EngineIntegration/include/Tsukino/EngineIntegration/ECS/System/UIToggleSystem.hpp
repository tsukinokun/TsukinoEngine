//----------------------------------------------------------------------------
//! @file   UIToggleSystem.hpp
//! @brief  画面UIの切り替えをクリックで入・切するシステム
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/Core/ECS/System/ISystem.hpp>

// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {

    //! 画面UIの切り替えをクリックで入・切するシステムです。
    //! @note   組の無い切り替えは押すたびに入れ替え、組のある切り替えは押したものだけを入にする。
    //!         値が変わった切り替えと組に changed を立てる（次のフレームの初めに下ろす）。
    //!         InteractionSystem の後に置くこと（同じフレームのクリックを読む）
    class UIToggleSystem : public Tsukino::ECS::ISystem {
    public:
        //! 切り替えを更新します。
        //! @param  [in] registry  ECS レジストリ
        //! @param  [in] deltaTime 前フレームからの経過時間（秒）
        void Update(Tsukino::ECS::Registry& registry, float deltaTime) override;
    };
}    // namespace Tsukino::BuiltIn::ECS
