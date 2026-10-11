//----------------------------------------------------------------------------
//! @file   UITweenSystem.cpp
//! @brief  画面UIのトゥイーンとボタンの手ごたえのシステムの実装
//----------------------------------------------------------------------------
#include <Tsukino/EngineIntegration/ECS/System/UITweenSystem.hpp>

#include <Tsukino/BuiltIn/ECS/Component/PointerTargetComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/UIButtonFeedbackComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/UICanvasGroupComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/UIMotionComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/UITweenComponent.hpp>
#include <Tsukino/BuiltIn/ECS/UI/UIMotionUtility.hpp>

#include <Tsukino/Core/ECS/Registry/Registry.hpp>
#include <Tsukino/Core/Math/Easing.hpp>

#include <entt/entt.hpp>

#include <algorithm>
#include <cmath>
#include <vector>

// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {
    namespace {
        namespace Easing = Tsukino::Core::Math::Easing;

        //! 長さ 0 のトゥイーンで割り算しないための最小の長さ（秒）
        constexpr float kMinDuration = 1.0e-4f;

        //--------------------------------------------------------------
        //! トゥイーンの時間を進め、今の値を求めます。
        //! @param  [in,out] tween     トゥイーン（elapsed を進める）
        //! @param  [in]     deltaTime 前フレームからの経過時間（秒）
        //! @param  [out]    finished  Once のトゥイーンが終わったら true
        //! @return 今の値
        //--------------------------------------------------------------
        float AdvanceTween(UITween& tween, float deltaTime, bool& finished) {
            tween.elapsed += deltaTime;
            finished       = false;

            // 始まる前は始めの値
            const float local = tween.elapsed - tween.delay;
            if(local < 0.0f)
                return tween.from;

            const float duration = std::max(tween.duration, kMinDuration);
            float       t        = 0.0f;

            if(tween.loop == UITweenLoop::Once) {
                t = local / duration;
                if(t >= 1.0f) {
                    finished = true;
                    t        = 1.0f;
                }
            } else {
                // 1周の長さ（PingPong は行きと帰り）と、繰り返しの間の待ち
                const float active = (tween.loop == UITweenLoop::PingPong) ? duration * 2.0f : duration;
                const float cycle  = active + std::max(tween.repeatDelay, 0.0f);
                const float inside = std::fmod(local, cycle);

                // 待ちの間は始めの値
                if(inside >= active)
                    return tween.from;

                // PingPong の帰り道は、行きを逆にたどる
                t = (tween.loop == UITweenLoop::PingPong && inside > duration) ? 2.0f - inside / duration : inside / duration;
            }
            return tween.from + (tween.to - tween.from) * Easing::Evaluate(tween.ease, t);
        }

        //--------------------------------------------------------------
        //! 値の種類に合わせて、見た目のずれと透明度に合成します。
        //! @param  [in]     property 動かす値
        //! @param  [in]     value    値
        //! @param  [in,out] motion   見た目のずれ
        //! @param  [in,out] alpha    透明度
        //--------------------------------------------------------------
        void Apply(UITweenProperty property, float value, UIMotionComponent& motion, float& alpha) {
            switch(property) {
            case UITweenProperty::OffsetX:
                motion.offset.x += value;
                break;
            case UITweenProperty::OffsetY:
                motion.offset.y += value;
                break;
            case UITweenProperty::Scale:
                motion.scale.x *= value;
                motion.scale.y *= value;
                break;
            case UITweenProperty::ScaleX:
                motion.scale.x *= value;
                break;
            case UITweenProperty::ScaleY:
                motion.scale.y *= value;
                break;
            case UITweenProperty::Rotation:
                motion.rotation += value;
                break;
            case UITweenProperty::Alpha:
                alpha *= value;
                break;
            }
        }

        //--------------------------------------------------------------
        //! 見た目のずれを単位に戻します（中心は自分の位置）。
        //! @param  [out] motion 見た目のずれ
        //--------------------------------------------------------------
        void ResetMotion(UIMotionComponent& motion) {
            motion.offset      = hlslpp::float2(0.0f, 0.0f);
            motion.scale       = hlslpp::float2(1.0f, 1.0f);
            motion.rotation    = 0.0f;
            motion.pivotIsSelf = true;
        }
    }    // namespace

    //----------------------------------------------------------------------------
    //! トゥイーンとボタンの手ごたえを反映します。
    //----------------------------------------------------------------------------
    void UITweenSystem::Update(Tsukino::ECS::Registry& registry, float deltaTime) {
        //--------------------------------------------------------------
        // 足りないコンポーネントを先に付ける（View の反復中に付けるとプールが組み替わるため、集めてから付ける）
        //--------------------------------------------------------------
        std::vector<Tsukino::ECS::Entity> needMotion;
        std::vector<Tsukino::ECS::Entity> needGroup;
        registry.View<UITweenComponent>().each([&](Tsukino::ECS::Entity entity, const UITweenComponent& component) {
            if(!registry.HasComponent<UIMotionComponent>(entity))
                needMotion.push_back(entity);
            const bool hasAlpha =
                std::any_of(component.tweens.begin(), component.tweens.end(), [](const UITween& t) { return t.property == UITweenProperty::Alpha; });
            if(hasAlpha && !registry.HasComponent<UICanvasGroupComponent>(entity))
                needGroup.push_back(entity);
        });
        registry.View<UIButtonFeedbackComponent>().each([&](Tsukino::ECS::Entity entity, const UIButtonFeedbackComponent&) {
            if(!registry.HasComponent<UIMotionComponent>(entity))
                needMotion.push_back(entity);
        });
        auto addMissing = [&]() {
            for(Tsukino::ECS::Entity entity : needMotion) {
                if(!registry.HasComponent<UIMotionComponent>(entity))
                    registry.AddComponent<UIMotionComponent>(entity);
            }
            for(Tsukino::ECS::Entity entity : needGroup) {
                if(!registry.HasComponent<UICanvasGroupComponent>(entity))
                    registry.AddComponent<UICanvasGroupComponent>(entity);
            }
            needMotion.clear();
            needGroup.clear();
        };
        addMissing();

        // 写し元が透明度を持っていれば、写し先にも付ける（写し元に今付けた分も含めるため、上で付けてから調べる。
        // 同じフレームに付けないと、写し先だけ 1 フレーム不透明に見える）。写しの連鎖は2段まで扱うので2回調べる
        for(int pass = 0; pass < 2; ++pass) {
            registry.View<UIMotionComponent>().each([&](Tsukino::ECS::Entity entity, const UIMotionComponent& motion) {
                const Tsukino::ECS::Entity source = motion.source;
                if(source != entt::null && registry.IsValid(source) && registry.HasComponent<UICanvasGroupComponent>(source)
                   && !registry.HasComponent<UICanvasGroupComponent>(entity))
                    needGroup.push_back(entity);
            });
            addMissing();
        }

        //--------------------------------------------------------------
        // 1. 動かすものを単位に戻す
        //--------------------------------------------------------------
        registry.View<UIMotionComponent>().each([&](Tsukino::ECS::Entity entity, UIMotionComponent& motion) {
            if(registry.HasComponent<UITweenComponent>(entity) || registry.HasComponent<UIButtonFeedbackComponent>(entity)) {
                ResetMotion(motion);
                if(UICanvasGroupComponent* group = registry.try_get<UICanvasGroupComponent>(entity))
                    group->alpha = 1.0f;
            }
        });

        //--------------------------------------------------------------
        // 2. トゥイーンを進めて合成する
        //--------------------------------------------------------------
        registry.View<UITweenComponent, UIMotionComponent>().each([&](Tsukino::ECS::Entity entity, UITweenComponent& component, UIMotionComponent& motion) {
            float alpha       = 1.0f;
            bool  anyFinished = false;
            for(UITween& tween : component.tweens) {
                // 終わった Once は終わりの値を当ててから取り除く（Punch なら始めの値に戻っている。holdAtEnd なら残す）
                bool        finished = false;
                const float value    = AdvanceTween(tween, deltaTime, finished);
                Apply(tween.property, value, motion, alpha);
                if(finished && !tween.holdAtEnd) {
                    tween.duration = -1.0f;    // 取り除く印
                    anyFinished    = true;
                }
            }
            if(anyFinished) {
                component.tweens.erase(std::remove_if(component.tweens.begin(), component.tweens.end(), [](const UITween& t) { return t.duration < 0.0f; }),
                                       component.tweens.end());
            }

            if(UICanvasGroupComponent* group = registry.try_get<UICanvasGroupComponent>(entity))
                group->alpha = alpha;
        });

        //--------------------------------------------------------------
        // 3. ボタンの手ごたえ
        //--------------------------------------------------------------
        registry.View<UIButtonFeedbackComponent, UIMotionComponent>().each(
            [&](Tsukino::ECS::Entity entity, UIButtonFeedbackComponent& feedback, UIMotionComponent& motion) {
                const PointerTargetComponent* pointer = registry.try_get<PointerTargetComponent>(entity);

                // 目標の拡大率（押している > 重なっている > ふだん）
                float target = 1.0f;
                if(feedback.interactable && pointer) {
                    if(pointer->pressed)
                        target = feedback.pressScale;
                    else if(pointer->hovered)
                        target = feedback.hoverScale;

                    if(pointer->clicked && feedback.clickPunch > 0.0f)
                        feedback.punchElapsed = 0.0f;
                }
                feedback.currentScale = Easing::Damp(feedback.currentScale, target, feedback.speed, deltaTime);

                // クリックの弾み
                float punch = 0.0f;
                if(feedback.punchElapsed >= 0.0f) {
                    feedback.punchElapsed += deltaTime;
                    const float t          = feedback.punchElapsed / std::max(feedback.punchSeconds, kMinDuration);
                    if(t >= 1.0f)
                        feedback.punchElapsed = -1.0f;
                    else
                        punch = feedback.clickPunch * Easing::Punch(t);
                }

                const float scale  = feedback.currentScale + punch;
                motion.scale.x    *= scale;
                motion.scale.y    *= scale;
            });

        //--------------------------------------------------------------
        // 4. 写し元の動きを写す。
        //    自分のトゥイーンかボタンの手ごたえも持つもの（パネルの上のボタンなど）は、自分の動きのあとに写し元の動きを重ねる。
        //    写し元もまた写し先のもの（パネルを写すボタンを、さらに写すラベル）があるので、
        //    写し元が写し先でないものを先に、写し先であるものを後に処理する（2段まで）
        //--------------------------------------------------------------
        auto sourceOf = [&](Tsukino::ECS::Entity entity, const UIMotionComponent& motion) -> Tsukino::ECS::Entity {
            const Tsukino::ECS::Entity source = motion.source;
            if(source == entt::null || source == entity || !registry.IsValid(source) || !registry.HasComponent<UIMotionComponent>(source))
                return entt::null;
            return source;
        };
        auto isFollower = [&](Tsukino::ECS::Entity entity) { return sourceOf(entity, registry.GetComponent<UIMotionComponent>(entity)) != entt::null; };
        auto mirror     = [&](Tsukino::ECS::Entity entity, UIMotionComponent& motion) {
            const Tsukino::ECS::Entity source = sourceOf(entity, motion);
            if(source == entt::null)
                return;
            const UIMotionComponent*  from            = registry.try_get<UIMotionComponent>(source);
            const TransformComponent* sourceTransform = registry.try_get<TransformComponent>(source);
            if(!from || !sourceTransform)
                return;
            const hlslpp::float2 sourcePivot = UIMotionUtility::ResolvePivot(*from, *sourceTransform);

            const bool hasOwnMotion = registry.HasComponent<UITweenComponent>(entity) || registry.HasComponent<UIButtonFeedbackComponent>(entity);
            if(hasOwnMotion && registry.HasComponent<TransformComponent>(entity)) {
                //------------------------------------------------------
                // 自分の中心 p は、自分の動きで p + 自分の offset に移り、そこから写し元の動きで
                // 「写し元の中心を基準に拡大・回転して写し元の offset だけずらした位置」に移る。
                // 中心は自分のまま、拡大率は掛け合わせ、移った量を offset にする（回転は足すだけの近似）
                //------------------------------------------------------
                const hlslpp::float2 pivot = UIMotionUtility::ResolvePivot(motion, registry.GetComponent<TransformComponent>(entity));
                const hlslpp::float2 moved = pivot + motion.offset - sourcePivot;
                const float          c     = std::cos(from->rotation);
                const float          s     = std::sin(from->rotation);
                const hlslpp::float2 scaled(float(moved.x) * float(from->scale.x), float(moved.y) * float(from->scale.y));
                const hlslpp::float2 rotated(float(scaled.x) * c - float(scaled.y) * s, float(scaled.x) * s + float(scaled.y) * c);

                motion.offset      = sourcePivot + rotated + from->offset - pivot;
                motion.scale       = motion.scale * from->scale;
                motion.rotation    = motion.rotation + from->rotation;
                motion.pivot       = pivot;
                motion.pivotIsSelf = false;
            } else {
                motion.offset      = from->offset;
                motion.scale       = from->scale;
                motion.rotation    = from->rotation;
                motion.pivot       = sourcePivot;
                motion.pivotIsSelf = false;
            }

            const UICanvasGroupComponent* group = registry.try_get<UICanvasGroupComponent>(source);
            UICanvasGroupComponent*       own   = registry.try_get<UICanvasGroupComponent>(entity);
            if(group && own)
                own->alpha = hasOwnMotion ? own->alpha * group->alpha : group->alpha;
        };
        for(const bool sourceIsFollower : {false, true}) {
            registry.View<UIMotionComponent>().each([&](Tsukino::ECS::Entity entity, UIMotionComponent& motion) {
                const Tsukino::ECS::Entity source = sourceOf(entity, motion);
                if(source != entt::null && isFollower(source) == sourceIsFollower)
                    mirror(entity, motion);
            });
        }
    }
}    // namespace Tsukino::BuiltIn::ECS
