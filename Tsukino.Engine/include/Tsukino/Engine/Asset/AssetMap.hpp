//--------------------------------------------------------------
//! @file	AssetMap.hpp
//! @brief  アセットの格納場所のエイリアスを定義
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
#include <unordered_map>
#include <vector>
#include <Tsukino/Core/typedef.hpp>
#include <Tsukino/Core/Memory.hpp>
#include <Tsukino/Engine/Asset/IAsset.hpp>
// 名前空間 : Tsukino::Asset
namespace Tsukino::Asset {
    // アセットの格納場所
    using AssetMap = std::unordered_map<u64, Tsukino::Core::Ref<IAsset>>;
}
