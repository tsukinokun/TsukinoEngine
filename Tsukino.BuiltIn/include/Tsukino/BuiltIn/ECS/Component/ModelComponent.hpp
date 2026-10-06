//-------------------------------------------------------------
//! @file   ModelComponent.hpp
//! @brief  ModelComponentクラスの宣言
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once
#include <Tsukino/Engine/Asset/AssetRef.hpp>

#include <hlsl++.h>

#include <vector>
// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {
    //-------------------------------------------------------------
    //! @struct ModelComponent
    //! @brief  3Dモデル描画に必要な情報を管理するコンポーネント
    //-------------------------------------------------------------
    struct ModelComponent {
        Tsukino::Asset::AssetRef modelHandle;       // ModelAsset への参照
        bool                     visible = true;    // 描画するかどうかのフラグ

        // 1.0未満にすると、通常のディファード（GBuffer）描画の代わりに
        // 半透明フォワード描画（RenderPass::TransparentDepth + Transparent）へ切り替わる。
        // ディファードのライティング結果はTonemapパスがrgbしか読まないため、
        // baseColorのアルファを下げてもディファード経路では一切フェードしない
        float opacity = 1.0f;

        // falseなら裏面を捨てる（カメラがめり込んでも内面が見えない）。
        // マントや髪など、片面ポリゴンを両側から見せたいモデルだけtrueにする
        bool doubleSided = false;

        // マテリアルの差し替え（Unity の MeshRenderer.sharedMaterials に当たる）。
        // 添字はモデルのマテリアルのスロット（メッシュの materialIndex）。有効なハンドルがあるスロットだけ
        // そのマテリアル（.tmat など）で描き、空・範囲外・無効のスロットはモデルのマテリアルのまま。
        // モデルのアセットは書き換えないので、同じモデルを使う別のエンティティには影響しない。
        // エンティティごとに色だけ変えたいときは MaterialPropertyBlockComponent を使う
        std::vector<Tsukino::Asset::AssetRef> materials;
    };

}    // namespace Tsukino::BuiltIn::ECS
