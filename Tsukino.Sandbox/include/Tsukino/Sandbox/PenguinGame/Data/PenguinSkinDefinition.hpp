//-------------------------------------------------------------
//! @file   PenguinSkinDefinition.hpp
//! @brief  PenguinSkinDefinitionクラスの宣言
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once
#include <array>
#include <string>
#include <Tsukino/Engine/Asset/AssetHandle.hpp>
// 名前空間 : PenguinGame::Data
namespace PenguinGame::Data{
    struct PenguinSkinDefinition {
        // スキン名（"Normal", "Santa", "Summer" など）
        std::string name;

        // 各部位のテクスチャセット
        std::array<Tsukino::Asset::AssetHandle, 2> centerTexs;
        std::array<Tsukino::Asset::AssetHandle, 2> leftTexs;
        std::array<Tsukino::Asset::AssetHandle, 2> rightTexs;
    };
}    // namespace PenguinGame::Data
