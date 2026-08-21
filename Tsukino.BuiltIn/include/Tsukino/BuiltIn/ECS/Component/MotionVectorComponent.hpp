//--------------------------------------------------------------
//! @file   MotionVectorComponent.hpp
//! @brief  速度バッファ生成用の前フレームデータ
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
#include <Tsukino/Core/Math/Matrix.hpp>
#include <Tsukino/Core/typedef.hpp>
// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {
    //--------------------------------------------------------------
    //! @struct MotionVectorComponent
    //! @brief  前フレームのワールド行列とボーン行列を保持する実行時キャッシュ
    //! @note   モーションブラーが有効なとき、MotionBlurSystem が
    //!         ModelComponent を持つエンティティへ自動でアタッチする。
    //!
    //!         中身は MotionVectorSnapshotSystem が「フレームの先頭」で
    //!         書き込む。TransformSystem / AnimationSystem が今フレームの値を
    //!         書く前なので、そこで読める値がちょうど前フレームの値になる。
    //!         この順序が崩れると速度が常にゼロになるので、
    //!         System の実行優先度を変更するときは注意すること。
    //!
    //!         1エンティティあたり約8KB。実行時キャッシュなのでシリアライズ
    //!         対象外とし、PrefabFactory には登録しない。
    //--------------------------------------------------------------
    struct MotionVectorComponent {
        static constexpr int MAX_BONES = 128;    // SkeletonOutputComponent::MAX_BONES と一致させること

        Tsukino::Core::Math::matrix prevWorld;                     // 前フレームのワールド行列
        alignas(16) float prevBones[MAX_BONES][16]{};              // 前フレームのボーン行列（スキンメッシュのみ）
        u32  prevBoneCount = 0;                                    // 前フレームのボーン数
        bool valid         = false;                                // 初回フレームは false（速度ゼロ扱い）
    };
}    // namespace Tsukino::BuiltIn::ECS
