//--------------------------------------------------------------
//! @file   TransformUtility.hpp
//! @brief  TransformComponentの親子関係・ワールド座標を扱うヘルパー
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>

#include <Tsukino/Core/ECS/Registry/Registry.hpp>
#include <Tsukino/Core/Log.hpp>

#include <hlsl++.h>
#include <entt/entt.hpp>

// 名前空間 : Tsukino::BuiltIn::ECS::TransformUtility
namespace Tsukino::BuiltIn::ECS::TransformUtility {

    //--------------------------------------------------------------
    // 親子チェーンを辿る際の深さ上限。
    // 循環参照が混入した場合の無限ループを防ぐための安全弁で、
    // 現実的な階層の深さより十分大きい値にしてある
    //--------------------------------------------------------------
    inline constexpr int kMaxHierarchyDepth = 64;

    //--------------------------------------------------------------
    //! @brief  ワールド座標を取り出す
    //! @note   行ベクトル規約なので平行移動成分は4行目（m[3]）にある
    //--------------------------------------------------------------
    [[nodiscard]] inline hlslpp::float3 GetWorldPosition(const TransformComponent& transform) noexcept {
        return hlslpp::float3(transform.worldMatrix[3].xyz);
    }

    //--------------------------------------------------------------
    //! @brief  ワールド行列に掛かっているXYスケールを取り出す
    //! @note   基底ベクトルの長さ。当たり判定でスプライトの実サイズを
    //!         求めるのに使う（FontRendererSystemと同じ取り出し方）
    //--------------------------------------------------------------
    [[nodiscard]] inline hlslpp::float2 GetWorldScale2D(const TransformComponent& transform) noexcept {
        const float scaleX = hlslpp::length(hlslpp::float3(transform.worldMatrix[0].xyz));
        const float scaleY = hlslpp::length(hlslpp::float3(transform.worldMatrix[1].xyz));
        return hlslpp::float2(scaleX, scaleY);
    }

    //--------------------------------------------------------------
    //! @brief  有効な親のワールド行列を取得する
    //! @return 親がいない／親が無効なら単位行列
    //--------------------------------------------------------------
    [[nodiscard]] inline Tsukino::Core::Math::matrix GetParentWorldMatrix(Tsukino::ECS::Registry& registry, const TransformComponent& transform) {
        const Tsukino::ECS::Entity parent = transform.parent;
        if(parent == entt::null || !registry.HasComponent<TransformComponent>(parent)) {
            return Tsukino::Core::Math::matrix::identity();
        }
        return registry.GetComponent<TransformComponent>(parent).worldMatrix;
    }

    //--------------------------------------------------------------
    //! @brief  candidateAncestor が entity の祖先（自分自身を含む）かどうか
    //! @note   SetParentの循環チェックに使う
    //--------------------------------------------------------------
    [[nodiscard]] inline bool IsAncestorOf(Tsukino::ECS::Registry& registry, Tsukino::ECS::Entity candidateAncestor, Tsukino::ECS::Entity entity) {
        Tsukino::ECS::Entity current = entity;
        for(int depth = 0; depth < kMaxHierarchyDepth; ++depth) {
            if(current == entt::null) {
                return false;
            }
            if(current == candidateAncestor) {
                return true;
            }
            if(!registry.HasComponent<TransformComponent>(current)) {
                return false;
            }
            current = registry.GetComponent<TransformComponent>(current).parent;
        }
        // 深さ上限に達した＝既に循環している。これ以上辿らず「祖先である」とみなして
        // 新たな親付けを拒否させる
        return true;
    }

    //--------------------------------------------------------------
    //! @brief  ワールド座標を指定する（親がいればローカル座標へ変換して格納）
    //! @note   dirtyを必ず立てるので、呼び出し側で立て忘れる事故が起きない
    //--------------------------------------------------------------
    inline void SetWorldPosition(Tsukino::ECS::Registry& registry, Tsukino::ECS::Entity entity, const hlslpp::float3& worldPosition) {
        if(!registry.HasComponent<TransformComponent>(entity)) {
            return;
        }
        auto& transform = registry.GetComponent<TransformComponent>(entity);

        const Tsukino::ECS::Entity parent = transform.parent;
        if(parent == entt::null || !registry.HasComponent<TransformComponent>(parent)) {
            // 親なし：ワールド座標＝ローカル座標
            transform.position = worldPosition;
        } else {
            //--------------------------------------------------------------
            // 親あり：親のワールド行列の逆行列を通してローカル座標へ落とす。
            // 行ベクトル規約なので「点（w=1）× 行列」の順で掛ける
            //--------------------------------------------------------------
            const auto&          parentWorld    = registry.GetComponent<TransformComponent>(parent).worldMatrix;
            const hlslpp::float4x4 invParentWorld = hlslpp::inverse(parentWorld);
            const hlslpp::float4 localPosition   = hlslpp::mul(hlslpp::float4(worldPosition, 1.0f), invParentWorld);
            transform.position                   = hlslpp::float3(localPosition.xyz);
        }

        transform.dirty = true;
    }

    //--------------------------------------------------------------
    //! @brief  親を設定する
    //! @param  child             [in] 子にするエンティティ
    //! @param  parent            [in] 親にするエンティティ（entt::nullで親子解除）
    //! @param  keepWorldPosition [in] trueなら見た目のワールド位置を維持するよう
    //!                                ローカル座標を再計算する
    //! @return 設定できたらtrue（循環になる場合はfalseで何もしない）
    //! @note   worldMatrixはTransformSystemが更新するため、生成直後など
    //!         まだ1度もTransformSystemを通していない状態でkeepWorldPosition=trueを
    //!         使うと、ゼロ行列を元に計算してしまう。その場合はfalseを指定するか、
    //!         TransformSystemを1度走らせてから呼ぶこと
    //--------------------------------------------------------------
    inline bool SetParent(Tsukino::ECS::Registry& registry, Tsukino::ECS::Entity child, Tsukino::ECS::Entity parent, bool keepWorldPosition = true) {
        if(!registry.HasComponent<TransformComponent>(child)) {
            Tsukino::Core::Log::Warn("SetParent: child has no TransformComponent");
            return false;
        }

        //--------------------------------------------------------------
        // 自分自身を親にする／子孫を親にすると循環して、TransformSystemの
        // ワールド行列更新から永久に外れる（無言で壊れる）ため事前に弾く
        //--------------------------------------------------------------
        if(parent != entt::null) {
            if(!registry.HasComponent<TransformComponent>(parent)) {
                Tsukino::Core::Log::Warn("SetParent: parent has no TransformComponent");
                return false;
            }
            if(IsAncestorOf(registry, child, parent)) {
                Tsukino::Core::Log::Warn("SetParent: rejected because it would create a cycle");
                return false;
            }
        }

        auto&                childTransform = registry.GetComponent<TransformComponent>(child);
        const hlslpp::float3 worldPosition  = GetWorldPosition(childTransform);

        childTransform.parent = parent;
        childTransform.dirty  = true;

        if(keepWorldPosition) {
            SetWorldPosition(registry, child, worldPosition);
        }

        return true;
    }

}    // namespace Tsukino::BuiltIn::ECS::TransformUtility
