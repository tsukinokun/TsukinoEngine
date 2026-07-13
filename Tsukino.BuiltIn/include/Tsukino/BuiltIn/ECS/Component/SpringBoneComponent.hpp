//-------------------------------------------------------------
//! @file   SpringBoneComponent.hpp
//! @brief  SpringBoneComponentクラスの宣言
//! @author 山﨑 愛
//-------------------------------------------------------------
#pragma once
#include <Tsukino/Engine/Physics/SpringBone/SpringBoneData.hpp>
#include <string>
#include <vector>

// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {

    //-------------------------------------------------------------
    //! @struct SpringBoneComponent
    //! @brief  揺れ物物理の設定＋実行時状態
    //-------------------------------------------------------------
    struct SpringBoneComponent {
        //-------------------------------------------------------------
        //! @struct ColliderDef
        //! @brief  コライダー定義（ノードは名前で指定、初回に解決）
        //-------------------------------------------------------------
        struct ColliderDef {
            std::string    attachNodeName;    // 空文字ならワールド固定
            hlslpp::float3 localOffset = hlslpp::float3(0.0f, 0.0f, 0.0f);
            float          radius      = 0.1f;
        };

        //-------------------------------------------------------------
        //! @struct ChainDef
        //! @brief  1本の揺れ物チェーンの定義（ノードは名前で指定）
        //-------------------------------------------------------------
        struct ChainDef {
            std::string                          name;
            std::string                          anchorNodeName;
            std::vector<std::string>             excludeNodeNames;
            u32                                  maxDepth = 0;
            Tsukino::Physics::SpringBoneSettings settings;
            std::vector<ColliderDef>             colliders;
        };

        std::vector<ChainDef> chainDefs;    // 人間が設定する定義（コード側で直接セット）

        std::vector<Tsukino::Physics::SpringBoneChain> chains;    // 実行時状態（AnimationSystemが自動生成）

        bool resolved = false;    // chainsをノード名から構築済みか
        bool enabled  = true;     // 物理を止めたい場合false
    };

}    // namespace Tsukino::BuiltIn::ECS
