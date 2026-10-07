//----------------------------------------------------------------------------
//! @file   ScreenModelComponent.hpp
//! @brief  3D モデルを画面の UI の層（画面スプライト・文字と同じ層）に描くコンポーネント
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/Core/ECS/EntityRef/EntityRef.hpp>

#include <hlsl++.h>

// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {

    //! 付けたエンティティと、その子孫のモデルを、ワールドではなく画面の UI の層に描く設定です
    //! （UI の中にアイテムやキャラクターの 3D モデルを出すときに使う）。
    //! @note ModelSystem が、モデルの持ち主か、その親をたどった先にこのコンポーネントを見つけると、
    //!       そのモデルを RenderPass::Overlay へ sortOrder 付きで積む。画面スプライト・文字と同じ並べ方なので、
    //!       「板（スプライト）→ モデル → 文字（スプライト）」のような重ね順を sortOrder だけで決められる。
    //!       ワールドには描かれず、影も落とさない。
    //!
    //!       見え方: このコンポーネントを持つエンティティのワールド位置に向けた正射影カメラで、
    //!       手前（+z の側）から -z の向きに見て、その位置を screenPosition に描く。
    //!       Transform は通常の 3D のまま（上が +y）なので、回転・拡縮・子のエンティティもそのまま使え、
    //!       ディレクショナルライトと空の環境光（IBL）はワールドと同じ向きから当たる（影と点光源は効かない）。
    //!       エンティティをワールドのどこに置いても、ワールド側の見た目には影響しない。
    //!
    //!       anchor に UI の部品（画面スプライトと同じ画面ピクセルの Transform を持つエンティティ）を指定すると、
    //!       その部品の画面上の位置に screenPosition を足した所に描き、部品の祖先に UIClipComponent があればその枠で切り取る。
    //!       スクロールする一覧の中身の子に anchor を置けば、モデルも行と一緒に動き、枠の外では切れる。
    //!
    //!       同じ sortOrder のモデル同士の前後は奥行きで決まる。sortOrder の違うモデル同士や、スプライト・文字との前後は
    //!       sortOrder で決まる（sortOrder が変わるたびに奥行きを消し直すので、重ね順の違うモデルが互いに隠れ合わない）。
    //!       ModelComponent::opacity は不透明度としてそのまま効く。
    struct ScreenModelComponent {
        hlslpp::float2          screenPosition = hlslpp::float2(0.0f, 0.0f);    // 描く位置（画面ピクセル。左上が原点、下が +y。画面スプライトと同じ）。anchor があるときは anchor の位置からのずらし量
        float                   pixelsPerUnit  = 1.0f;                          // モデルの 1unit を何ピクセルで描くか
        int                     sortOrder      = 0;                             // 画面スプライト・文字との重ね順（小さいほど奥）
        Tsukino::ECS::EntityRef anchor;                                         // 位置と切り取りの基準にする UI の部品（無ければ screenPosition の位置に描く）
    };
}    // namespace Tsukino::BuiltIn::ECS
