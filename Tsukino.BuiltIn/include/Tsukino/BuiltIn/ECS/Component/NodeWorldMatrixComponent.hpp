//-------------------------------------------------------------
//! @file   NodeWorldMatrixComponent.hpp
//! @brief  NodeWorldMatrixComponent構造体の宣言
//-------------------------------------------------------------
#pragma once
#include <Tsukino/Core/Math/Matrix.hpp>

#include <vector>
// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {
    //-------------------------------------------------------------
    //! @struct NodeWorldMatrixComponent
    //! @brief  AnimationSystemが毎フレーム計算する、各ノードのスケール込みグローバル行列
    //!         （globalNodeMatrices、揺れ物補正後の最終値）を他エンティティ（武器のボーン
    //!         ソケットアタッチ等）から参照できるよう公開するコンポーネント。
    //!         SkeletonOutputComponentのスキニング行列と同じ行列がそのまま入っているため、
    //!         見た目（描画されるスキンメッシュ）と完全に一致する“正解”のボーン姿勢として扱える。
    //!         NodeWorldPoseComponent（揺れ物物理用の位置・回転のみの軽量近似。スケールは1と
    //!         仮定）とは異なりスケールを反映済みなので、見た目との一致精度が必要な用途はこちらを使う。
    //!         modelData.nodesのインデックスに対応し、モデルローカル空間（所有エンティティ自身の
    //!         TransformComponentは未反映）である点はNodeWorldPoseComponentと同じ。
    //-------------------------------------------------------------
    struct NodeWorldMatrixComponent {
        std::vector<Tsukino::Core::Math::matrix> matrices;
    };

}    // namespace Tsukino::BuiltIn::ECS
