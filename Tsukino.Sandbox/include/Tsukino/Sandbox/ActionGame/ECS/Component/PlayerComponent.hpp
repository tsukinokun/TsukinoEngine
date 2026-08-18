//-------------------------------------------------------------
//! @file   PlayerComponent.hpp
//! @brief  PlayerComponent構造体の宣言
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once
#include <Tsukino/Core/ECS/Entity/Entity.hpp>
#include <vector>
// 名前空間 : ActionGame::ECS
namespace ActionGame::ECS {
    //-------------------------------------------------------------
    //! @struct PlayerComponent
    //! @brief  プレイヤーエンティティであることを表すコンポーネント
    //-------------------------------------------------------------
    struct PlayerComponent {
        float moveSpeed = 300.0f;    //!< 水平移動速度（1ユニット≒1cm規約。軽いジョグ程度）
        float turnLerpSpeed = 12.0f; //!< 移動方向への向き直しの補間速度（大きいほど素早く向く）

        float sprintSpeedMultiplier = 1.6f;     //!< Shift押下時、moveSpeedに掛ける倍率
        bool  isSprinting            = false;    //!< 移動中にShiftが押されているか（PlayerSystemが毎フレーム更新。PlayerAnimationSystemが参照）

        Tsukino::ECS::Entity weaponEntity = entt::null;    //!< 装備中の武器エンティティ（WeaponComponentを持つ）。切り替え時はここを差し替える

        std::vector<Tsukino::ECS::Entity> weaponInventory;      //!< 浮遊武器の一覧（切り替え対象）。シーン初期化時に設定する
        int                                selectedWeaponIndex = 0;    //!< weaponInventory内の現在選択インデックス。weaponEntityと同期させる
    };
}    // namespace ActionGame::ECS
