//----------------------------------------------------------------------------
//! @file   UIMotionUtility.hpp
//! @brief  画面UIの見た目だけのずれ（UIMotionComponent）の行列を作るヘルパー
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/UIMotionComponent.hpp>

#include <Tsukino/Core/Math/Matrix.hpp>

// 名前空間 : Tsukino::BuiltIn::ECS::UIMotionUtility
namespace Tsukino::BuiltIn::ECS::UIMotionUtility {

    //! 拡大・回転の中心を返します。
    //! @param  [in] motion    見た目のずれ
    //! @param  [in] transform 同じエンティティのトランスフォーム
    //! @return 中心（親の空間の座標）
    [[nodiscard]] inline hlslpp::float2 ResolvePivot(const UIMotionComponent& motion, const TransformComponent& transform) {
        return motion.pivotIsSelf ? hlslpp::float2(transform.position.xy) : motion.pivot;
    }

    //! ずれが無い（単位の）状態かを返します。
    //! @param  [in] motion 見た目のずれ
    //! @return ずれが無ければ true
    [[nodiscard]] inline bool IsIdentity(const UIMotionComponent& motion) {
        return motion.offset.x == 0.0f && motion.offset.y == 0.0f && motion.scale.x == 1.0f && motion.scale.y == 1.0f && motion.rotation == 0.0f;
    }

    //! ローカル行列のあとに掛ける、見た目のずれの行列を作ります。
    //! @param  [in] motion    見た目のずれ
    //! @param  [in] transform 同じエンティティのトランスフォーム（中心を自分の位置にするとき使う）
    //! @return 「中心へ移す → 拡大 → 回転 → 中心へ戻してずらす」行列（親の空間で働く）
    [[nodiscard]] inline Tsukino::Core::Math::matrix MakeMatrix(const UIMotionComponent& motion, const TransformComponent& transform) {
        const hlslpp::float2 pivot = ResolvePivot(motion, transform);

        // 行ベクトル規約: mul(a, b) は a を先に適用してから b を適用する
        const auto toOrigin = Tsukino::Core::Math::matrix::translate(-pivot.x, -pivot.y, 0.0f);
        const auto scale    = Tsukino::Core::Math::matrix::scale(motion.scale.x, motion.scale.y, 1.0f);
        const auto rotate   = Tsukino::Core::Math::matrix::rotateZ(motion.rotation);
        const auto back     = Tsukino::Core::Math::matrix::translate(pivot.x + motion.offset.x, pivot.y + motion.offset.y, 0.0f);
        return hlslpp::mul(hlslpp::mul(toOrigin, scale), hlslpp::mul(rotate, back));
    }
}    // namespace Tsukino::BuiltIn::ECS::UIMotionUtility
