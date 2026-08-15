//-------------------------------------------------------------
//! @file   TpsCameraComponent.hpp
//! @brief  TpsCameraComponent構造体の宣言
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once
#include <Tsukino/Core/ECS/Entity/Entity.hpp>
// 名前空間 : ActionGame::ECS
namespace ActionGame::ECS {
    //-------------------------------------------------------------
    //! @struct TpsCameraComponent
    //! @brief  対象（プレイヤー）を中心にマウスで旋回する三人称視点カメラのパラメータ
    //-------------------------------------------------------------
    struct TpsCameraComponent {
        Tsukino::ECS::Entity target = entt::null;    //!< 追従対象のエンティティ（プレイヤー）

        float distance  = 300.0f;    //!< 対象からのカメラの距離
        // 対象のTransform位置は足元（CharacterControllerComponent.centerOffset採用後）を表す。
        // 頭の少し上あたりを狙うため、身長210cm相当を見込んだオフセットにしている
        float height     = 140.0f;    //!< 対象の位置（足元）からの基準高さオフセット（頭の少し上あたりを狙う）
        float lookHeight = 215.0f;    //!< 注視点の高さオフセット（頭の少し上あたりを見る）

        float followLerpSpeed = 8.0f;    //!< 位置追従の補間速度（大きいほど素早く追従する）

        // --- マウスによる旋回 ---
        float yaw   = 0.0f;    //!< 現在のカメラyaw（ラジアン。0でプレイヤーの初期正面=+Z方向を映す）
        float pitch = 0.2f;    //!< 現在のカメラpitch（ラジアン。正で見下ろし、負で見上げ）

        float mouseSensitivity = 0.0012f;    //!< マウス1ピクセル移動あたりの回転量（ラジアン）
        float minPitch          = -0.5f;      //!< pitchの下限（見上げすぎ防止）
        float maxPitch          = 1.3f;       //!< pitchの上限（見下ろしすぎ防止）

        // --- マウスキャプチャ（カーソル非表示＋中央固定）の状態 ---
        bool mouseCaptured        = true;     //!< true: カーソルを隠して旋回操作に使う（Escキーで切り替え可能）
        bool wasCapturedLastFrame = false;    //!< 直前フレームで実際にキャプチャされていたか（復帰時の誤入力防止用）
    };
}    // namespace ActionGame::ECS
