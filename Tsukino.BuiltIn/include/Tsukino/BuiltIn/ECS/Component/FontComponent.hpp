//-------------------------------------------------------------
//! @file   FontComponent.hpp
//! @brief  FontComponentクラスの宣言
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once
#include <Tsukino/Engine/Asset/AssetHandle.hpp>

#include <hlsl++.h>
#include <string>
// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {
    //-------------------------------------------------------------
    //! @struct FontComponent
    //! @brief  フォント描画に必要な情報を管理するコンポ
    //-------------------------------------------------------------
    struct FontComponent {
        std::wstring                text;                     // 描画するテキスト
        Tsukino::Asset::AssetHandle fontHandle;               // フォントアセットのハンドル
        hlslpp::float4              color  = {1, 1, 1, 1};    // フォントの色
        hlslpp::float2              origin = {0, 0};          // 回転の中心点などの微調整用
    };
}    // namespace Tsukino::BuiltIn::ECS
