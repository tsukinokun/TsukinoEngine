//-------------------------------------------------------------
//! @file   TransformSystem.hpp
//! @brief  TransformSystemクラスの宣言
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once
#include <Tsukino/Core/ECS/System/ISystem.hpp>
#include <Tsukino/Core/Math/Matrix.hpp>
#include <cstdint>
#include <unordered_map>
#include <vector>
#include <span>
// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {
    struct TransformComponent;    // 前方宣言

    //-------------------------------------------------------------
    //! @class  TransformSystem
    //! @brief  トランスフォームの階層構造を管理し、親子関係を維持するシステム
    //-------------------------------------------------------------
    class TransformSystem final : public Tsukino::ECS::ISystem {
    public:
        //-------------------------------------------------------------
        // システムの更新
        //! @param  registry    [in] ECS レジストリ
        //! @param  deltaTime   [in] 前フレームからの経過時間
        //-------------------------------------------------------------
        void Update(Tsukino::ECS::Registry& registry, float deltaTime) override;

    private:
        //! @brief 親エンティティ -> その子エンティティ一覧
        using ChildrenMap = std::unordered_map<std::uint32_t, std::vector<Tsukino::ECS::Entity>>;

        //-------------------------------------------------------------
        //! @brief 親チェーンを遡れる深さの上限（循環検出・再帰深度の安全弁）
        //-------------------------------------------------------------
        static constexpr int kMaxHierarchyDepth = 64;

        //-------------------------------------------------------------
        // 親チェーンが循環しているか調べる
        //! @param  registry [in] ECS レジストリ
        //! @param  entity   [in] 調べたいエンティティ
        //! @return 循環している（または深すぎる）場合 true
        //-------------------------------------------------------------
        static bool HasCycle(Tsukino::ECS::Registry& registry, Tsukino::ECS::Entity entity);

        //-------------------------------------------------------------
        // ローカル行列の更新
        //! @param  transform [in/out] トランスフォームコンポーネント
        //-------------------------------------------------------------
        static void UpdateLocalMatrix(TransformComponent& transform) noexcept;

        //-------------------------------------------------------------
        // ワールド行列の更新（再帰処理）
        //! @param  registry    [in]     ECS レジストリ
        //! @param  entity      [in]     更新対象のエンティティ
        //! @param  parentWorld [in]     親のワールド行列
        //! @param  children    [in]     親 -> 子の対応表（Updateが毎フレーム構築する）
        //-------------------------------------------------------------
        static void UpdateWorldMatrixRecursive(Tsukino::ECS::Registry&            registry,
                                               Tsukino::ECS::Entity               entity,
                                               const Tsukino::Core::Math::matrix& parentWorld,
                                               const ChildrenMap&                 children) noexcept;

        // 一時バッファ（メモリ確保の最適化用）
        std::vector<Tsukino::ECS::Entity> m_rootEntities;

        //-------------------------------------------------------------
        //! @brief 親 -> 子 の対応表（一時バッファ）
        //! @note  以前は子を探すたびにTransformComponentのView全体を走査していたため、
        //!        エンティティ数Nに対してO(N^2)になっていた。TransformSystemは
        //!        1フレームに3回動くので、敵を大量に湧かせるとここが支配的なコストになる。
        //!        Updateの先頭で1パス舐めてこの表を作り、再帰では表を引くだけにしている。
        //!
        //!        毎フレームmap自体はクリアせず、値のvectorだけをclearして容量を残す
        //!        （再ハッシュとヒープ確保を避けるため）。使われなくなったキーは
        //!        空のvectorとして残るが、探索結果が空になるだけで害はない
        //-------------------------------------------------------------
        ChildrenMap m_childrenByParent;
    };

}    // namespace Tsukino::BuiltIn::ECS
