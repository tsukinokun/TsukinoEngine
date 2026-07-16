//--------------------------------------------------------------
//! @file   SpringBonePhysics.hpp
//! @brief  揺れ物(SpringBone)物理の計算関数群
//! @author 山﨑 愛
//--------------------------------------------------------------
#pragma once
#include <Tsukino/Engine/Physics/SpringBone/SpringBoneData.hpp>
#include <Tsukino/GraphicsCommon/Node/NodeData.hpp>
#include <string>
#include <unordered_set>
#include <vector>

// 名前空間 Tsukino::Physics
namespace Tsukino::Physics::SpringBonePhysics {

    //--------------------------------------------------------------
    //! @brief  ノード階層をDFSで辿り、揺れ物チェーンを構築する
    //! @param  name             デバッグ用名前
    //! @param  anchorNodeIndex  揺れの起点（物理を受けない固定ノード）
    //! @param  nodes            モデル全体のノード配列（ModelData.nodes）
    //! @param  excludeNodeNames ここに含まれる名前のノードから先は辿らない
    //! @param  maxDepth         アンカーから何階層まで辿るか（0で無制限）
    //! @param  settings         チェーンの物理パラメータ
    //--------------------------------------------------------------
    SpringBoneChain BuildChainFromHierarchy(const std::string&                           name,
                                            u32                                          anchorNodeIndex,
                                            const std::vector<GraphicsCommon::NodeData>& nodes,
                                            const std::unordered_set<std::string>&       excludeNodeNames,
                                            u32                                          maxDepth,
                                            const SpringBoneSettings&                    settings);

    //--------------------------------------------------------------
    //! @brief  特定の1本のボーン(rootNodeIndex)を起点にチェーンを構築する。
    //!         その親を「動かないアンカー」として自動的に使うが、
    //!         アンカーの他の子（兄弟ボーン）は一切巻き込まない。
    //!         胸のように「特定の1本だけを、共通の親を基準に揺らしたい」
    //!         ケース向け（BuildChainFromHierarchyだと親の子を全部拾ってしまう）。
    //! @param  name             デバッグ用名前
    //! @param  rootNodeIndex    チェーンの起点となる、実際に揺らしたいボーン
    //!                          （このノード自身が最初のシミュレーション対象になる）
    //! @param  nodes            モデル全体のノード配列（ModelData.nodes）
    //! @param  excludeNodeNames ここに含まれる名前のノードから先は辿らない
    //! @param  maxDepth         rootNodeIndexから何階層まで辿るか（0で無制限、1ならrootNodeIndex単体）
    //! @param  settings         チェーンの物理パラメータ
    //--------------------------------------------------------------
    SpringBoneChain BuildChainFromRoot(const std::string&                           name,
                                       u32                                          rootNodeIndex,
                                       const std::vector<GraphicsCommon::NodeData>& nodes,
                                       const std::unordered_set<std::string>&       excludeNodeNames,
                                       u32                                          maxDepth,
                                       const SpringBoneSettings&                    settings);

    //--------------------------------------------------------------
    //! @brief  現在の姿勢でシミュレーション状態を初期化する
    //!         （SpringBoneComponent登録直後、物理を破綻させないために
    //!         Update()の前に一度呼ぶ）
    //--------------------------------------------------------------
    void InitializeChain(SpringBoneChain& chain, const std::vector<WorldPose>& animatedPoses);

    //--------------------------------------------------------------
    //! @brief  1チェーン分の物理を1フレーム進める
    //! @param  chain          [in,out] 更新対象（currentPosition等が書き換わる）
    //! @param  deltaTime      経過時間（秒）
    //! @param  animatedPoses  ノードインデックスで引ける、物理を考慮しない現フレームの姿勢
    //--------------------------------------------------------------
    void UpdateChain(SpringBoneChain& chain, float deltaTime, const std::vector<WorldPose>& animatedPoses);

}    // namespace Tsukino::Physics::SpringBonePhysics
