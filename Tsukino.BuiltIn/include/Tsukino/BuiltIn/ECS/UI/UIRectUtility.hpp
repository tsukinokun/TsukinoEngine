//--------------------------------------------------------------
//! @file   UIRectUtility.hpp
//! @brief  画面UIの部品の四角（UIRectComponent）を引いて、画面上の範囲や当たりを求めるヘルパー
//--------------------------------------------------------------
#pragma once
#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/UIRectComponent.hpp>

#include <Tsukino/Core/ECS/Registry/Registry.hpp>
#include <Tsukino/Core/Math/Matrix.hpp>

#include <hlsl++.h>

#include <algorithm>
#include <cmath>

// 名前空間 : Tsukino::BuiltIn::ECS::UIRectUtility
namespace Tsukino::BuiltIn::ECS::UIRectUtility {

    //--------------------------------------------------------------
    //! @struct LocalRect
    //! @brief  ローカルの座標（ワールド行列で写す前）での四角。中心と大きさで持つ
    //--------------------------------------------------------------
    struct LocalRect {
        hlslpp::float2 center = hlslpp::float2(0.0f, 0.0f);    // 四角の中心（Transform の位置からのずれ）
        hlslpp::float2 size   = hlslpp::float2(0.0f, 0.0f);    // 幅・高さ
    };

    //--------------------------------------------------------------
    //! @brief  UIRectComponent の四角をローカルの四角にする
    //--------------------------------------------------------------
    [[nodiscard]] inline LocalRect FromRect(const UIRectComponent& rect) noexcept {
        LocalRect local;
        local.size   = rect.size;
        local.center = (hlslpp::float2(0.5f, 0.5f) - rect.pivot) * rect.size;
        return local;
    }

    //--------------------------------------------------------------
    //! @brief  エンティティのローカルの四角を求める
    //! @param  registry     [in] ECS レジストリ
    //! @param  entity       [in] 調べるエンティティ
    //! @param  fallbackSize [in] UIRectComponent が無いときの大きさ（中心は Transform の位置）
    //! @return ローカルの四角
    //--------------------------------------------------------------
    [[nodiscard]] inline LocalRect Resolve(Tsukino::ECS::Registry& registry, Tsukino::ECS::Entity entity, const hlslpp::float2& fallbackSize) {
        if(const auto* rect = registry.try_get<UIRectComponent>(entity))
            return FromRect(*rect);

        LocalRect local;
        local.size = fallbackSize;
        return local;
    }

    //--------------------------------------------------------------
    //! @brief  中心が原点の単位の四角を、画面上の四角へ写す行列
    //! @param  local       [in] ローカルの四角
    //! @param  worldMatrix [in] エンティティのワールド行列
    //--------------------------------------------------------------
    [[nodiscard]] inline hlslpp::float4x4 ToMatrix(const LocalRect& local, const hlslpp::float4x4& worldMatrix) {
        const auto sizeMatrix   = Tsukino::Core::Math::matrix::scale(local.size.x, local.size.y, 1.0f);
        const auto offsetMatrix = Tsukino::Core::Math::matrix::translate(hlslpp::float3(local.center.x, local.center.y, 0.0f));
        return hlslpp::mul(hlslpp::mul(sizeMatrix, offsetMatrix), worldMatrix);
    }

    //--------------------------------------------------------------
    //! @brief  ローカルの点を画面上の点にする（行ベクトル規約。Z は 0 として扱う）
    //--------------------------------------------------------------
    [[nodiscard]] inline hlslpp::float2 LocalToWorld(const hlslpp::float4x4& worldMatrix, const hlslpp::float2& point) {
        const hlslpp::float2 axisX(worldMatrix[0].x, worldMatrix[0].y);
        const hlslpp::float2 axisY(worldMatrix[1].x, worldMatrix[1].y);
        const hlslpp::float2 origin(worldMatrix[3].x, worldMatrix[3].y);
        return origin + axisX * point.x + axisY * point.y;
    }

    //--------------------------------------------------------------
    //! @brief  画面上の点が四角の中にあるか（拡大・回転を考慮する）
    //! @param  worldMatrix [in] エンティティのワールド行列
    //! @param  local       [in] ローカルの四角
    //! @param  point       [in] 調べる点（UI の座標）
    //--------------------------------------------------------------
    [[nodiscard]] inline bool Contains(const hlslpp::float4x4& worldMatrix, const LocalRect& local, const hlslpp::float2& point) {
        // 画面の点をローカルへ戻す（X 軸・Y 軸の 2×2 の逆行列）
        const float ax = worldMatrix[0].x, ay = worldMatrix[0].y;
        const float bx = worldMatrix[1].x, by = worldMatrix[1].y;
        const float determinant = ax * by - bx * ay;
        if(std::abs(determinant) < 1.0e-8f)
            return false;    // 潰れている四角には当たらない

        const float dx     = float(point.x) - float(worldMatrix[3].x);
        const float dy     = float(point.y) - float(worldMatrix[3].y);
        const float localX = (dx * by - dy * bx) / determinant;
        const float localY = (dy * ax - dx * ay) / determinant;

        const float halfW = std::abs(float(local.size.x)) * 0.5f;
        const float halfH = std::abs(float(local.size.y)) * 0.5f;
        return std::abs(localX - float(local.center.x)) <= halfW && std::abs(localY - float(local.center.y)) <= halfH;
    }

    //--------------------------------------------------------------
    //! @brief  四角を画面に写したものを囲む、軸に沿った範囲を求める
    //! @param  worldMatrix [in]  エンティティのワールド行列
    //! @param  local       [in]  ローカルの四角
    //! @param  outMin      [out] 左上
    //! @param  outMax      [out] 右下
    //--------------------------------------------------------------
    inline void WorldBounds(const hlslpp::float4x4& worldMatrix, const LocalRect& local, hlslpp::float2& outMin, hlslpp::float2& outMax) {
        const hlslpp::float2 half = local.size * 0.5f;
        const hlslpp::float2 corners[4] = {
            LocalToWorld(worldMatrix, local.center + hlslpp::float2(-half.x, -half.y)),
            LocalToWorld(worldMatrix, local.center + hlslpp::float2(half.x, -half.y)),
            LocalToWorld(worldMatrix, local.center + hlslpp::float2(-half.x, half.y)),
            LocalToWorld(worldMatrix, local.center + hlslpp::float2(half.x, half.y)),
        };
        outMin = corners[0];
        outMax = corners[0];
        for(const hlslpp::float2& corner : corners) {
            outMin = hlslpp::min(outMin, corner);
            outMax = hlslpp::max(outMax, corner);
        }
    }
}    // namespace Tsukino::BuiltIn::ECS::UIRectUtility
