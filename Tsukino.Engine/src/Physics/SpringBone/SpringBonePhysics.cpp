//--------------------------------------------------------------
//! @file   SpringBonePhysics.cpp
//! @brief  揺れ物(SpringBone)物理の計算関数群 実装
//! @author 山﨑 愛
//--------------------------------------------------------------
#include <Tsukino/Engine/Physics/SpringBone/SpringBonePhysics.hpp>
#include <Tsukino/Engine/Physics/SpringBone/SpringBoneMath.hpp>
#include <Tsukino/Core/Log.hpp>
#include <algorithm>
#include <functional>
#include <cmath>

// 名前空間 Tsukino::Physics::SpringBonePhysics
namespace Tsukino::Physics::SpringBonePhysics {

    namespace {
        constexpr float kMaxDeltaTime = 1.0f / 30.0f;    // フレーム落ち時の暴れ防止
        constexpr float kDegToRad     = 3.14159265358979323846f / 180.0f;
    }    // namespace

//--------------------------------------------------------------
    //! @brief  ノード階層をDFSで辿り、揺れ物チェーンを構築する
    //--------------------------------------------------------------
    SpringBoneChain BuildChainFromHierarchy(const std::string&                           name,
                                             u32                                          anchorNodeIndex,
                                             const std::vector<GraphicsCommon::NodeData>& nodes,
                                             const std::unordered_set<std::string>&       excludeNodeNames,
                                             u32                                          maxDepth,
                                             const SpringBoneSettings&                    settings) {
        SpringBoneChain chain;
        chain.name            = name;
        chain.anchorNodeIndex = anchorNodeIndex;
        chain.settings        = settings;

        if(anchorNodeIndex >= nodes.size()) {
            Tsukino::Core::Log::Error("SpringBonePhysics: invalid anchor node for chain '" + name + "'");
            return chain;
        }

        // DFSでアンカーの子孫を辿る。親は必ず自分より前のインデックスになる
        // （UpdateChainで先頭から順番に処理できる）。
        std::function<void(u32, i32, u32)> Visit = [&](u32 nodeIndex, i32 parentIndexInChain, u32 depth) {
            if(maxDepth != 0 && depth > maxDepth) {
                return;
            }
            if(excludeNodeNames.count(nodes[nodeIndex].name) > 0) {
                return;
            }

            i32 myIndexInChain = -1;

            // アンカー自身は登録しない（アンカーはFKに完全追従する固定点）
            if(nodeIndex != anchorNodeIndex) {
                SpringBoneNode n;
                n.nodeIndex          = nodeIndex;
                n.parentIndexInChain = parentIndexInChain;
                chain.nodes.push_back(n);
                myIndexInChain = static_cast<i32>(chain.nodes.size() - 1);
            }

            for(u32 childIndex : nodes[nodeIndex].childIndices) {
                Visit(childIndex, myIndexInChain, depth + 1);
            }
        };

        // ============================================================
        // [NOTE] 複雑な階層構造での注意点
        // 
        // 1. 兄弟ノードの扱い:
        //    アンカーの子として登録された兄弟ノードは、全てチェーンに含まれる。
        //    これは髪のような「全ての毛先を揺らす」ケースに適している。
        // 
        // 2. 分岐構造の処理:
        //    兄弟関係のノードは並列に処理されるため、互いに影響を受けない。
        //    各兄弟はアンカーを親として独立したチェーン要素となる。
        // 
        // 3. excludeNodeNamesの動作:
        //    指定された名前のノードからその子孫は全て除外される。
        //    これは髪型や服装で「特定の部分を固定」するための機能。
        // ============================================================
        Visit(anchorNodeIndex, -1, 0);
        return chain;
    }

    //--------------------------------------------------------------
    //! @brief  BuildChainFromHierarchyの深さ制限の注意点
    //--------------------------------------------------------------
    /*
    maxDepthの動作:
    - maxDepth = 0: 無制限に子孫を辿る
    - maxDepth = 1: アンカーの直接の子だけを含む
    - maxDepth = 2: アンカーの子とその子の孫まで含む
    
    ただし、アンカー自身はチェーンには含まれないので、
    maxDepth=1の場合は「アンカーの子1階層目」が対象になる。
    
    [NOTE] 現在の実装:
    if(maxDepth != 0 && depth > maxDepth) の条件
    これはdepth <= maxDepthのときのみVisitを続ける、という意味。
    depth=0でアンカーをVisit(ただし登録されない)
    depth=1で子ノードをVisit
    depth=2で孫ノードをVisit...
    
    したがってmaxDepth=1の場合、depth=1で子ノードが登録され、
    depth=2でのVisitは打ち切られるので、子の子は含まれない。
    これは意図通りの動作ではあるが、ドキュメント化が不足している。
    */

