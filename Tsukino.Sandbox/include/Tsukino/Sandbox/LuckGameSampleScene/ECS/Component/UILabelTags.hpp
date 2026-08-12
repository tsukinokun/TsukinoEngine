//-------------------------------------------------------------
//! @file   UILabelTags.hpp
//! @brief  UIラベル識別用タグコンポーネントの宣言
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once

// 名前空間 : LuckGameSampleScene::ECS
namespace LuckGameSampleScene::ECS {

    //-------------------------------------------------------------
    //! @struct CpuHandLabelTag
    //! @brief  CPUの役表示ラベルであることを示すタグ
    //! @note   entt は空の構造体をストレージなしの「タグ」として最適化し、
    //!         Registry::AddComponent の T& 戻り値や view.each への参照渡しが
    //!         できなくなるため、ダミーメンバでそれを避けている
    //!         （DebugCameraTag と同じ回避策）。
    //-------------------------------------------------------------
    struct CpuHandLabelTag {
        bool dummy = true;
    };

    //-------------------------------------------------------------
    //! @struct PlayerHandLabelTag
    //! @brief  プレイヤーの役表示ラベルであることを示すタグ
    //-------------------------------------------------------------
    struct PlayerHandLabelTag {
        bool dummy = true;
    };

    //-------------------------------------------------------------
    //! @struct MessageLabelTag
    //! @brief  中央メッセージ表示ラベルであることを示すタグ
    //-------------------------------------------------------------
    struct MessageLabelTag {
        bool dummy = true;
    };

}    // namespace LuckGameSampleScene::ECS
