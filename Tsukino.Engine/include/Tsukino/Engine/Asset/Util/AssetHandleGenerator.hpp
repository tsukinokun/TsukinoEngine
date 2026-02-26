//--------------------------------------------------------------
//! @file   AssetHandleGenerator.hpp
//! @brief  アセットハンドル生成ヘルパー
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
#include <random>
#include <Tsukino/Engine/Asset/AssetHandle.hpp>
// 名前空間 : Tsukino::Asset
namespace Tsukino::Asset {
    //--------------------------------------------------------------
    //! @class  AssetHandleGenerator
    //! @brief  アセットハンドル生成クラス
    //--------------------------------------------------------------
    class AssetHandleGenerator {
    public:
        //--------------------------------------------------------------
        //! @brief  アセットハンドルを生成する関数
        //! @return 生成されたアセットハンドル
        //--------------------------------------------------------------
        static AssetHandle Generate() {
            static std::random_device                 rd;           // ランダムデバイスの初期化
            static std::mt19937_64                    gen(rd());    // メルセンヌ・ツイスターの64ビット版の初期化
            static std::uniform_int_distribution<u64> dist;         // 0から最大のu64までの一様分布の初期化
            return AssetHandle(dist(gen));                          // ランダムなu64値を生成してアセットハンドルを返す
        }
    };
}    // namespace Tsukino::Asset
