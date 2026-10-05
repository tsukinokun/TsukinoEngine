//-------------------------------------------------------------
//! @file   ScrollViewSystem.cpp
//! @brief  ScrollViewSystemクラスの実装
//-------------------------------------------------------------
#include <Tsukino/EngineIntegration/ECS/System/ScrollViewSystem.hpp>
#include <Tsukino/EngineIntegration/EngineContext.hpp>

#include <Tsukino/BuiltIn/ECS/Component/ScrollBarComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/ScrollViewComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/SpriteComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/UIClipComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Transform/TransformUtility.hpp>
#include <Tsukino/BuiltIn/ECS/UI/UIClipUtility.hpp>

#include <Tsukino/Engine/Asset/AssetManager.hpp>
#include <Tsukino/Engine/Asset/Texture/TextureAsset.hpp>

#include <Tsukino/Core/Input/InputSystem.hpp>
#include <Tsukino/Core/ECS/Registry/Registry.hpp>

#include <hlsl++.h>
#include <entt/entt.hpp>

#include <algorithm>
#include <cmath>

// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {
    namespace {
        namespace TU = Tsukino::BuiltIn::ECS::TransformUtility;

        //! @brief  PageUp/PageDown で動く量（枠の高さに対する割合）。少し重ねて、どこまで読んだか見失わないようにする
        constexpr float kPageRatio = 0.9f;

        //! @brief  表示位置が目標にこれより近づいたら、そこで揃える（ピクセル）
        constexpr float kSnapDistance = 0.5f;

        //-------------------------------------------------------------
        //! @brief  スクロールバーの今の形（画面ピクセル）
        //-------------------------------------------------------------
        struct BarGeometry {
            bool  valid      = false;    // スクロールバーがあり、表示する
            float centerX    = 0.0f;     // 溝とつまみの中心X
            float trackTop   = 0.0f;     // 溝の上端
            float trackLeft  = 0.0f;     // 溝の左端
            float trackRight = 0.0f;     // 溝の右端
            float trackSize  = 0.0f;     // 溝の長さ
            float thumbSize  = 0.0f;     // つまみの長さ
            float thumbTop   = 0.0f;     // つまみの上端
        };

        //-------------------------------------------------------------
        //! @brief  スクロールバーの形を求める
        //! @param  registry  [in] ECS レジストリ
        //! @param  scroll    [in] スクロールビュー
        //! @param  viewSize  [in] 枠の高さ
        //! @param  maxOffset [in] 表示位置の最大値
        //! @return スクロールバーの形。スクロールバーが無い・中身が枠に収まるときは valid が false
        //-------------------------------------------------------------
        BarGeometry ComputeBarGeometry(Tsukino::ECS::Registry& registry, const ScrollViewComponent& scroll, float viewSize, float maxOffset) {
            BarGeometry geometry;
            if(scroll.scrollBar == entt::null || !registry.HasComponent<TransformComponent>(scroll.scrollBar) ||
               !registry.HasComponent<ScrollBarComponent>(scroll.scrollBar))
                return geometry;
            if(!scroll.enabled || maxOffset <= 0.0f || scroll.contentHeight <= 0.0f)
                return geometry;

            const ScrollBarComponent& bar    = registry.GetComponent<ScrollBarComponent>(scroll.scrollBar);
            const hlslpp::float3      center = TU::GetWorldPosition(registry.GetComponent<TransformComponent>(scroll.scrollBar));

            geometry.valid      = true;
            geometry.centerX    = float(center.x);
            const float trackWidth  = bar.size.x;
            const float trackLength = bar.size.y;
            geometry.trackSize      = trackLength;
            geometry.trackTop       = float(center.y) - trackLength * 0.5f;
            geometry.trackLeft      = float(center.x) - trackWidth * 0.5f;
            geometry.trackRight     = float(center.x) + trackWidth * 0.5f;

            // つまみの長さは「枠に見えている割合」。短すぎると掴めないので下限を設ける
            geometry.thumbSize = std::clamp(trackLength * viewSize / scroll.contentHeight, std::min(bar.minThumbLength, trackLength), trackLength);
            geometry.thumbTop  = geometry.trackTop + (geometry.trackSize - geometry.thumbSize) * std::clamp(scroll.offset / maxOffset, 0.0f, 1.0f);
            return geometry;
        }

        //-------------------------------------------------------------
        //! @brief  画面スプライトを、指定の大きさ（ピクセル）になる拡大率にする
        //! @param  ctx       [in] エンジンコンテキスト（テクスチャの大きさを引く）
        //! @param  transform [in] 拡大率を書くトランスフォーム
        //! @param  sprite    [in] 大きさを測るスプライト
        //! @param  size      [in] 表示する大きさ（0 なら隠す）
        //-------------------------------------------------------------
        void SetSpriteSize(Tsukino::EngineIntegration::EngineContext* ctx, TransformComponent& transform, const SpriteComponent& sprite, const hlslpp::float2& size) {
            hlslpp::float3 scale(0.0f, 0.0f, 1.0f);
            if(size.x > 0.0f && size.y > 0.0f) {
                auto texture = std::static_pointer_cast<Tsukino::Asset::TextureAsset>(ctx->assetManager->Get(sprite.textureHandle));
                if(texture && texture->width > 0 && texture->height > 0)
                    scale = hlslpp::float3(size.x / static_cast<float>(texture->width), size.y / static_cast<float>(texture->height), 1.0f);
            }
            transform.scale = scale;
            transform.dirty = true;
        }

        //-------------------------------------------------------------
        //! @brief  スクロールバー（溝とつまみ）を表示位置に合わせて置き直す。表示しないときは隠す
        //! @param  registry [in] ECS レジストリ
        //! @param  ctx      [in] エンジンコンテキスト
        //! @param  scroll   [in] スクロールビュー
        //! @param  geometry [in] スクロールバーの形
        //-------------------------------------------------------------
        void PlaceScrollBar(Tsukino::ECS::Registry& registry, Tsukino::EngineIntegration::EngineContext* ctx, const ScrollViewComponent& scroll,
                            const BarGeometry& geometry) {
            if(scroll.scrollBar == entt::null || !registry.HasComponent<ScrollBarComponent>(scroll.scrollBar))
                return;

            const ScrollBarComponent& bar = registry.GetComponent<ScrollBarComponent>(scroll.scrollBar);

            if(auto* track = registry.try_get<TransformComponent>(scroll.scrollBar)) {
                if(const auto* sprite = registry.try_get<SpriteComponent>(scroll.scrollBar))
                    SetSpriteSize(ctx, *track, *sprite, geometry.valid ? bar.size : hlslpp::float2(0.0f, 0.0f));
            }

            if(bar.thumb == entt::null || !registry.HasComponent<TransformComponent>(bar.thumb))
                return;

            TransformComponent& thumb = registry.GetComponent<TransformComponent>(bar.thumb);
            if(const auto* sprite = registry.try_get<SpriteComponent>(bar.thumb))
                SetSpriteSize(ctx, thumb, *sprite, geometry.valid ? hlslpp::float2(bar.size.x, geometry.thumbSize) : hlslpp::float2(0.0f, 0.0f));

            if(geometry.valid) {
                const float z = TU::GetWorldPosition(thumb).z;
                TU::SetWorldPosition(registry, bar.thumb, hlslpp::float3(geometry.centerX, geometry.thumbTop + geometry.thumbSize * 0.5f, z));
            }
        }
    }    // namespace

    //-------------------------------------------------------------
    //! @brief システムの更新
    //-------------------------------------------------------------
    void ScrollViewSystem::Update(Tsukino::ECS::Registry& registry, float deltaTime) {
        Tsukino::EngineIntegration::EngineContext* ctx = registry.GetContext<Tsukino::EngineIntegration::EngineContext*>();
        if(!ctx || !ctx->inputSystem || !ctx->assetManager)
            return;

        const Tsukino::Input::InputSystem& input     = *ctx->inputSystem;
        i32                                rawMouseX = 0;
        i32                                rawMouseY = 0;
        input.GetMousePosition(&rawMouseX, &rawMouseY);
        const float          mouseX = static_cast<float>(rawMouseX);
        const float          mouseY = static_cast<float>(rawMouseY);
        const hlslpp::float2 mouse(mouseX, mouseY);

        const float wheel         = input.GetWheelDelta();
        const bool  buttonPressed = input.IsKeyPressed(Input::KeyCode::LButton);
        const bool  buttonDown    = input.IsKeyDown(Input::KeyCode::LButton);

        registry.View<TransformComponent, UIClipComponent, ScrollViewComponent>().each(
            [&](Tsukino::ECS::Entity, const TransformComponent& transform, const UIClipComponent& clip, ScrollViewComponent& scroll) {
                const UIClipUtility::ClipBounds bounds    = UIClipUtility::ComputeBounds(transform, clip);
                const float                     viewSize  = bounds.Height();
                const float                     maxOffset = std::max(0.0f, scroll.contentHeight - viewSize);

                //-------------------------------------------------------------
                // 入力。enabled でないビュー（閉じている画面など）は何も受けず、ドラッグも打ち切る
                //-------------------------------------------------------------
                if(!scroll.enabled) {
                    scroll.pointerDown   = false;
                    scroll.thumbDragging = false;
                } else {
                    BarGeometry bar = ComputeBarGeometry(registry, scroll, viewSize, maxOffset);

                    // ホイール（上へ回すと正なので、表示位置は上＝小さい方へ）
                    const bool overBar = bar.valid && mouseX >= bar.trackLeft && mouseX <= bar.trackRight && mouseY >= bar.trackTop &&
                                         mouseY <= bar.trackTop + bar.trackSize;
                    if(wheel != 0.0f && (bounds.Contains(mouse) || overBar))
                        scroll.targetOffset -= wheel * scroll.wheelStep;

                    // キー
                    if(input.IsKeyPressed(Input::KeyCode::Up))
                        scroll.targetOffset -= scroll.keyStep;
                    if(input.IsKeyPressed(Input::KeyCode::Down))
                        scroll.targetOffset += scroll.keyStep;
                    if(input.IsKeyPressed(Input::KeyCode::PageUp))
                        scroll.targetOffset -= viewSize * kPageRatio;
                    if(input.IsKeyPressed(Input::KeyCode::PageDown))
                        scroll.targetOffset += viewSize * kPageRatio;
                    if(input.IsKeyPressed(Input::KeyCode::Home))
                        scroll.targetOffset = 0.0f;
                    if(input.IsKeyPressed(Input::KeyCode::End))
                        scroll.targetOffset = maxOffset;

                    //-------------------------------------------------------------
                    // 押した瞬間：つまみ → 溝 → 枠の中の順に調べる
                    //-------------------------------------------------------------
                    if(buttonPressed) {
                        scroll.dragScrolling = false;
                        const bool onThumb   = overBar && mouseY >= bar.thumbTop && mouseY <= bar.thumbTop + bar.thumbSize;
                        if(onThumb) {
                            scroll.thumbDragging   = true;
                            scroll.thumbGrabOffset = mouseY - bar.thumbTop;
                        } else if(overBar) {
                            // 溝の空いている所：つまみのある側へ1画面ぶん
                            scroll.targetOffset += (mouseY < bar.thumbTop ? -1.0f : 1.0f) * viewSize * kPageRatio;
                        } else if(bounds.Contains(mouse)) {
                            scroll.pointerDown   = true;
                            scroll.pressPointerY = mouseY;
                            scroll.pressOffset   = scroll.offset;
                        }
                    }

                    if(!buttonDown) {
                        scroll.pointerDown   = false;
                        scroll.thumbDragging = false;
                    }

                    //-------------------------------------------------------------
                    // 中身のドラッグ。しきい値を超えた位置を基準にし直すので、動き出しで跳ねない
                    //-------------------------------------------------------------
                    if(scroll.pointerDown) {
                        if(!scroll.dragScrolling && maxOffset > 0.0f && std::abs(mouseY - scroll.pressPointerY) >= scroll.dragThreshold) {
                            scroll.dragScrolling = true;
                            scroll.pressPointerY = mouseY;
                            scroll.pressOffset   = scroll.offset;
                        }
                        if(scroll.dragScrolling)
                            scroll.targetOffset = scroll.offset = scroll.pressOffset - (mouseY - scroll.pressPointerY);
                    }

                    // つまみのドラッグ。つまみが動ける範囲の何割かを、表示位置の何割かにする
                    if(scroll.thumbDragging && bar.valid) {
                        const float movable = bar.trackSize - bar.thumbSize;
                        const float ratio   = (movable > 0.0f) ? (mouseY - scroll.thumbGrabOffset - bar.trackTop) / movable : 0.0f;
                        scroll.targetOffset = scroll.offset = std::clamp(ratio, 0.0f, 1.0f) * maxOffset;
                    }
                }

                //-------------------------------------------------------------
                // 表示位置を目標へなめらかに寄せる（フレームレートによらず同じ速さになるよう指数で寄せる）
                //-------------------------------------------------------------
                scroll.targetOffset = std::clamp(scroll.targetOffset, 0.0f, maxOffset);
                const float blend   = 1.0f - std::exp(-std::max(0.0f, scroll.followSpeed) * std::max(0.0f, deltaTime));
                scroll.offset += (scroll.targetOffset - scroll.offset) * blend;
                if(std::abs(scroll.targetOffset - scroll.offset) < kSnapDistance)
                    scroll.offset = scroll.targetOffset;
                scroll.offset = std::clamp(scroll.offset, 0.0f, maxOffset);

                //-------------------------------------------------------------
                // 中身を置き直す。中身の原点（左上）を、枠の左上から表示位置ぶん上へずらした所にする
                //-------------------------------------------------------------
                if(scroll.content != entt::null && registry.HasComponent<TransformComponent>(scroll.content)) {
                    TransformComponent&  content  = registry.GetComponent<TransformComponent>(scroll.content);
                    const hlslpp::float3 position = hlslpp::float3(-clip.size.x * 0.5f, -clip.size.y * 0.5f - scroll.offset, float(content.position.z));
                    if(float(content.position.x) != float(position.x) || float(content.position.y) != float(position.y)) {
                        content.position = position;
                        content.dirty    = true;
                    }
                }

                PlaceScrollBar(registry, ctx, scroll, ComputeBarGeometry(registry, scroll, viewSize, maxOffset));
            });
    }
}    // namespace Tsukino::BuiltIn::ECS
