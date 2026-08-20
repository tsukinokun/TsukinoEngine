//-------------------------------------------------------------
//! @file   AnimationPlayerComponent.hpp
//! @brief  AnimationPlayerComponentクラスの宣言
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once
#include <Tsukino/Engine/Asset/AssetHandle.hpp>
#include <string>
// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {
    //-------------------------------------------------------------
    //! @class  AnimationPlayerComponent
    //! @brief  アニメーションの再生状態を管理するクラス
    //-------------------------------------------------------------
    struct AnimationPlayerComponent {
        Tsukino::Asset::AssetHandle current_clip_id;    // リソースマネージャ内のID
        u32                         animation_index = 0;// 再生するアニメーションのインデックス
        float                       elapsed_time;       // 現在の再生時間（秒）
        float                       playback_speed;     // 1.0f = 等速
        bool                        is_looping;         // ループ再生フラグ
        bool                        is_playing;         // 一時停止フラグ

        // ループしない（is_looping=false）クリップが最後まで再生し終えたか。
        // AnimationSystemが毎フレーム再計算するポーリング値であり、終了した瞬間だけtrueになる
        // イベントではない（クリップが切り替わるまでtrueが継続する）
        bool                        is_finished = false;

        // クリップ内の再生レンジ（秒）。1本のFBXから連撃の各段のような部分区間を切り出して
        // 再生するために使う。既定値（0, 0）ではクリップ全体を再生し、従来と同じ挙動になる。
        // elapsed_timeは「このレンジ先頭からの経過秒」であり、クリップ先頭からの絶対時刻ではない
        float                       clip_start_time = 0.0f;    // レンジ開始（秒）
        float                       clip_end_time   = 0.0f;    // レンジ終了（秒）。0以下ならクリップ末尾まで

        // In Place再生。trueの間、ルートモーションノード（既定ではスケルトン最上位ボーン＝Hips）の
        // 水平移動キーを無視し、固定位置へ差し替える。移動はキャラクターコントローラが担当する
        // 設計のため、クリップ側の前進成分がコリジョンとのズレになるのを防ぐ
        bool                        in_place = false;

        // ルートモーションを持つノード名（Mixamoなら"mixamorig:Hips"）。
        // 空ならAnimationSystemがスケルトン内で最も浅いボーンノードを自動判定する
        std::string                 root_motion_node_name;
        u32                         root_motion_node_index = UINT32_MAX;    // 解決済みキャッシュ
        bool                        root_motion_resolved   = false;        // 解決済みか（SpringBoneComponent::resolvedと同じ運用）

        // In Placeの固定基準。in_place（または遷移中のoutgoing.in_place）が最初に有効になった
        // 瞬間の生のHips位置を凍結して保持し、以後クリップが切り替わっても（連撃の各段など）
        // 同じ基準を使い続ける。毎回「そのクリップのレンジ先頭」を基準にすると、1本の連続した
        // モーションを時間レンジで複数段に分割している場合に、段の切り替わりごとに元の
        // モーションが持つ前進量ぶんだけ位置がジャンプしてしまう（コンボ中に何度も
        // カクカク前進して見える不具合の原因だった）
        bool                        root_motion_lock_active = false;
        float                       root_motion_lock_x      = 0.0f;
        float                       root_motion_lock_z      = 0.0f;
    };
}    // namespace Tsukino::BuiltIn::ECS