    //--------------------------------------------------------------
    //! @brief 特定の1本のボーン(rootNodeIndex)を起点にチェーンを構築する。
    //--------------------------------------------------------------
    SpringBoneChain BuildChainFromRoot(const std::string&                           name,
                                       u32                                          rootNodeIndex,
                                       const std::vector<GraphicsCommon::NodeData>& nodes,
                                       const std::unordered_set<std::string>&       excludeNodeNames,
                                       u32                                          maxDepth,
                                       const SpringBoneSettings&                    settings) {
        SpringBoneChain chain;
        chain.name = name;

        if(rootNodeIndex >= nodes.size()) {
            Tsukino::Core::Log::Error("SpringBonePhysics: invalid root node for chain '" + name + "'");
            return chain;
        }

        // rootNodeIndex自身の親を「動かないアンカー」として使う。
        // BuildChainFromHierarchyと違い、アンカーの他の子（兄弟）は一切辿らない。
        chain.anchorNodeIndex = nodes[rootNodeIndex].parentIndex;
        chain.settings        = settings;

        // DFSでrootNodeIndex自身とその子孫を辿る（rootNodeIndex自身も対象に含める）。
        std::function<void(u32, i32, u32)> Visit = [&](u32 nodeIndex, i32 parentIndexInChain, u32 depth) {
            if(maxDepth != 0 && depth > maxDepth) {
                return;
            }
            if(excludeNodeNames.count(nodes[nodeIndex].name) > 0) {
                return;
            }

            SpringBoneNode n;
            n.nodeIndex          = nodeIndex;
            n.parentIndexInChain = parentIndexInChain;
            chain.nodes.push_back(n);
            i32 myIndexInChain = static_cast<i32>(chain.nodes.size() - 1);

            for(u32 childIndex : nodes[nodeIndex].childIndices) {
                Visit(childIndex, myIndexInChain, depth + 1);
            }
        };

        // ============================================================
        // [NOTE] maxDepthの動作
        // Visitをdepth=1で開始するため、maxDepthの解釈がややこしい:
        // - maxDepth = 0: 無制限
        // - maxDepth = 1: rootNodeIndex単体のみ（depth=1のみ許可、depth=2で打ち切り）
        // - maxDepth = 2: root + 子 (depth=1, 2まで許可)
        // 
        // これは「自身を含めて何階層」という意味で、
        // maxDepth=1は「自分だけ」、maxDepth=2は「自分+子」という解釈になる。
        // ============================================================
        Visit(rootNodeIndex, -1, 1);
        return chain;
    }

    //--------------------------------------------------------------
    //! @brief  BuildChainFromRootの注意点
    //--------------------------------------------------------------
    /*
    BuildChainFromRootは:
    1. rootNodeIndexの親をアンカーとして使用
    2. rootNodeIndex自身とその子孫をチェーンに含める
    
    重要な点:
    - rootNodeIndex自身はチェーンの最初のノードとして登録される
    - したがってparentIndexInChainは常に-1になる（最初のノードはアンカーの下）
    - ただし、2番目以降のノードは正しいparentIndexInChainを持つ
    
    [NOTE] restLengthの計算:
    InitializeChain()で計算されるが、rootNodeIndexの場合は:
    - parentIndexInChain = -1 のとき、parentPos = anchorPos
    - したがってrestLength = distance(anchorPos, rootPos)
    
    これは正しい動作だが、チェーン構築時に一度目を通しておくと
    デバッグしやすくなる。
    */

