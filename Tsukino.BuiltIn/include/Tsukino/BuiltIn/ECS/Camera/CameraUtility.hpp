//----------------------------------------------------------------------------
//! @file   CameraUtility.hpp
//! @brief  カメラを使ったスクリーン座標とワールド座標の相互変換
//! @detail マウスで 3D のオブジェクトを選ぶ（ピッキング）ためのレイの生成と、
//!         ワールド座標を画面のピクセル座標へ写す処理をまとめています。
//!         CameraSystem が更新した viewProjMatrix / invViewProjMatrix を使います。
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/BuiltIn/ECS/Component/CameraComponent.hpp>

#include <hlsl++.h>

#include <cmath>

// 名前空間 : Tsukino::BuiltIn::ECS::CameraUtility
namespace Tsukino::BuiltIn::ECS::CameraUtility {

    //! 始点と向きで表した半直線です。
    struct Ray {
        hlslpp::float3 origin    = hlslpp::float3(0.0f, 0.0f, 0.0f);    // 始点（ワールド座標）
        hlslpp::float3 direction = hlslpp::float3(0.0f, 0.0f, 1.0f);    // 向き（正規化済み）

        //! 始点から距離 t 進んだ位置を返します。
        //! @param  [in] t 始点からの距離。
        //! @return その位置のワールド座標。
        [[nodiscard]] hlslpp::float3 GetPoint(float t) const { return origin + direction * t; }
    };

    //----------------------------------------------------------------------------
    //! 画面上の点を通るレイを作ります。
    //! 逆 Z（near が深度 1、far が深度 0）の射影を前提に、near 面と far 面の点を逆射影して結びます。
    //! 透視投影でも正射影でも使えます。
    //! @param  [in] camera      CameraSystem が更新済みのカメラ。
    //! @param  [in] screenPoint 画面上の点（ビューポート左上原点のピクセル座標）。
    //! @param  [in] screenSize  ビューポートの大きさ（ピクセル）。
    //! @return ワールド空間のレイ。
    //----------------------------------------------------------------------------
    [[nodiscard]] inline Ray ScreenPointToRay(const CameraComponent& camera, const hlslpp::float2& screenPoint, const hlslpp::float2& screenSize) {
        // ピクセル → NDC（Y は上向きが正）
        const float ndcX = (float(screenPoint.x) / float(screenSize.x)) * 2.0f - 1.0f;
        const float ndcY = 1.0f - (float(screenPoint.y) / float(screenSize.y)) * 2.0f;

        // 行ベクトル規約なので mul(v, m) の順。w で割って同次座標から戻す
        hlslpp::float4 nearPoint = hlslpp::mul(hlslpp::float4(ndcX, ndcY, 1.0f, 1.0f), camera.invViewProjMatrix);
        hlslpp::float4 farPoint  = hlslpp::mul(hlslpp::float4(ndcX, ndcY, 0.0f, 1.0f), camera.invViewProjMatrix);
        const hlslpp::float3 nearWorld = nearPoint.xyz / nearPoint.w;
        const hlslpp::float3 farWorld  = farPoint.xyz / farPoint.w;

        Ray ray;
        ray.origin    = nearWorld;
        ray.direction = hlslpp::normalize(farWorld - nearWorld);
        return ray;
    }

    //----------------------------------------------------------------------------
    //! ワールド座標を画面上の点へ写します。
    //! @param  [in]  camera      CameraSystem が更新済みのカメラ。
    //! @param  [in]  worldPoint  ワールド座標。
    //! @param  [in]  screenSize  ビューポートの大きさ（ピクセル）。
    //! @param  [out] screenPoint 画面上の点（ビューポート左上原点のピクセル座標）。
    //! @return カメラの前方にあれば true。後方なら false で screenPoint は変更しません。
    //----------------------------------------------------------------------------
    inline bool WorldToScreenPoint(const CameraComponent& camera, const hlslpp::float3& worldPoint, const hlslpp::float2& screenSize, hlslpp::float2& screenPoint) {
        const hlslpp::float4 clip = hlslpp::mul(hlslpp::float4(worldPoint, 1.0f), camera.viewProjMatrix);
        if(float(clip.w) <= 0.0f) {
            return false;
        }
        const hlslpp::float2 ndc = clip.xy / clip.w;
        screenPoint              = hlslpp::float2((float(ndc.x) * 0.5f + 0.5f) * float(screenSize.x), (0.5f - float(ndc.y) * 0.5f) * float(screenSize.y));
        return true;
    }

