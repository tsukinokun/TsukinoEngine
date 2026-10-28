//----------------------------------------------------------------------------
//! @file   UITweenUtility.hpp
//! @brief  画面UIのトゥイーン（UITweenComponent）を再生・停止するヘルパー
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/BuiltIn/ECS/Component/UITweenComponent.hpp>

#include <Tsukino/Core/ECS/Registry/Registry.hpp>

#include <hlsl++.h>

#include <algorithm>
#include <cstdint>

// 名前空間 : Tsukino::BuiltIn::ECS::UITweenUtility
namespace Tsukino::BuiltIn::ECS::UITweenUtility {

    //! 下の定型（SlideIn など）が使う tag です。
    //! @note   ゲームが自分のトゥイーンに使う tag と重ならないよう、0xFFFFFF00 から上をエンジンが使う。
    //!         ゲームの tag はこれより小さい値にすること
    namespace Tag {
        inline constexpr std::uint32_t kSlideX = 0xFFFFFF00u;    // SlideIn・SlideOut の左右のずれ
        inline constexpr std::uint32_t kSlideY = 0xFFFFFF01u;    // SlideIn・SlideOut の上下のずれ
        inline constexpr std::uint32_t kFade   = 0xFFFFFF02u;    // SlideIn・SlideOut・FadeIn・FadeOut の透明度
        inline constexpr std::uint32_t kPunch  = 0xFFFFFF03u;    // Punch の拡大率
    }    // namespace Tag

    //! トゥイーンを再生します。
    //! @param  [in] registry ECS レジストリ
    //! @param  [in] entity   動かすエンティティ
    //! @param  [in] tween    トゥイーン（elapsed は 0 から始め直す）
    //! @note   UITweenComponent が無ければ付ける。tag が 0 でなく、同じ tag のトゥイーンが再生中なら置き換える
    inline void Play(Tsukino::ECS::Registry& registry, Tsukino::ECS::Entity entity, const UITween& tween) {
        if(!registry.IsValid(entity))
            return;

        UITweenComponent& component =
            registry.HasComponent<UITweenComponent>(entity) ? registry.GetComponent<UITweenComponent>(entity) : registry.AddComponent<UITweenComponent>(entity);
        UITween started = tween;
        started.elapsed = 0.0f;

        if(tween.tag != 0) {
            auto it = std::find_if(component.tweens.begin(), component.tweens.end(), [&](const UITween& t) { return t.tag == tween.tag; });
            if(it != component.tweens.end()) {
                *it = started;
                return;
            }
        }
        component.tweens.push_back(started);
    }

    //! 指定した tag のトゥイーンを止めて取り除きます（値は次の UITweenSystem の更新で元に戻る）。
    //! @param  [in] registry ECS レジストリ
    //! @param  [in] entity   エンティティ
    //! @param  [in] tag      止めるトゥイーンの tag
    inline void Stop(Tsukino::ECS::Registry& registry, Tsukino::ECS::Entity entity, std::uint32_t tag) {
        if(!registry.IsValid(entity) || !registry.HasComponent<UITweenComponent>(entity))
            return;
        auto& tweens = registry.GetComponent<UITweenComponent>(entity).tweens;
        tweens.erase(std::remove_if(tweens.begin(), tweens.end(), [&](const UITween& t) { return t.tag == tag; }), tweens.end());
    }

    //! 指定した tag のトゥイーンが再生中かを返します。
    //! @param  [in] registry ECS レジストリ
    //! @param  [in] entity   エンティティ
    //! @param  [in] tag      調べるトゥイーンの tag
    //! @return 再生中なら true
    [[nodiscard]] inline bool IsPlaying(Tsukino::ECS::Registry& registry, Tsukino::ECS::Entity entity, std::uint32_t tag) {
        if(!registry.IsValid(entity) || !registry.HasComponent<UITweenComponent>(entity))
            return false;
        const auto& tweens = registry.GetComponent<UITweenComponent>(entity).tweens;
        return std::any_of(tweens.begin(), tweens.end(), [&](const UITween& t) { return t.tag == tag; });
    }

    //! 透明から現れさせます。
    //! @param  [in] registry ECS レジストリ
    //! @param  [in] entity   エンティティ
    //! @param  [in] delay    始めるまでの待ち（秒）
    //! @param  [in] seconds  長さ（秒）
    inline void FadeIn(Tsukino::ECS::Registry& registry, Tsukino::ECS::Entity entity, float delay = 0.0f, float seconds = 0.3f) {
        UITween tween;
        tween.property  = UITweenProperty::Alpha;
        tween.from      = 0.0f;
        tween.to        = 1.0f;
        tween.duration  = seconds;
        tween.delay     = delay;
        tween.ease      = Tsukino::Core::Math::Easing::EaseType::OutCubic;
        tween.holdAtEnd = true;
        tween.tag       = Tag::kFade;
        Play(registry, entity, tween);
    }