    //--------------------------------------------------------------
    //! @brief  揺れ物チェーンの初期化
    //--------------------------------------------------------------
    void InitializeChain(SpringBoneChain& chain, const std::vector<WorldPose>& animatedPoses) {
        if(chain.anchorNodeIndex >= animatedPoses.size()) {
            return;
        }

        chain.previousAnchorPosition = animatedPoses[chain.anchorNodeIndex].position;
        chain.anchorInitialized      = true;

        for(auto& node : chain.nodes) {
            if(node.nodeIndex >= animatedPoses.size()) {
                continue;
            }

            const hlslpp::float3 parentPos =
                (node.parentIndexInChain < 0) ? chain.previousAnchorPosition : chain.nodes[node.parentIndexInChain].currentPosition;

            const hlslpp::float3 myPos = animatedPoses[node.nodeIndex].position;

            // ============================================================
            // [NOTE] restLength = 0 のケース
            // モデルの問題として、親と同じ位置にボーンがある場合、
            // restLengthが0になる。これはUpdateChain()の
            // ApplyLengthConstraint()でNaNを引き起こす。
            // 
            // 対策: 適切な最小値を設定する、または
            // モデルの階層構造を修正する。
            // ============================================================
            node.restLength = float(hlslpp::length(myPos - parentPos));

            node.currentPosition   = myPos;
            node.previousPosition  = myPos;
            node.correctedRotation = animatedPoses[node.nodeIndex].rotation;
            node.initialized       = true;
        }
    }

