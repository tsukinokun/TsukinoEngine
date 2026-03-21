//-------------------------------------------------------------
//! @file   TransformSystem.hpp
//! @brief  TransformSystemクラスの宣言
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once
#include <Tsukino/Core/ECS/System/ISystem.hpp>
#include <Tsukino/Core/Math/Matrix.hpp>
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
        //-------------------------------------------------------------
        static void UpdateWorldMatrixRecursive(Tsukino::ECS::Registry&            registry,
                                               Tsukino::ECS::Entity               entity,
                                               const Tsukino::Core::Math::matrix& parentWorld) noexcept;

        // 一時バッファ（メモリ確保の最適化用）
        std::vector<Tsukino::ECS::Entity> m_rootEntities;
    };

}    // namespace Tsukino::BuiltIn::ECS
