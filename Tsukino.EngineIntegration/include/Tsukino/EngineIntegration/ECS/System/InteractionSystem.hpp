//-------------------------------------------------------------
//! @file   InteractionSystem.hpp
//! @brief  InteractionSystemクラスの宣言
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once
#include <Tsukino/Core/ECS/System/ISystem.hpp>
#include <Tsukino/Core/ECS/Entity/Entity.hpp>

#include <entt/entt.hpp>
// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {
    struct TransformComponent;    // 前方宣言

    //-------------------------------------------------------------
    //! @class  InteractionSystem
    //! @brief  インタラクションを管理するシステム
    //! @note   画面空間のスプライトに対するマウス操作を2つ扱う。
    //!         ・PointerTargetComponent：マウスの重なり（hovered）とクリック（clicked）を毎フレーム書く
    //!         ・DraggableComponent     ：左ボタンで掴んでドラッグする
    //!         どちらも、マウスの下で最も手前（sortOrderが大きい）のスプライト1つだけが対象で、
    //!         そこから親を辿って最初に見つかったコンポーネントが反応する。
    //!         祖先に UIClipComponent があるスプライトは、その枠の外では当たらない。
    //!         祖先に ScrollViewComponent がある PointerTargetComponent の clicked は、
    //!         押した瞬間ではなく「押したものの上で離した瞬間」に立つ（中身のドラッグと区別するため）。
    //!         判定はスプライトのworldMatrixで行う。フレームの先頭（UIを動かす処理や
    //!         TransformSystemより前）に置けば、前フレームに描いた＝画面に見えている配置と一致し、
    //!         同じフレームのメニュー処理が結果を読める
    //-------------------------------------------------------------
    class InteractionSystem final : public Tsukino::ECS::ISystem {
    public:
        //-------------------------------------------------------------
        // システムの更新
        //! @param  registry    [in] ECS レジストリ
        //! @param  deltaTime   [in] 前フレームからの経過時間
        //-------------------------------------------------------------
        void Update(Tsukino::ECS::Registry& registry, float deltaTime) override;

        //-------------------------------------------------------------
        //! @brief  指定座標がドラッグ可能なスプライト上、またはドラッグ中の
        //!         スプライトが存在するかを調べる関数
        //! @param  registry [in] ECS レジストリ
        //! @param  x        [in] 判定するX座標
        //! @param  y        [in] 判定するY座標
        //! @return true: いずれかのスプライト上にある、またはドラッグ中
        //-------------------------------------------------------------
        static bool HitTest(Tsukino::ECS::Registry& registry, float x, float y);

    private:
        // 左ボタンを押したときに重なっていたPointerTargetComponentのエンティティ。
        // スクロールの枠の中のクリックを「押したものの上で離した」で判定するため、離すまで覚えておく
        Tsukino::ECS::Entity m_pressedTarget = entt::null;
    };

}    // namespace Tsukino::BuiltIn::ECS