    //--------------------------------------------------------------
    //! @brief  揺れ物チェーンの更新
    //--------------------------------------------------------------
    void UpdateChain(SpringBoneChain& chain, float deltaTime, const std::vector<WorldPose>& animatedPoses) {
        if(chain.anchorNodeIndex >= animatedPoses.size()) {
            return;
        }

        deltaTime = std::min(deltaTime, kMaxDeltaTime);
        if(deltaTime <= 0.0f) {
            return;
        }

        // 未初期化ならこのフレームの姿勢で初期化してから更新する
        if(!chain.anchorInitialized || (!chain.nodes.empty() && !chain.nodes.front().initialized)) {
            InitializeChain(chain, animatedPoses);
        }

        const SpringBoneSettings& s = chain.settings;

        // ------------------------------------------------------------
        // アンカー（ルート）の移動量から慣性補正を計算
        // ------------------------------------------------------------
        const hlslpp::float3 anchorPos   = animatedPoses[chain.anchorNodeIndex].position;
        const hlslpp::float3 anchorDelta = anchorPos - chain.previousAnchorPosition;
        chain.previousAnchorPosition     = anchorPos;

        const float          inertiaKeep = 1.0f - std::clamp(s.inertia, 0.0f, 1.0f);
        const hlslpp::float3 gravity     = hlslpp::normalize(s.gravityDir);
        const hlslpp::float3 gravityStep = gravity * (s.gravityScale * deltaTime * deltaTime);

        const float angleLimitRad = s.angleLimitDeg * kDegToRad;

        for(u32 i = 0; i < chain.nodes.size(); ++i) {
            SpringBoneNode& node = chain.nodes[i];
            if(node.nodeIndex >= animatedPoses.size()) {
                continue;
            }

            const hlslpp::float3 parentSimPos      = (node.parentIndexInChain < 0) ? anchorPos : chain.nodes[node.parentIndexInChain].currentPosition;
            const hlslpp::float3 animatedTargetPos = animatedPoses[node.nodeIndex].position;    // 剛体だった場合の位置

            // ============================================================
            // [BUG NOTE] 慣性補正の実装はUnityChan/VRMと異なる
            // UnityChan: force += (prevTipPos - currTipPos) * dragForce / sqrDt
            // VRM spec: inertia = (currentTail - prevTail) * (1.0 - dragForce)
            // 現実装: prevPos = previousPosition + anchorDelta * inertiaKeep
            // これにより、親からの慣性伝播が不自然になる可能性がある
            // ============================================================
            hlslpp::float3 prevPos = node.previousPosition + anchorDelta * inertiaKeep;

            // Verlet積分
            const float    dragKeep = 1.0f - std::clamp(s.drag, 0.0f, 1.0f);
            hlslpp::float3 velocity = (node.currentPosition - prevPos) * dragKeep;
            hlslpp::float3 newPos   = node.currentPosition + velocity + gravityStep;

            node.previousPosition = node.currentPosition;

            // ============================================================
            // [BUG NOTE] stiffness実装は単純な線形補間
            // UnityChan: force += rotation * (boneAxis * stiffnessForce) / sqrDt
            // 現実装: newPos = newPos + (animatedTargetPos - newPos) * stiffness
            // これにより、dragとの相互作用が不自然になる可能性がある
            // ============================================================
            const float stiffness = std::clamp(s.stiffness, 0.0f, 1.0f);
            newPos                = newPos + (animatedTargetPos - newPos) * stiffness;

            // 長さ制約
            auto ApplyLengthConstraint = [&]() {
                // ============================================================
                // [BUG NOTE] ゼロ長ベクトルの正規化でNaNが発生する可能性
                // newPos == parentSimPosの場合、normalize()がNaNを返す
                // ============================================================
                hlslpp::float3 dir = hlslpp::normalize(newPos - parentSimPos);
                newPos             = parentSimPos + dir * node.restLength;
            };
            ApplyLengthConstraint();

            // 角度制限
            if(angleLimitRad > 0.0f) {
                // ============================================================
                // [BUG NOTE] 角度制限の参照方向が不適切
                // 現在: animatedTargetPos - parentSimPos (アニメーション姿勢からの角度)
                // UnityChan/VRM: boneAxis (ボーンの自然方向からの角度)
                // これにより、ボーンがアニメで曲がっている場合の角度制限が不自然になる
                // ============================================================
                hlslpp::float3 restDir = hlslpp::normalize(animatedTargetPos - parentSimPos);
                hlslpp::float3 curDir  = hlslpp::normalize(newPos - parentSimPos);

                float cosAngle = std::clamp(float(hlslpp::dot(restDir, curDir)), -1.0f, 1.0f);
                float angle    = std::acos(cosAngle);

                if(angle > angleLimitRad && angle > 1e-5f) {
                    float          t          = angleLimitRad / angle;
                    hlslpp::float3 clampedDir = NLerpDirection(restDir, curDir, t);
                    newPos                    = parentSimPos + clampedDir * node.restLength;
                }
            }

            // コライダー衝突解決
            for(u32 iter = 0; iter < s.collisionIterations; ++iter) {
                bool anyPushed = false;

                for(const auto& collider : chain.colliders) {
                    hlslpp::float3 colliderPos = collider.localOffset;
                    if(collider.attachNodeIndex != UINT32_MAX && collider.attachNodeIndex < animatedPoses.size()) {
                        const WorldPose& pose = animatedPoses[collider.attachNodeIndex];
                        // ============================================================
                        // [BUG NOTE] コライダー位置の回転適用
                        // hlslpp::mul(localOffset, rotation) の順序が正しいか要検証
                        // hlslpp++のmul(A,B)は「Aを適用してからBを適用する」仕様
                        // ベクトルにクォータニオンを掛ける場合は mul(rotation, vector) が正しい可能性
                        // ============================================================
                        colliderPos           = pose.position + hlslpp::mul(collider.localOffset, pose.rotation);
                    }

                    hlslpp::float3 diff    = newPos - colliderPos;
                    float          dist    = float(hlslpp::length(diff));
                    float          minDist = collider.radius + s.boneRadius;

                    if(dist < minDist && dist > 1e-6f) {
                        hlslpp::float3 pushDir = diff / dist;
                        newPos                 = colliderPos + pushDir * minDist;
                        anyPushed              = true;
                    }
                }

                if(!anyPushed) {
                    break;
                }
                ApplyLengthConstraint();
            }

            // 確定した位置とワールド回転を保存
            //    （バインドポーズ方向→現在方向へのswing回転をアニメ回転に合成。
            //    子ノードの計算には currentPosition のみ使うため、ツイストのズレは
            //    見た目に影響しない）
            node.currentPosition = newPos;

            hlslpp::float3 animRestDir  = hlslpp::normalize(animatedTargetPos - parentSimPos);
            hlslpp::float3 correctedDir = hlslpp::normalize(newPos - parentSimPos);

            hlslpp::quaternion swing = QuatFromToRotation(animRestDir, correctedDir);
            node.correctedRotation   = hlslpp::normalize(hlslpp::mul(animatedPoses[node.nodeIndex].rotation, swing));
        }
    }

}    // namespace Tsukino::Physics::SpringBonePhysics
