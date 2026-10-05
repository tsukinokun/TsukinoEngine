//-------------------------------------------------------------
//! @file   ScrollViewComponent.hpp
//! @brief  ScrollViewComponentクラスの宣言
//-------------------------------------------------------------
#pragma once
#include <Tsukino/Core/ECS/Entity/Entity.hpp>

#include <entt/entt.hpp>

#include <algorithm>

// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {
    //-------------------------------------------------------------
    //! @struct ScrollViewComponent
    //! @brief  画面UIの縦スクロール
    //! @note   UIClipComponent と一緒に「枠」のエンティティへ付ける。ScrollViewSystem が動かす。
    //!         - content は枠の子にする。中身の行は content の子にし、位置は content から見た
    //!           相対位置（左上が原点、下が +Y）で置く
    //!         - 枠の外は描かれず、クリックも効かない（UIClipComponent）
    //!         - 操作：ホイール（枠の上で）、↑↓・PageUp/PageDown・Home/End、中身のドラッグ、
    //!           スクロールバー（ScrollBarComponent。つまみのドラッグ・溝のクリック）
    //!         - 枠の中の PointerTargetComponent は、押した瞬間ではなく「離した瞬間」に clicked になる。
    //!           中身をドラッグしてスクロールした押し込みはクリックにならない
    //-------------------------------------------------------------
    struct ScrollViewComponent {
        //-------------------------------------------------------------
        // 設定（作る側が決める）
        //-------------------------------------------------------------
        Tsukino::ECS::Entity content   = entt::null;    // 中身のエンティティ（枠の子）。ScrollViewSystem が位置を動かす
        Tsukino::ECS::Entity scrollBar = entt::null;    // スクロールバーの溝（ScrollBarComponent を持つ）。無ければ entt::null
        float                contentHeight = 0.0f;      // 中身の高さ（画面ピクセル）。枠より低ければスクロールしない
        bool                 enabled       = true;      // 入力を受けるか（閉じている画面のスクロールは false にする）。false の間はスクロールバーも隠す
        float                wheelStep     = 60.0f;     // ホイール1目盛りで動く量（ピクセル）
        float                keyStep       = 40.0f;     // ↑↓キー1回で動く量（ピクセル）
        float                followSpeed   = 18.0f;     // 目標位置への追従の速さ（大きいほど速い。1秒あたりの減衰率）
        float                dragThreshold = 6.0f;      // 押したまま動かしたとき、ドラッグとみなす距離（ピクセル）

        //-------------------------------------------------------------
        // 状態（ScrollViewSystem が書く）
        //-------------------------------------------------------------
        float offset       = 0.0f;     // 今の表示位置（0 が一番上。下へスクロールするほど増える）
        float targetOffset = 0.0f;     // 目標の表示位置（offset はここへなめらかに寄っていく）

        bool  pointerDown     = false;    // 枠の中で左ボタンを押している
        bool  dragScrolling   = false;    // この押し込みで中身をドラッグしてスクロールした（次に押すまで true のまま）
        float pressPointerY   = 0.0f;     // ドラッグの基準にしたマウスのY
        float pressOffset     = 0.0f;     // ドラッグの基準にした表示位置
        bool  thumbDragging   = false;    // スクロールバーのつまみをドラッグしている
        float thumbGrabOffset = 0.0f;     // つまみの上端から掴んだ位置までの距離

        //-------------------------------------------------------------
        //! @brief  表示位置を指定する（範囲外は ScrollViewSystem が収める）
        //! @param  newOffset [in] 表示位置（0 が一番上）
        //! @param  immediate [in] true なら追従せず即座にその位置にする
        //-------------------------------------------------------------
        void ScrollTo(float newOffset, bool immediate = false) {
            targetOffset = std::max(0.0f, newOffset);
            if(immediate)
                offset = targetOffset;
        }

        //-------------------------------------------------------------
        //! @brief  一番上へ即座に戻す（画面を開き直したとき用）
        //-------------------------------------------------------------
        void ScrollToTop() {
            ScrollTo(0.0f, true);
        }
    };
}    // namespace Tsukino::BuiltIn::ECS