    //----------------------------------------------------------------------------
    //! レイと平面の交点までの距離を求めます。
    //! @param  [in]  ray         レイ。
    //! @param  [in]  planePoint  平面上の任意の点。
    //! @param  [in]  planeNormal 平面の法線（正規化済み）。
    //! @param  [out] distance    レイの始点から交点までの距離。
    //! @return 前方で交われば true。平行または後方なら false。
    //----------------------------------------------------------------------------
    inline bool IntersectPlane(const Ray& ray, const hlslpp::float3& planePoint, const hlslpp::float3& planeNormal, float& distance) {
        const float denom = hlslpp::dot(ray.direction, planeNormal);
        // 平行に近いと交点が遠方へ飛んで不安定になるため除外
        if(std::fabs(denom) < 1e-6f) {
            return false;
        }
        const float t = float(hlslpp::dot(planePoint - ray.origin, planeNormal)) / denom;
        if(t < 0.0f) {
            return false;
        }
        distance = t;
        return true;
    }

    //----------------------------------------------------------------------------
    //! レイとワールド行列で置いた矩形の交点を求めます。
    //! 矩形はローカル空間の原点を中心に、軸 axisU 方向へ幅 size.x、軸 axisV 方向へ高さ size.y を持ちます。
    //! 裏側からのレイも当たりとして扱います。
    //! @param  [in]  ray         レイ。
    //! @param  [in]  worldMatrix 矩形のワールド行列（行ベクトル規約）。
    //! @param  [in]  axisU       ローカル空間で幅の方向になる軸（例：X 軸）。
    //! @param  [in]  axisV       ローカル空間で高さの方向になる軸（例：Z 軸）。
    //! @param  [in]  size        ローカル空間での幅と高さ。
    //! @param  [out] distance    レイの始点から交点までの距離。
    //! @param  [out] uv          交点の矩形上の位置（0〜1、左下が 0,0）。
    //! @return 矩形の範囲内で交われば true。
    //----------------------------------------------------------------------------
    inline bool IntersectRect(const Ray& ray, const Tsukino::Core::Math::matrix& worldMatrix, const hlslpp::float3& axisU, const hlslpp::float3& axisV,
                              const hlslpp::float2& size, float& distance, hlslpp::float2& uv) {
        // ローカルの軸をワールドへ。スケールも含むため、長さを保ったまま使う
        const hlslpp::float3 center = worldMatrix[3].xyz;
        const hlslpp::float3 worldU = hlslpp::mul(hlslpp::float4(axisU, 0.0f), worldMatrix).xyz;
        const hlslpp::float3 worldV = hlslpp::mul(hlslpp::float4(axisV, 0.0f), worldMatrix).xyz;
        const hlslpp::float3 normal = hlslpp::normalize(hlslpp::cross(worldU, worldV));

        float t = 0.0f;
        if(!IntersectPlane(ray, center, normal, t) && !IntersectPlane(ray, center, -normal, t)) {
            return false;
        }

        // 交点を矩形の軸へ射影して、範囲内かを調べる
        const hlslpp::float3 local  = ray.GetPoint(t) - center;
        const float          lenU2  = hlslpp::dot(worldU, worldU);
        const float          lenV2  = hlslpp::dot(worldV, worldV);
        const float          u      = float(hlslpp::dot(local, worldU)) / lenU2 / float(size.x) + 0.5f;
        const float          v      = float(hlslpp::dot(local, worldV)) / lenV2 / float(size.y) + 0.5f;
        if(u < 0.0f || u > 1.0f || v < 0.0f || v > 1.0f) {
            return false;
        }
        distance = t;
        uv       = hlslpp::float2(u, v);
        return true;
    }

}    // namespace Tsukino::BuiltIn::ECS::CameraUtility
