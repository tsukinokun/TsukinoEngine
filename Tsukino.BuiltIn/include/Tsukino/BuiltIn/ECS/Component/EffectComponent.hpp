#pragma once
//--------------------------------------------------------------
//! @file   EffectComponent.hpp
//! @brief  エフェクトコンポーネントの定義
//! @author 山﨑愛
//--------------------------------------------------------------
#include <Tsukino/Engine/Asset/AssetHandle.hpp>
#include <Tsukino/Core/Path.hpp>

// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {

    //--------------------------------------------------------------
    //! @struct  EffectComponent
    //! @brief  Effekseerエフェクトを再生するためのコンポーネント
    //--------------------------------------------------------------
    struct EffectComponent {
        Tsukino::Asset::AssetHandle effectAsset;          //!< .efk アセット
        Tsukino::Core::Path         effectPath;           //!< エフェクトファイルのパス
        int                         handle    = -1;       //!< Effekseer インスタンスハンドル
        float                       playSpeed = 1.0f;     //!< 再生速度
        bool                        looping   = false;    //!< ループ再生フラグ
        bool                        stopped   = false;    //!< 停止フラグ
        bool                        active    = false;    //!< 再生中フラグ

        //--------------------------------------------------------------
        // エフェクトの制作単位とワールドの単位系が食い違う場合の変換に使う拡大率
        // （例：1ユニット≒1cmのワールドでメートル単位のエフェクトを再生するなら100前後）。
        // EffectSystem::PlayEffectへそのまま渡すため、再生を開始する瞬間の1回だけ効く
        //--------------------------------------------------------------
        float scale = 1.0f;    //!< 再生スケール（等倍=1.0）

        //--------------------------------------------------------------
        // 位置（TransformComponent::position）は常に毎フレーム追従させるが、姿勢は既定では
        // 追従しない。飛翔体のように進行方向を向かせたい場合だけこれを立てる
        //--------------------------------------------------------------
        bool followRotation = false;    //!< trueならTransformComponent::rotationも毎フレーム反映する
    };
}    // namespace Tsukino::BuiltIn::ECS