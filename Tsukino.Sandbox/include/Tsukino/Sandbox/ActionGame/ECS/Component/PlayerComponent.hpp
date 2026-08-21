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

        Tsukino::ECS::Entity pickupTarget = entt::null;    //!< 現在拾える対象（Fキーの対象）。PickupSystemが毎フレーム更新する

        //-------------------------------------------------------------
        // 左クリックの生入力。PlayerSystemが立て、PlayerAnimationSystemが消費してfalseへ戻す。
        // WeaponComponent::attackRequestedとは意味が異なる点に注意：こちらは「入力があったか」、
        // attackRequestedは「（コンボ受付を含めた判定の結果）攻撃スイングが実際に始まったか」を表す。
        // 攻撃中の入力はすぐには反映されず、PlayerAnimationSystem側で1回だけ先行入力としてバッファされる
        //-------------------------------------------------------------
        bool attackInputPressed = false;
    };
}    // namespace ActionGame::ECS
