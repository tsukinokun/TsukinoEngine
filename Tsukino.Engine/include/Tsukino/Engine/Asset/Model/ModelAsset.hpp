//-------------------------------------------------------------
//! @file   ModelAsset.hpp
//! @brief  モデルアセットの定義
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
#include <Tsukino/Engine/Asset/IAsset.hpp>

#include <Tsukino/GraphicsCommon/Model/ModelData.hpp>

#include <Tsukino/Core/Math/Matrix.hpp>

#include <vector>
#include <string>
// 名前空間 ：Tsukino::Asset
namespace Tsukino::Asset {
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
        //! @brief  アセットの種類を取得する関数
        //! @return アセットの種類
        //--------------------------------------------------------------
        [[nodiscard]] AssetType GetType() const override { return AssetType::Model; }

        // モデルデータ（メッシュ・スケルトン・アニメーション・マテリアルパス）
        Tsukino::GraphicsCommon::ModelData modelData;

        // マテリアルのランタイムハンドル（modelData.materials と同インデックス）
        std::vector<AssetHandle> materialHandles;
    };

}    // namespace Tsukino::Asset
