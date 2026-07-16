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

        Visit(anchorNodeIndex, -1, 0);
        return chain;
    }

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

        Visit(rootNodeIndex, -1, 1);
        return chain;
    }

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

            // 慣性補正：親(アンカー)の移動を previousPosition にも一部反映
            hlslpp::float3 prevPos = node.previousPosition + anchorDelta * inertiaKeep;

            // Verlet積分
            const float    dragKeep = 1.0f - std::clamp(s.drag, 0.0f, 1.0f);
            hlslpp::float3 velocity = (node.currentPosition - prevPos) * dragKeep;
            hlslpp::float3 newPos   = node.currentPosition + velocity + gravityStep;

            node.previousPosition = node.currentPosition;

            // stiffness：アニメーション本来の位置に一定割合戻す
            const float stiffness = std::clamp(s.stiffness, 0.0f, 1.0f);
            newPos                = newPos + (animatedTargetPos - newPos) * stiffness;

            // 長さ制約
            auto ApplyLengthConstraint = [&]() {
                hlslpp::float3 dir = hlslpp::normalize(newPos - parentSimPos);
                newPos             = parentSimPos + dir * node.restLength;
            };
            ApplyLengthConstraint();

            // 角度制限
            if(angleLimitRad > 0.0f) {
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
