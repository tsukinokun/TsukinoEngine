//-------------------------------------------------------------
//! @file   WeaponComponent.hpp
//! @brief  WeaponComponent構造体の宣言
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once
#include <Tsukino/Core/ECS/Entity/Entity.hpp>

#include <hlsl++.h>
// 名前空間 : ActionGame::ECS
namespace ActionGame::ECS {
    //-------------------------------------------------------------
    //! @struct WeaponComponent
    //! @brief  武器エンティティに付与するコンポーネント。
    //!         所有者（プレイヤー）に追従しつつ、攻撃入力時に範囲内の敵へダメージを与える。
    //!         ボーンアタッチ未実装のため所有者からの固定ローカルオフセットで位置を近似し、
    //!         当たり判定もJolt物理を使わず距離判定で簡易的に行う（どちらもPhase Bで本実装に差し替え予定）
    //-------------------------------------------------------------
    struct WeaponComponent {
        Tsukino::ECS::Entity owner = entt::null;    //!< 武器を所持しているエンティティ

        hlslpp::float3 localOffset = hlslpp::float3(30.0f, 100.0f, 60.0f);    //!< 所有者からの相対オフセット

        float damage         = 20.0f;    //!< 命中時に与えるダメージ
        float range           = 90.0f;   //!< 攻撃判定の到達距離（武器位置からの単純な距離判定に使用）
        float activeDuration = 0.25f;    //!< 攻撃入力後、当たり判定が有効な時間（秒）
        float cooldown       = 0.4f;     //!< 攻撃後、再攻撃可能になるまでのクールダウン（秒）

        bool  attackRequested = false;    //!< 攻撃入力を受け取ったか（PlayerSystemがセットする）
        bool  isActive        = false;    //!< 現在当たり判定が有効か
        float activeTimer     = 0.0f;     //!< 当たり判定有効時間の残り
        float cooldownTimer   = 0.0f;     //!< クールダウンの残り
    };
}    // namespace ActionGame::ECS
