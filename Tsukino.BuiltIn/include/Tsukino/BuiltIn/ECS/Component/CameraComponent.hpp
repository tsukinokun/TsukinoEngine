//-------------------------------------------------------------
//! @file   CameraComponent.hpp
//! @brief  CameraComponentクラスの宣言
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once
#include <Tsukino/Core/Math/Matrix.hpp>
// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {
    //-------------------------------------------------------------
    //! @struct CameraComponent
    //! @brief  カメラの位置や投影パラメータを管理
    //-------------------------------------------------------------
    struct CameraComponent {
        //-------------------------------------------------------------
        //! @enum   投影タイプ
        //! @brief  カメラの投影方法を指定 (Perspective: 3D用、Orthographic: 2D用)
        //-------------------------------------------------------------
        enum class ProjectionType {
            Perspective,    // 3D用
            Orthographic    // 2D用
        };

        // 投影タイプ
        ProjectionType projectionType = ProjectionType::Perspective;

        // Ortho用の表示幅(ウィンドウの縦ピクセル数)
        float orthoSize = 720.0f;

        // --- 投影パラメータ ---
        float fov         = 45.0f;           // 垂直画角
        float aspectRatio = 16.0f / 9.0f;    // 画面の幅と高さの比率
        float nearZ       = 0.1f;            // ニアクリップ距離
        float farZ        = 1000.0f;         // ファークリップ距離

        bool           useLookAt    = false;                      // trueなら指定した座標を向き続ける
        hlslpp::float3 lookAtTarget = hlslpp::float3(0, 0, 0);    // 注視する座標

        // --- 行列キャッシュ ---
        Tsukino::Core::Math::matrix viewMatrix;          // ビュー行列
        Tsukino::Core::Math::matrix projectionMatrix;    // 射影行列
        Tsukino::Core::Math::matrix viewProjMatrix;      // ビュー射影行列

        // --- 制御フラグ ---
        bool isPrimary = true;    // メインカメラとして使用するか
        bool dirty     = true;    // 行列が更新されているか
    };
}    // namespace Tsukino::BuiltIn::ECS