    //! 透明にして消えさせます（透明のまま残る。破棄は呼ぶ側で行う）。
    //! @param  [in] registry ECS レジストリ
    //! @param  [in] entity   エンティティ
    //! @param  [in] delay    始めるまでの待ち（秒）
    //! @param  [in] seconds  長さ（秒）
    inline void FadeOut(Tsukino::ECS::Registry& registry, Tsukino::ECS::Entity entity, float delay = 0.0f, float seconds = 0.3f) {
        UITween tween;
        tween.property  = UITweenProperty::Alpha;
        tween.from      = 1.0f;
        tween.to        = 0.0f;
        tween.duration  = seconds;
        tween.delay     = delay;
        tween.ease      = Tsukino::Core::Math::Easing::EaseType::InCubic;
        tween.holdAtEnd = true;
        tween.tag       = Tag::kFade;
        Play(registry, entity, tween);
    }

    //! 離れた所から本来の位置へ滑り込ませ、同時に透明から現れさせます。
    //! @param  [in] registry ECS レジストリ
    //! @param  [in] entity   エンティティ
    //! @param  [in] from     始めのずれ（本来の位置からの UI 座標の差。画面外を指せば画面外から入る）
    //! @param  [in] delay    始めるまでの待ち（秒）
    //! @param  [in] seconds  長さ（秒）
    //! @param  [in] ease     動きの緩急（既定は少し行き過ぎて戻る OutBack）
    inline void SlideIn(Tsukino::ECS::Registry& registry, Tsukino::ECS::Entity entity, const hlslpp::float2& from, float delay = 0.0f,
                        float seconds = 0.45f, Tsukino::Core::Math::Easing::EaseType ease = Tsukino::Core::Math::Easing::EaseType::OutBack) {
        UITween tween;
        tween.duration  = seconds;
        tween.delay     = delay;
        tween.ease      = ease;
        tween.holdAtEnd = true;

        tween.property = UITweenProperty::OffsetX;
        tween.from     = from.x;
        tween.to       = 0.0f;
        tween.tag      = Tag::kSlideX;
        Play(registry, entity, tween);

        tween.property = UITweenProperty::OffsetY;
        tween.from     = from.y;
        tween.tag      = Tag::kSlideY;
        Play(registry, entity, tween);

        FadeIn(registry, entity, delay, seconds);
    }

    //! 本来の位置から離れた所へ滑り出させ、同時に透明にします（透明のまま残る。破棄は呼ぶ側で行う）。
    //! @param  [in] registry ECS レジストリ
    //! @param  [in] entity   エンティティ
    //! @param  [in] to       終わりのずれ（本来の位置からの UI 座標の差）
    //! @param  [in] delay    始めるまでの待ち（秒）
    //! @param  [in] seconds  長さ（秒）
    //! @param  [in] ease     動きの緩急（既定はだんだん速くなる InCubic）
    inline void SlideOut(Tsukino::ECS::Registry& registry, Tsukino::ECS::Entity entity, const hlslpp::float2& to, float delay = 0.0f,
                         float seconds = 0.35f, Tsukino::Core::Math::Easing::EaseType ease = Tsukino::Core::Math::Easing::EaseType::InCubic) {
        UITween tween;
        tween.duration  = seconds;
        tween.delay     = delay;
        tween.ease      = ease;
        tween.holdAtEnd = true;

        tween.property = UITweenProperty::OffsetX;
        tween.from     = 0.0f;
        tween.to       = to.x;
        tween.tag      = Tag::kSlideX;
        Play(registry, entity, tween);

        tween.property = UITweenProperty::OffsetY;
        tween.to       = to.y;
        tween.tag      = Tag::kSlideY;
        Play(registry, entity, tween);

        FadeOut(registry, entity, delay, seconds);
    }

    //! 一瞬大きくして元に戻し、弾ませます。
    //! @param  [in] registry ECS レジストリ
    //! @param  [in] entity   エンティティ
    //! @param  [in] amount   いちばん大きいときに拡大率へ足す量
    //! @param  [in] seconds  長さ（秒）
    inline void Punch(Tsukino::ECS::Registry& registry, Tsukino::ECS::Entity entity, float amount = 0.25f, float seconds = 0.45f) {
        UITween tween;
        tween.property = UITweenProperty::Scale;
        tween.from     = 1.0f;
        tween.to       = 1.0f + amount;
        tween.duration = seconds;
        tween.ease     = Tsukino::Core::Math::Easing::EaseType::Punch;
        tween.tag      = Tag::kPunch;
        Play(registry, entity, tween);
    }
}    // namespace Tsukino::BuiltIn::ECS::UITweenUtility
