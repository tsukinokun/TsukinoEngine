//----------------------------------------------------------------------------
//! @file   UITweenSystem.hpp
//! @brief  画面UIのトゥイーンとボタンの手ごたえを、見た目のずれ（UIMotionComponent）と透明度に反映するシステム
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/Core/ECS/System/ISystem.hpp>

// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {

    //! 画面UIのトゥイーンとボタンの手ごたえのシステムです。
    //! @note   毎フレーム次の順に行う。
    //!         1. UITweenComponent か UIButtonFeedbackComponent を持つエンティティの UIMotionComponent を単位に戻す
    //!         2. トゥイーンの時間を進めて値を合成する（拡大率と α は掛け合わせ、ずれと回転は足し合わせる）。
    //!            α は UICanvasGroupComponent::alpha に書く。終わった Once のトゥイーンは取り除く
    //!         3. ボタンの手ごたえ（PointerTargetComponent の hovered・pressed・clicked）を拡大率に掛ける
    //!         4. UIMotionComponent::source を持つものに、写し元の動き（中心は写し元で確定させた位置）と
    //!            UICanvasGroupComponent::alpha を写す。自分の動きも持つものは、自分の動きのあとに写し元の動きを重ねる。
    //!            写し元が写し先でないものを先に処理するので、パネル → ボタン → ラベルの2段まで正しく写る
    //!         必要な UIMotionComponent・UICanvasGroupComponent は無ければ付ける。
    //!         UITween::unscaledTime のトゥイーンとボタンの手ごたえは、レジストリに UnscaledTimeContext があれば
    //!         その経過時間で進める（ゲームがポーズ中に deltaTime = 0 を渡しても、メニューの UI は動かせる）。
    //!         InteractionSystem の後（同じフレームのホバー・クリックを読む）、トゥイーンを再生する UI のシステムの後、
    //!         TransformSystem の前（同じフレームの worldMatrix に反映する）に置くこと
    class UITweenSystem : public Tsukino::ECS::ISystem {
    public:
        //! トゥイーンとボタンの手ごたえを反映します。
        //! @param  [in] registry  ECS レジストリ
        //! @param  [in] deltaTime 前フレームからの経過時間（秒）
        void Update(Tsukino::ECS::Registry& registry, float deltaTime) override;
    };
}    // namespace Tsukino::BuiltIn::ECS
