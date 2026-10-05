//-------------------------------------------------------------
//! @file   ScrollViewSystem.hpp
//! @brief  ScrollViewSystemクラスの宣言
//-------------------------------------------------------------
#pragma once
#include <Tsukino/Core/ECS/System/ISystem.hpp>
// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {
    //-------------------------------------------------------------
    //! @class  ScrollViewSystem
    //! @brief  ScrollViewComponent の入力を受けて表示位置を動かし、中身とスクロールバーを置き直すシステム
    //! @note   入力（enabled のビューだけ）：
    //!         ・ホイール：マウスが枠の上にあるとき
    //!         ・キー：↑↓（keyStep）・PageUp/PageDown（枠の高さの9割）・Home/End
    //!         ・中身のドラッグ：枠の中で押し、dragThreshold を超えて動かすと指に吸い付いて動く
    //!         ・スクロールバー：つまみのドラッグ、溝の空いている所のクリックで1画面ぶん
    //!         表示位置は目標位置へ followSpeed でなめらかに寄せる（ドラッグ中は即座に合わせる）。
    //!         InteractionSystem の後（同じフレームのクリック判定が前のフレームのドラッグ状態を読む）、
    //!         TransformSystem の前（動かした中身が同じフレームに描かれる）に置くこと
    //-------------------------------------------------------------
    class ScrollViewSystem : public Tsukino::ECS::ISystem {
    public:
        //-------------------------------------------------------------
        // システムの更新
        //! @param  registry  [in] ECS レジストリ
        //! @param  deltaTime [in] 前フレームからの経過時間（秒）
        //-------------------------------------------------------------
        void Update(Tsukino::ECS::Registry& registry, float deltaTime) override;
    };
}    // namespace Tsukino::BuiltIn::ECS
