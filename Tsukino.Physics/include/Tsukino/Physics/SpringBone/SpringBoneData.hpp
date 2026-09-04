//--------------------------------------------------------------
//! @file   SpringBoneData.hpp
//! @brief  揺れ物(SpringBone)物理のデータ構造定義
//! @author 山﨑 愛
//--------------------------------------------------------------
#pragma once
#include <Tsukino/Core/typedef.hpp>
#include <hlsl++.h>
#include <string>
#include <vector>
#include <cereal/types/string.hpp>
#include <cereal/types/vector.hpp>

// 名前空間 Tsukino::Physics
namespace Tsukino::Physics {

    //--------------------------------------------------------------
    //! @struct WorldPose
    //! @brief  あるノードの「物理を考慮しない」アニメーション適用後のワールド姿勢
    //--------------------------------------------------------------
    struct WorldPose {
        hlslpp::float3     position = hlslpp::float3(0.0f, 0.0f, 0.0f);
        hlslpp::quaternion rotation = hlslpp::quaternion(0.0f, 0.0f, 0.0f, 1.0f);
    };

    //--------------------------------------------------------------
    //! @struct SpringColliderSphere
    //! @brief  球コライダー。特定ノードにアタッチして毎フレーム追従させる
    //--------------------------------------------------------------
    struct SpringColliderSphere {
        u32            attachNodeIndex = UINT32_MAX;    // アタッチ先ノード（UINT32_MAXならワールド固定）
        hlslpp::float3 localOffset     = hlslpp::float3(0.0f, 0.0f, 0.0f);
        float          radius          = 0.1f;

        // ============================================================
        // [NOTE] コライダーの回転処理
        //
        // SpringBonePhysics.cppで以下のように回転を適用している（これが正しい）:
        // colliderPos = pose.position + hlslpp::mul(collider.localOffset, pose.rotation);
        //
        // hlsl++はベクトルとクォータニオンの積に2つのオーバーロードを持ち、意味が異なる:
        //   mul(vector, quat) : 前方回転（q v q*）。UnityChanの `rotation * offset` に相当する
        //   mul(quat, vector) : 逆回転（_hlslpp_quat_mul_vec_inv_ps）。用途が違うので混同しないこと
        //
        // また、クォータニオン同士の積は行列積と合成順が逆になる:
        //   「Aを適用してからBを適用する」合成は、行列なら mul(A, B)、
        //   クォータニオンなら mul(B, A)（Hamilton積。後から効かせる側を左に置く）
        //   → hlslpp本体の unit_tests_quaternion.cpp「M_AB = M_A * M_B / Q_AB = Q_B * Q_A」参照
        // ============================================================

        //--------------------------------------------------------------
        //! @brief  シリアライズ関数
        //--------------------------------------------------------------
        template <class Archive>
        void serialize(Archive& ar) {
            ar(attachNodeIndex, localOffset.x, localOffset.y, localOffset.z, radius);
        }
    };

    //--------------------------------------------------------------
    //! @struct SpringBoneSettings
    //! @brief  チェーン単位で調整するパラメータ（KawaiiPhysicsのプロパティに相当）
    //--------------------------------------------------------------
    struct SpringBoneSettings {
        float          stiffness    = 0.6f;     //!< 元のアニメーション姿勢に戻ろうとする強さ (0-1)
        float          drag         = 0.15f;    //!< 減衰。大きいほど揺れがすぐ収まる (0-1)
        float          inertia      = 0.6f;     //!< ルート(アンカー)の移動をどれだけ子に伝えるか (0=完全に置いてかれる, 1=完全追従)
        float          gravityScale = 1.0f;     //!< 重力の強さ倍率
        hlslpp::float3 gravityDir   = hlslpp::float3(0.0f, -1.0f, 0.0f);

