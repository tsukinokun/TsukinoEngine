//-------------------------------------------------------------
//! @file   BehaviorTreeComponent.hpp
//! @brief  BehaviorTreeComponent構造体の宣言
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once
#include <Tsukino/Sandbox/ActionGame/ECS/Utility/BehaviorTree.hpp>
#include <Tsukino/Sandbox/ActionGame/ECS/Component/EnemyBlackboard.hpp>

#include <memory>
// 名前空間 : ActionGame::ECS
namespace ActionGame::ECS {
    //! @brief 敵のビヘイビアツリーが使うノード型（黒板はEnemyBlackboard固定）
    using EnemyBehaviorNode = BehaviorNode<EnemyBlackboard>;

    //-------------------------------------------------------------
    //! @struct BehaviorTreeComponent
    //! @brief  ビヘイビアツリー駆動の敵であることを表すコンポーネント。
    //!         付与の有無がEnemySystem（旧来の直進追跡）との排他フラグを兼ねる
    //-------------------------------------------------------------
    struct BehaviorTreeComponent {
        //-------------------------------------------------------------
        // ツリーはComposite内にRunning再開位置（m_runningIndex）を持つため、
        // 敵ごとに専用のインスタンスを1本持つ（他エンティティと共有しない）
        //-------------------------------------------------------------
        std::shared_ptr<EnemyBehaviorNode> root;             //!< ツリーのルートノード
        EnemyBlackboard                    blackboard;        //!< このエンティティ専用の黒板
    };
}    // namespace ActionGame::ECS
