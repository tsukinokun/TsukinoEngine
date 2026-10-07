//--------------------------------------------------------------
//! @file   UIClipUtility.hpp
//! @brief  画面UIの切り取り枠（UIClipComponent）を引くヘルパー
//--------------------------------------------------------------
#pragma once
#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/UIClipComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Transform/TransformUtility.hpp>

#include <Tsukino/Core/ECS/Registry/Registry.hpp>

#include <hlsl++.h>
#include <entt/entt.hpp>

// 名前空間 : Tsukino::BuiltIn::ECS::UIClipUtility
namespace Tsukino::BuiltIn::ECS::UIClipUtility {

    //--------------------------------------------------------------
    //! @struct ClipBounds
    //! @brief  切り取り枠の範囲（UI の座標。左上原点。シザーに使うときは UICanvas::ToPixel で画面のピクセルにする）
    //--------------------------------------------------------------
    struct ClipBounds {
        float left   = 0.0f;    // 左端
        float top    = 0.0f;    // 上端
        float right  = 0.0f;    // 右端
        float bottom = 0.0f;    // 下端

        //! @brief  点が枠の中にあるか
        [[nodiscard]] bool Contains(const hlslpp::float2& point) const noexcept {
            const float x = point.x;
            const float y = point.y;
            return x >= left && x <= right && y >= top && y <= bottom;
        }

        //! @brief  中心と大きさで表した矩形が、枠と少しでも重なるか
        [[nodiscard]] bool Overlaps(const hlslpp::float2& center, const hlslpp::float2& size) const noexcept {
            const float x     = center.x;
            const float y     = center.y;
            const float halfW = float(size.x) * 0.5f;
            const float halfH = float(size.y) * 0.5f;
            return x + halfW > left && x - halfW < right && y + halfH > top && y - halfH < bottom;
        }

        //! @brief  枠の高さ
        [[nodiscard]] float Height() const noexcept { return bottom - top; }
    };

    //--------------------------------------------------------------
    //! @brief  UIClipComponent を持つエンティティの枠の範囲を求める
    //! @param  transform [in] 枠のエンティティのトランスフォーム
    //! @param  clip      [in] 枠のエンティティの UIClipComponent
    //! @return 枠の範囲
    //--------------------------------------------------------------
    [[nodiscard]] inline ClipBounds ComputeBounds(const TransformComponent& transform, const UIClipComponent& clip) noexcept {
        const hlslpp::float3 center = TransformUtility::GetWorldPosition(transform);
        const hlslpp::float2 half   = clip.size * TransformUtility::GetWorldScale2D(transform) * 0.5f;

        ClipBounds bounds;
        bounds.left   = center.x - half.x;
        bounds.top    = center.y - half.y;
        bounds.right  = center.x + half.x;
        bounds.bottom = center.y + half.y;
        return bounds;
    }

    //--------------------------------------------------------------
    //! @brief  エンティティを切り取る枠（自身または一番近い祖先の UIClipComponent）を探す
    //! @param  registry  [in]  ECS レジストリ
    //! @param  entity    [in]  調べるエンティティ
    //! @param  outBounds [out] 見つかった枠の範囲
    //! @return 枠があれば true
    //--------------------------------------------------------------
    [[nodiscard]] inline bool TryGetClipBounds(Tsukino::ECS::Registry& registry, Tsukino::ECS::Entity entity, ClipBounds& outBounds) {
        const Tsukino::ECS::Entity clipEntity = TransformUtility::FindNearestWith<UIClipComponent>(registry, entity);
        if(clipEntity == entt::null)
            return false;

        outBounds = ComputeBounds(registry.GetComponent<TransformComponent>(clipEntity), registry.GetComponent<UIClipComponent>(clipEntity));
        return true;
    }
}    // namespace Tsukino::BuiltIn::ECS::UIClipUtility