        // ============================================================
        // [TODO] boneAxis パラメータの追加が必要
        // UnityChanToonShader実装:
        //   public Vector3 boneAxis = new Vector3 (-1.0f, 0.0f, 0.0f);
        // VRM仕様:
        //   boneAxis: The direction of the bone in its rest state
        // 
        // 現在の実装は「アニメーション姿勢の方向」を基準に角度制限を行うが、
        // これはUnityChan/VRMと異なる振る舞いを示す。
        // 
        // 推奨: boneAxisを追加し、以下の点で使用する
        // 1. 角度制限の基準方向
        // 2. stiffnessの適用方向（boneAxis * stiffnessForce）
        // 3. 長さ制約の基準方向
        // ============================================================
        
        float boneRadius    = 0.03f;    //!< 各ボーンの太さ（コライダー衝突判定用）
        float angleLimitDeg = 45.0f;    //!< 親ボーン方向からの最大曲げ角度（0以下で無制限）

        u32 collisionIterations = 2;    //!< コライダー押し出し後に長さ制約を再適用する回数

        //--------------------------------------------------------------
        //! @brief  シリアライズ関数
        //--------------------------------------------------------------
        template <class Archive>
        void serialize(Archive& ar) {
            ar(stiffness, drag, inertia, gravityScale, gravityDir.x, gravityDir.y, gravityDir.z, boneRadius, angleLimitDeg, collisionIterations);
        }
    };

    //--------------------------------------------------------------
    //! @struct SpringBoneNode
    //! @brief  シミュレーション対象の1ボーン分のランタイム状態
    //--------------------------------------------------------------
    struct SpringBoneNode {
        u32 nodeIndex          = UINT32_MAX;    //!< ModelData.nodes 上のインデックス
        i32 parentIndexInChain = -1;            //!< チェーン内での親インデックス（-1ならアンカー直下）

        float restLength = 0.0f;    //!< 親からのバインドポーズ距離（伸びない棒として扱う）
                                    // ============================================================
                                    // [NOTE] restLength = 0 の場合、ApplyLengthConstraint()で
                                    // normalize()がNaNを生成するため、使用側で注意が必要
                                    // ============================================================

        hlslpp::float3     currentPosition   = hlslpp::float3(0.0f, 0.0f, 0.0f);              //!< 現在のシミュレーション位置(world)
        hlslpp::float3     previousPosition  = hlslpp::float3(0.0f, 0.0f, 0.0f);              //!< 1フレーム前のシミュレーション位置(world)
        hlslpp::quaternion correctedRotation = hlslpp::quaternion(0.0f, 0.0f, 0.0f, 1.0f);    //!< 補正後のワールド回転（他ノードの計算には未使用。書き戻し用）

        bool initialized = false;
    };

    //--------------------------------------------------------------
    //! @struct SpringBoneChain
    //! @brief  1つの根元(アンカー)からぶら下がる揺れ物チェーン（木構造も可）
    //--------------------------------------------------------------
    struct SpringBoneChain {
        std::string        name;                            //!< デバッグ表示用の名前（例: "Hair_L", "Skirt_Front"）
        u32                anchorNodeIndex = UINT32_MAX;    //!< 物理を受けない固定親（頭、腰など）
        SpringBoneSettings settings;

        std::vector<SpringBoneNode>       nodes;    //!< DFS順（親は必ず子より前）
        std::vector<SpringColliderSphere> colliders;

        // 直前フレームのアンカー位置（慣性計算用）
        hlslpp::float3 previousAnchorPosition = hlslpp::float3(0.0f, 0.0f, 0.0f);
        bool           anchorInitialized      = false;

        // ============================================================
        // [NOTE] コライダー衝突解決の注意点
        // 
        // UpdateChain()の衝突処理は、各イテレーションで:
        // 1. 全コライダーに対して衝突判定
        // 2. 衝突していればノードを押し出し
        // 3. 長さ制約を適用
        // 
        // ただし、複数のコライダーが重なる場合や、押し出し後に
        // 別のコライダーと衝突する場合、イテレーション内で
        // 再チェックされないため、位置が不自然に押し出されることがある。
        // 
        // 推奨: collisionIterationsを増やす、または反復的ソルバー方式に変更
        // ============================================================
    };

}    // namespace Tsukino::Physics
