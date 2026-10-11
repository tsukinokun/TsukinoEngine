//----------------------------------------------------------------------------
//! @file   UITweenUtility.hpp
//! @brief  画面UIのトゥイーン（UITweenComponent）を再生・停止するヘルパー
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/BuiltIn/ECS/Component/UITweenComponent.hpp>

#include <Tsukino/Core/ECS/Registry/Registry.hpp>

#include <algorithm>
#include <cstdint>

// 名前空間 : Tsukino::BuiltIn::ECS::UITweenUtility
namespace Tsukino::BuiltIn::ECS::UITweenUtility {

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
}    // namespace Tsukino::BuiltIn::ECS::UITweenUtility
