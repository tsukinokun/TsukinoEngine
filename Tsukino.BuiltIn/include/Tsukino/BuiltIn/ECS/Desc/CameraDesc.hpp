//--------------------------------------------------------------
//! @file   CameraDesc.hpp
//! @brief  カメラコンポーネント生成用の純粋な設計図（Desc）定義
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
#include <Tsukino/BuiltIn/ECS/Component/CameraComponent.hpp>

#include <hlsl++.h>
// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {

    //--------------------------------------------------------------
    //! @struct CameraDesc
    //! @brief  JSONから読み込むためのカメラ初期化パラメータ（cereal非依存）
    //--------------------------------------------------------------
    struct CameraDesc {
        // 投影タイプ
        CameraComponent::ProjectionType projectionType = CameraComponent::ProjectionType::Perspective;

        float orthoSize   = 720.0f;          // Ortho用の表示幅(ウィンドウの縦ピクセル数)
        float fov         = 45.0f;           // 垂直画角
        float aspectRatio = 16.0f / 9.0f;    // 画面の幅と高さの比率
        float nearZ       = 0.1f;            // ニアクリップ距離
        float farZ        = 1000.0f;         // ファークリップ距離

        bool           useLookAt    = false;                               // trueなら指定した座標を向き続ける
        hlslpp::float3 lookAtTarget = hlslpp::float3(0.0f, 0.0f, 0.0f);    // 注視する座標
        bool           isPrimary    = true;                                // メインカメラとして使用するか
    };

}    // namespace Tsukino::BuiltIn::ECS
