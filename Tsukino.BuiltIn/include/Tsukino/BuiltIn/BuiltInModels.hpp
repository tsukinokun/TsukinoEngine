//--------------------------------------------------------------
//! @file   BuiltInModels.hpp
//! @brief  ビルトインモデル（立方体・平面・球）集約クラスの宣言
//--------------------------------------------------------------
#pragma once
#include <Tsukino/Engine/Asset/AssetHandle.hpp>

// 名前空間 : Tsukino::Asset
namespace Tsukino::Asset {
    class AssetManager;    // 前方宣言
}

// 名前空間 : Tsukino::BuiltIn
namespace Tsukino::BuiltIn {
    //--------------------------------------------------------------
    //! @class   BuiltInModels
    //! @brief   ビルトインモデル（基本図形）集約クラス（Unity の GameObject.CreatePrimitive に当たる）
    //! @note    ModelComponent::modelHandle にそのまま入れて使う。いずれも原点中心・大きさ 1 で、
    //!          Transform の scale で拡大縮小する。当たり判定（CollisionComponent::extent）は
    //!          scale を見ないので、ワールドでの半分の大きさ（scale * 0.5）を別に入れる。
    //--------------------------------------------------------------
    class BuiltInModels {
    public:
        //--------------------------------------------------------------
        //! @brief ビルトインモデルを読み込みます。
        //! @param assetManager [in] アセットマネージャーへのポインタ
        //--------------------------------------------------------------
        void Initialize(Tsukino::Asset::AssetManager* assetManager);

        Tsukino::Asset::AssetHandle cube;      // 立方体（1 x 1 x 1、面ごとに平らな法線）
        Tsukino::Asset::AssetHandle plane;     // 平面（XZ 平面の 1 x 1、Y = 0、+Y 向き。裏からは見えない）
        Tsukino::Asset::AssetHandle sphere;    // 球（半径 0.5 = 直径 1）
    };
}    // namespace Tsukino::BuiltIn
