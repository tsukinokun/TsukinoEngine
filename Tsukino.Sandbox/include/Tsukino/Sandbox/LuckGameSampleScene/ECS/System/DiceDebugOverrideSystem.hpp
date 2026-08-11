//-------------------------------------------------------------
//! @file   DiceDebugOverrideSystem.hpp
//! @brief  DiceDebugOverrideSystemクラスの宣言
//! @author 山﨑愛
//! @details フェーズ3の役判定検証用。物理挙動に頼らず、キー入力で
//!          強制的に出目を確定させ HandJudgeSystem の全パターンを
//!          素早く確認するためのデバッグ専用システム。
//!          _DEBUG ビルドでのみコンパイル・登録すること。
//!          動作確認が終わったら Scene 側の登録ごと削除してよい。
//-------------------------------------------------------------
#pragma once
#ifdef _DEBUG
#include <Tsukino/Core/ECS/System/ISystem.hpp>

// 名前空間 : LuckGameSampleScene::ECS
namespace LuckGameSampleScene::ECS {

    //-------------------------------------------------------------
    //! @class  DiceDebugOverrideSystem
    //! @brief  数字キー(1〜6)で役のパターンを強制的に出目確定させるデバッグシステム
    //! @details
    //!   D1: ピンゾロ (1,1,1)      D4: ヒフミ   (1,2,3)
    //!   D2: アラシ   (4,4,4)      D5: 目       (2,2,5)
    //!   D3: シゴロ   (4,5,6)      D6: 目なし   (1,2,4)
    //-------------------------------------------------------------
    class DiceDebugOverrideSystem : public Tsukino::ECS::ISystem {
    public:
        void Update(Tsukino::ECS::Registry& registry, float deltaTime) override;
    };

}    // namespace LuckGameSampleScene::ECS
#endif    // _DEBUG
