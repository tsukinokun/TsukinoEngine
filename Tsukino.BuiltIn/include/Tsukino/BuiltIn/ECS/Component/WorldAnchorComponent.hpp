//-------------------------------------------------------------
//! @file   WorldAnchorComponent.hpp
//! @brief  WorldAnchorComponent構造体の宣言
//-------------------------------------------------------------
#pragma once
#include <Tsukino/Core/ECS/Entity/Entity.hpp>

#include <hlsl++.h>
// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {
    //-------------------------------------------------------------
    //! @struct WorldAnchorComponent
    //! @brief  3Dワールド座標にある対象(target)を追従し、そのスクリーン投影位置を
    //!         自分自身のTransformComponent.positionへ書き込むためのコンポーネント。
    //!         HPバー・ネームプレート・ダメージ数値・インタラクトプロンプトなど、
    //!         「ワールド座標に追従する画面UI」全般に使う汎用コンポーネント。
    //!         このコンポーネントを付けたエンティティ自身にTransformComponentと、
    //!         SpriteComponent／FontComponentなど実際に描画したいものを付けておくと、
    //!         WorldAnchorSystemが毎フレーム位置を更新してくれる（描画側は変更不要）
    //!
    //!         追従先はエンティティ（target）と固定ワールド座標（fixedWorldPosition）の
    //!         2通りから選べる。ダメージ数値・ヒットマーカーのように「対象が消えても
    //!         その場に残ってほしい」表示はuseFixedWorldPositionを使う
    //-------------------------------------------------------------
    struct WorldAnchorComponent {
        Tsukino::ECS::Entity target = entt::null;    // 追従対象エンティティ（TransformComponent必須。entt::nullなら非表示扱い）

        // ワールドの一点に貼り付けるモード。trueのときtargetは参照されず、
        // 対象エンティティが破棄されても表示位置が壊れない
        bool           useFixedWorldPosition = false;                              // trueならtargetを無視しfixedWorldPositionへ貼り付ける
        hlslpp::float3 fixedWorldPosition    = hlslpp::float3(0.0f, 0.0f, 0.0f);    // 貼り付け先のワールド座標

        hlslpp::float3 worldOffset  = hlslpp::float3(0.0f, 0.0f, 0.0f);    // 基準ワールド座標からのオフセット（例：頭上）
        hlslpp::float2 screenOffset = hlslpp::float2(0.0f, 0.0f);          // 投影後のピクセル単位の微調整オフセット

        bool visible = true;    // 直近フレームで画面内に投影できたか（WorldAnchorSystemが更新する。読み取り専用）
    };
}    // namespace Tsukino::BuiltIn::ECS
