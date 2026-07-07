//-------------------------------------------------------------
//! @file   DotSpawnerComponent.hpp
//! @brief  ドットの生成設定コンポーネント
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once

// 名前空間 : WaterGame::ECS
namespace WaterGame::ECS {

    //-------------------------------------------------------------
    //! @struct DotSpawnerComponent
    //! @brief  地形の高さに沿ってドットをランダム配置するための設定
    //!         DotSpawnSystemが地形の物理Body準備後に1回だけ消費する
    //-------------------------------------------------------------
    struct DotSpawnerComponent {
        int          dotCount     = 50;        //!< 生成するドットの数
        float        areaHalfSize = 500.0f;    //!< スポーン範囲（原点中心の正方形の半辺長）※地形サイズに合わせて調整
        float        heightOffset = 20.0f;     //!< 地形表面からの高さオフセット（ドットが浮いて見える分）
        float        dotRadius    = 15.0f;     //!< 見た目上の半径（EatSystemの当たり判定にも使用）
        unsigned int seed         = 777;       //!< 乱数シード

        bool hasSpawned = false;    //!< 生成済みフラグ（一度だけ実行するためのガード）
    };

}    // namespace WaterGame::ECS
