//--------------------------------------------------------------
//! @file   ModelData.hpp
//! @brief  モデルデータの構造体を定義
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
#include <Tsukino/GraphicsCommon/Node/NodeData.hpp>
#include <Tsukino/GraphicsCommon/Mesh/MeshData.hpp>
#include <Tsukino/GraphicsCommon/Material/MaterialData.hpp>
#include <vector>
#include <string>

// hlslpp のシリアライズ
namespace hlslpp {
    namespace interop {
        template <class Archive>
        void serialize(Archive& ar, float3& f) {
            ar(f.x, f.y, f.z);
        }
        template <class Archive>
        void serialize(Archive& ar, float4& f) {
            ar(f.x, f.y, f.z, f.w);
        }
    }    // namespace interop
}    // namespace hlslpp

// 名前空間 Tsukino::GraphicsCommon
namespace Tsukino::GraphicsCommon {
    //--------------------------------------------------------------
    //! @struct  VectorKey
    //! @brief キーフレーム（位置、回転、スケール）
    //--------------------------------------------------------------
    struct VectorKey {
        float                   time;
        hlslpp::interop::float3 value;
        //--------------------------------------------------------------
        //! @brief  シリアライズ関数
        //! @param  ar [out] アーカイブオブジェクト
        //--------------------------------------------------------------
        template <class Archive>
        void serialize(Archive& ar) {
            ar(time, value);
        }
    };

    //--------------------------------------------------------------
    //! @struct  QuaternionKey
    //! @brief   キーごとの回転
    //--------------------------------------------------------------
    struct QuaternionKey {
        float                   time;
        hlslpp::interop::float4 value;    // wを含む

        //--------------------------------------------------------------
        //! @brief  シリアライズ関数
        //! @param  ar [out] アーカイブオブジェクト
        //--------------------------------------------------------------
        template <class Archive>
        void serialize(Archive& ar) {
            ar(time, value);
        }
    };

    //--------------------------------------------------------------
    //! @struct AnimationChannel
    //! @brief  アニメーションチャンネル
    //--------------------------------------------------------------
    struct AnimationChannel {
        std::string                nodeName;    // どのノードを動かすか
        std::vector<VectorKey>     positionKeys;
        std::vector<QuaternionKey> rotationKeys;
        std::vector<VectorKey>     scaleKeys;

        //--------------------------------------------------------------
        //! @brief  シリアライズ関数
        //! @param  ar [out] アーカイブオブジェクト
        //--------------------------------------------------------------
        template <class Archive>
        void serialize(Archive& ar) {
            ar(nodeName, positionKeys, rotationKeys, scaleKeys);
        }
    };

    //--------------------------------------------------------------
    //! @struct AnimationData
    //! @brief  1つのアニメーションデータ
    //--------------------------------------------------------------
    struct AnimationData {
        std::string                   name;
        float                         duration;          // 全体の長さ
        float                         ticksPerSecond;    // 1秒あたりのチック数
        std::vector<AnimationChannel> channels;

        //--------------------------------------------------------------
        //! @brief  シリアライズ関数
        //! @param  ar [out] アーカイブオブジェクト
        //--------------------------------------------------------------
        template <class Archive>
        void serialize(Archive& ar) {
            ar(name, duration, ticksPerSecond, channels);
        }
    };

    //--------------------------------------------------------------
    //! @struct BoneInfo
    //! @brief  ボーンの情報
    //--------------------------------------------------------------
    struct BoneInfo {
        std::string      name;
        u32              nodeIndex;
        hlslpp::float4x4 inverseBindPose;
    };

    //--------------------------------------------------------------
    //! @struct SkeletonData
    //! @brief  スケルトン全体のデータ構造
    //--------------------------------------------------------------
    struct SkeletonData {
        std::vector<BoneInfo> bones;
    };

    //--------------------------------------------------------------
    //! @struct ModelData
    //! @brief  モデル全体のデータ構造
    //--------------------------------------------------------------
    struct ModelData {
        std::vector<NodeData>      nodes;
        std::vector<MeshData>      meshes;
        std::vector<MaterialData>  materials;
        std::vector<AnimationData> animations;    
        SkeletonData               skeleton;    // スケルトンデータ
        u32                        rootNodeIndex = 0;

        //--------------------------------------------------------------
        //! @brief  シリアライズ関数
        //! @param  ar [out] アーカイブオブジェクト
        //--------------------------------------------------------------
        template <class Archive>
        void serialize(Archive& ar) {
            ar(nodes, meshes, materials, animations, rootNodeIndex);
        }
    };

}    // namespace Tsukino::GraphicsCommon
