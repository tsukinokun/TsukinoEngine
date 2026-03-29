//-------------------------------------------------------------
//! @file   ModelAsset.hpp
//! @brief  モデルアセットの定義
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
#include <Tsukino/Engine/Asset/IAsset.hpp>

#include <Tsukino/Core/Math/Matrix.hpp>

#include <vector>
#include <string>
// 名前空間 ：Tsukino::Asset
namespace Tsukino::Asset {
    //--------------------------------------------------------------
    //! @struct RenderUnit
    //! @brief  描画に必要なメッシュとマテリアルの組み合わせを表す構造体
    //--------------------------------------------------------------
    struct RenderUnit {
        AssetHandle meshHandle;        // MeshAssetへのID
        AssetHandle materialHandle;    // MaterialAssetへのID
    };

    //--------------------------------------------------------------
    //! @struct ModelNode
    //! @brief  モデルの階層構造を表すノード
    //--------------------------------------------------------------
    struct ModelNode {
        std::string                 name;         // ノード名（"Arm_L" など）
        Tsukino::Core::Math::matrix transform;    // 親ノードからの相対座標（Local Transform）

        std::vector<RenderUnit> renderUnits;    // このノードが持つ描画パーツ

        int              parentIndex = -1;    // 親ノードのインデックス
        std::vector<int> children;            // 子ノードのインデックス
    };

    //--------------------------------------------------------------
    //! @class  ModelAsset
    //! @brief  複数のメッシュとマテリアルを統括するアセット
    //--------------------------------------------------------------
    class ModelAsset : public IAsset {
    public:
        //--------------------------------------------------------------
        //! @brief  デフォルトコンストラクタ
        //--------------------------------------------------------------
        ModelAsset() = default;

        //--------------------------------------------------------------
        //! @brief  引数付きコンストラクタ
        //! @param  handle  アセットのハンドル
        //--------------------------------------------------------------
        [[nodiscard]] AssetHandle GetHandle() const override { return m_handle; }

        //--------------------------------------------------------------
        //! @brief  アセットの種類を取得する関数
        //! @return アセットの種類
        //--------------------------------------------------------------
        [[nodiscard]] AssetType GetType() const override { return AssetType::Model; }

        //--------------------------------------------------------------
        //! @brief  ハンドルを設定する関数
        //! @param  handle  設定するハンドル
        //--------------------------------------------------------------
        void SetHandle(AssetHandle handle) { m_handle = handle; }

        // 全ノードのリスト（フラットに持っておくと更新処理が速い）
        std::vector<ModelNode> nodes;

        // 全体の境界ボックス（カリング用）
        // BoundingBox aabb;

        // アニメーションデータ（将来的にここへ追加）
        // std::vector<AnimationData> animations;

    private:
        AssetHandle m_handle;    // アセットのハンドル
    };

}    // namespace Tsukino::Asset
