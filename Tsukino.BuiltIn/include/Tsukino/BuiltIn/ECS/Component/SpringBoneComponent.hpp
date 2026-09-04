//-------------------------------------------------------------
//! @file   SpringBoneComponent.hpp
//! @brief  SpringBoneComponentクラスの宣言
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once
#include <Tsukino/Physics/SpringBone/SpringBoneData.hpp>
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

            // ============================================================
            // [NOTE] attachNodeNameの扱い
            // 空文字の場合、attachNodeIndex = UINT32_MAX となり、
            // コライダーはワールド固定の球となる。
            // これは「全体的な衝突対象」として使用できる。
            // ============================================================
        };

        //-------------------------------------------------------------
        //! @struct ChainDef
        //! @brief  1本の揺れ物チェーンの定義（ノードは名前で指定）
        //-------------------------------------------------------------
        struct ChainDef {
            std::string name;
            std::string anchorNodeName;    // BuildChainFromHierarchy用：この子孫を全部揺らす（例：髪）
            std::string rootNodeName;      // BuildChainFromRoot用：このボーン自身から揺らす、兄弟は巻き込まない（例：胸）
                                           // ※ rootNodeNameが空でなければこちらを優先し、anchorNodeNameは無視する
            std::vector<std::string>             excludeNodeNames;
            u32                                  maxDepth = 0;
            Tsukino::Physics::SpringBoneSettings settings;
            std::vector<ColliderDef>             colliders;

            // ============================================================
            // [NOTE] rootNodeName vs anchorNodeName の使い分け
            // 
            // - rootNodeName使用: 特定の1本だけを揺らしたい場合
            //   例: 胸の左右個別の揺れ、首の左右個別の揺れ
            //   特徴: 兄弟ボーンは巻き込まれない
            // 
            // - anchorNodeName使用: あるノードの全子孫を揺らしたい場合
            //   例: 髪の全ての毛先、スカートの全てのパネル
            //   特徴: 兄弟も含めて全ての子孫が揺る
            // 
            // maxDepth = 0 の場合は無制限（全子孫）
            // maxDepth = 1 の場合は「自分だけ」または「直接の子だけ」
            // ============================================================
        };

        std::vector<ChainDef> chainDefs;    // 人間が設定する定義（コード側で直接セット）

        std::vector<Tsukino::Physics::SpringBoneChain> chains;    // 実行時状態（AnimationSystemが自動生成）

        bool resolved = false;    // chainsをノード名から構築済みか
        bool enabled  = true;     // 物理を止めたい場合false

        // ============================================================
        // [NOTE] resolvedフラグの注意点
        // 
        // AnimationSystem::Update()で初回チェックされるが、
        // モデルがロードされ直った場合や、チェーン定義が変更された場合
        // はresolved = false にリセットする必要がある。
        // 
        // 現在の実装では、resolved = trueのまま残り、
        // モデル差し替え時に古いチェーンが残る可能性がある。
        // 
        // 推奨: モデルハンドルが変更された場合はresolved = false にする、
        // または毎フレームチェーン定義のハッシュを比較する。
        // ============================================================
    };

}    // namespace Tsukino::BuiltIn::ECS
