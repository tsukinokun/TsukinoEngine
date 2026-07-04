//-------------------------------------------------------------
//! @file   TerrainGenerationRequestComponent.hpp
//! @brief  地形生成リクエストコンポーネントの宣言
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once
#include <cstdint>
// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {
    //-------------------------------------------------------------------------
    // @enum  TerrainNoiseType
    // @brief 地形の高さを決めるノイズの種類
    //-------------------------------------------------------------------------
    enum class TerrainNoiseType {
        Flat,    // 高さ0の平坦な地形（デバッグ用）
        Noise    // フラクタルノイズによる起伏地形
    };

    //-------------------------------------------------------------------------
    // @struct TerrainGenerationRequestComponent
    // @brief  このコンポーネントが付いたエンティティに対し、
    //         HeightmapGenerationSystemが地形データを生成してCollisionComponentへ書き込む。
    //         処理後、このコンポーネント自体は削除される（ワンショット）。
    //-------------------------------------------------------------------------
    struct TerrainGenerationRequestComponent {
        float            amplitude      = 10.0f;    // 高さ方向のスケール
        float            noiseFrequency = 0.05f;    // ノイズの周波数（小さいほど緩やかな起伏）
        uint32_t         seed           = 0;        // 乱数シード
        TerrainNoiseType noiseType      = TerrainNoiseType::Noise;
    };
}    // namespace Tsukino::BuiltIn::ECS
