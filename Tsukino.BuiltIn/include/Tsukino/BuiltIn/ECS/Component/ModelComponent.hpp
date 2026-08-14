//-------------------------------------------------------------
//! @file   ModelComponent.hpp
//! @brief  ModelComponentクラスの宣言
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once
#include <Tsukino/Engine/Asset/AssetRef.hpp>

#include <hlsl++.h>
// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {
    //-------------------------------------------------------------
    //! @struct ModelComponent
    //! @brief  3Dモデル描画に必要な情報を管理するコンポーネント
    //-------------------------------------------------------------
    struct ModelComponent {
        Tsukino::Asset::AssetRef modelHandle;       // ModelAsset への参照
        bool                     visible = true;    // 描画するかどうかのフラグ
    };

}    // namespace Tsukino::BuiltIn::ECS
