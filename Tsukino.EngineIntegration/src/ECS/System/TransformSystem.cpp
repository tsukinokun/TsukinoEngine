//-------------------------------------------------------------
//! @file   TransformSystem.cpp
//! @brief  TransformSystemクラスの実装
//! @author 山﨑愛
//-------------------------------------------------------------
#include <Tsukino/EngineIntegration/ECS/System/TransformSystem.hpp>
#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>

#include <Tsukino/Core/Log.hpp>

#include <hlsl++.h>
#include <entt/entt.hpp>

// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {
    //-------------------------------------------------------------
    //! @brief システムの更新
    //-------------------------------------------------------------
    void TransformSystem::Update(Tsukino::ECS::Registry& registry, float deltaTime) {
        // 全てのTransformComponentを持つエンティティを取得
        auto view = registry.View<TransformComponent>();

        // ルートエンティティのリストをクリア（再利用）
        m_rootEntities.clear();

        //---------------------------------------------------------
        // 親 -> 子 の対応表もクリアする。
        // map自体は消さず、値のvectorだけをclearして確保済み容量を残す
        // （毎フレームの再ハッシュとヒープ確保を避けるため）
        //---------------------------------------------------------
        for(auto& entry : m_childrenByParent) {
            entry.second.clear();
        }

        //---------------------------------------------------------
        // ローカル行列の更新と、ルート／親子関係の収集を1パスで行う
        //---------------------------------------------------------
        for(auto entity : view) {
            auto& transform = view.template get<TransformComponent>(entity);

            // dirtyフラグが立っている場合のみローカル行列を更新
            if(transform.dirty) {
                UpdateLocalMatrix(transform);
            }

            //-----------------------------------------------------
            // 親がいない、親が無効、または循環参照になっている場合は
            // ルートエンティティとして記録する。
            //
            // 循環をルート扱いにするのは、放置すると「ルートでもなく再帰にも
            // 入らない」状態になり、worldMatrixが永久に更新されないまま
            // 無言で壊れるため（クラッシュしないので原因追跡が難しい）
            //-----------------------------------------------------
            const Tsukino::ECS::Entity parent = transform.parent;

            if(parent == entt::null || !registry.HasComponent<TransformComponent>(parent)) {
                m_rootEntities.push_back(entity);
            } else if(HasCycle(registry, entity)) {
                m_rootEntities.push_back(entity);
            } else {
                m_childrenByParent[static_cast<std::uint32_t>(parent)].push_back(entity);
            }
        }

        // ルートエンティティからワールド行列を階層的に更新
        const auto identity = Tsukino::Core::Math::matrix::identity();
        for(const auto rootEntity : m_rootEntities) {
            UpdateWorldMatrixRecursive(registry, rootEntity, identity, m_childrenByParent);
        }
    }

    //-------------------------------------------------------------
    //! @brief 親チェーンを遡って循環参照になっていないか調べる
    //-------------------------------------------------------------
    bool TransformSystem::HasCycle(Tsukino::ECS::Registry& registry, Tsukino::ECS::Entity entity) {
        Tsukino::ECS::Entity current = entity;

        //---------------------------------------------------------
        // 自分に戻ってきたら循環。深さ上限に達した場合も、
        // 現実的な階層ではありえない深さなので循環とみなして打ち切る
        // （再帰でのスタックオーバーフローを防ぐ役割も兼ねる）
        //---------------------------------------------------------
        for(int depth = 0; depth < kMaxHierarchyDepth; ++depth) {
            if(!registry.HasComponent<TransformComponent>(current)) {
                return false;
            }

            const Tsukino::ECS::Entity parent = registry.GetComponent<TransformComponent>(current).parent;
            if(parent == entt::null) {
                return false;
            }
            if(parent == entity) {
                Tsukino::Core::Log::Warn("TransformSystem: parent cycle detected. Treating the entity as a root.");
                return true;
            }

            current = parent;
        }

        Tsukino::Core::Log::Warn("TransformSystem: hierarchy is too deep (or cyclic). Treating the entity as a root.");
        return true;
    }

    //-------------------------------------------------------------
    //! @brief ローカル行列の更新
    //-------------------------------------------------------------
    void TransformSystem::UpdateLocalMatrix(TransformComponent& transform) noexcept {
        // TRS行列の構築 (Scale * Rotation * Translation)
        // 行ベクトル規約: mul(a, b) は a を先に適用してから b を適用する
        const auto scaleMatrix       = Tsukino::Core::Math::matrix::scale(transform.scale);
        const auto rotationMatrix    = Tsukino::Core::Math::matrix(hlslpp::float4x4(transform.rotation));
        const auto translationMatrix = Tsukino::Core::Math::matrix::translate(transform.position);

        // ローカル行列 = スケール → 回転 → 平行移動 の順で適用
        transform.localMatrix = hlslpp::mul(scaleMatrix, hlslpp::mul(rotationMatrix, translationMatrix));

        transform.dirty = false;
    }

    //-------------------------------------------------------------
    //! @brief ワールド行列の更新（再帰処理）
    //-------------------------------------------------------------
    void TransformSystem::UpdateWorldMatrixRecursive(Tsukino::ECS::Registry&            registry,
                                                     Tsukino::ECS::Entity               entity,
                                                     const Tsukino::Core::Math::matrix& parentWorld,
                                                     const ChildrenMap&                 children) noexcept {
        // エンティティがTransformComponentを持っているか確認
        if(!registry.HasComponent<TransformComponent>(entity)) {
            return;
        }

        auto& transform = registry.GetComponent<TransformComponent>(entity);

        //---------------------------------------------------------
        // ワールド行列 = 自分のローカル行列 -> 親のワールド行列 の順に適用する。
        // 行ベクトル規約では mul(a, b) は a を先に適用してから b を適用するため、
        // 子のローカル行列が左、親のワールド行列が右になる
        // （UpdateLocalMatrixのScale * Rotation * Translationと同じ考え方。
        //   ModelSystemのmul(nodeTransform, worldMatrix)とも一致する）
        //---------------------------------------------------------
        transform.worldMatrix = hlslpp::mul(transform.localMatrix, parentWorld);

        //---------------------------------------------------------
        // 子エンティティを再帰的に更新する。
        // 以前はここでView全体を走査して「親が自分のもの」を線形探索していたため、
        // エンティティ数Nに対して全体でO(N^2)になっていた。
        // Updateが構築した対応表を引くだけにしてO(N)へ落としている
        //---------------------------------------------------------
        const auto it = children.find(static_cast<std::uint32_t>(entity));
        if(it == children.end()) {
            return;
        }

        for(const auto childEntity : it->second) {
            UpdateWorldMatrixRecursive(registry, childEntity, transform.worldMatrix, children);
        }
    }

}    // namespace Tsukino::BuiltIn::ECS
