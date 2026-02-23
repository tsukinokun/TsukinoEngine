//-------------------------------------------------------------
//! @file   TransformSystem.cpp
//! @brief  TransformSystemクラスの実装
//! @author 山﨑愛
//-------------------------------------------------------------
#include "Tsukino/Core/ECS/System/TransformSystem.hpp"
#include <hlsl++.h>
#include <entt/entt.hpp>

// 名前空間 : Tsukino::ECS
namespace Tsukino::ECS {
    //-------------------------------------------------------------
    //! @brief システムの更新
    //-------------------------------------------------------------
    void TransformSystem::Update(Tsukino::ECS::Registry& registry, float deltaTime) {
        // 全てのTransformComponentを持つエンティティを取得
        auto view = registry.View<TransformComponent>();

        // ルートエンティティのリストをクリア（再利用）
        m_rootEntities.clear();

        // ローカル行列の更新とルートエンティティの収集
        for (auto entity : view) {
            auto& transform = view.template get<TransformComponent>(entity);
            
            // dirtyフラグが立っている場合のみローカル行列を更新
            if (transform.dirty) {
                UpdateLocalMatrix(transform);
            }

            // 親がいない、または親が無効な場合はルートエンティティとして記録
            if (transform.parent == entt::null || !registry.HasComponent<TransformComponent>(transform.parent)) {
                m_rootEntities.push_back(entity);
            }
        }

        // ルートエンティティからワールド行列を階層的に更新
        const auto identity = Tsukino::Core::Math::matrix::identity();
        for (const auto rootEntity : m_rootEntities) {
            UpdateWorldMatrixRecursive(registry, rootEntity, identity);
        }
    }

    //-------------------------------------------------------------
    //! @brief ローカル行列の更新
    //-------------------------------------------------------------
    void TransformSystem::UpdateLocalMatrix(TransformComponent& transform) noexcept {
        // TRS行列の構築 (Translation * Rotation * Scale)
        // 注意: hlsl++の行列乗算は右から左に適用される
        const auto scaleMatrix = Tsukino::Core::Math::matrix::scale(transform.scale);
        const auto rotationMatrix = Tsukino::Core::Math::matrix(hlslpp::float4x4(transform.rotation));
        const auto translationMatrix = Tsukino::Core::Math::matrix::translate(transform.position);

        // ローカル行列 = 平行移動 * 回転 * スケール
        transform.localMatrix = hlslpp::mul(translationMatrix, hlslpp::mul(rotationMatrix, scaleMatrix));
        
        // dirtyフラグをリセット
        transform.dirty = false;
    }

    //-------------------------------------------------------------
    //! @brief ワールド行列の更新（再帰処理）
    //-------------------------------------------------------------
    void TransformSystem::UpdateWorldMatrixRecursive(
        Tsukino::ECS::Registry& registry,
        Entity entity,
        const Tsukino::Core::Math::matrix& parentWorld
    ) noexcept {
        // エンティティがTransformComponentを持っているか確認
        if (!registry.HasComponent<TransformComponent>(entity)) {
            return;
        }

        auto& transform = registry.GetComponent<TransformComponent>(entity);

        // ワールド行列 = 親のワールド行列 * 自分のローカル行列
        transform.worldMatrix = hlslpp::mul(parentWorld, transform.localMatrix);

        // 子エンティティを探索して再帰的に更新
        // キャッシュフレンドリーな実装: viewを1回だけ取得
        auto view = registry.View<TransformComponent>();
        for (auto childEntity : view) {
            const auto& childTransform = view.template get<TransformComponent>(childEntity);
            
            // このエンティティを親として持つ子エンティティを見つける
            if (childTransform.parent == entity) {
                // 子のワールド行列を再帰的に更新
                UpdateWorldMatrixRecursive(registry, childEntity, transform.worldMatrix);
            }
        }
    }

}    // namespace Tsukino::ECS
