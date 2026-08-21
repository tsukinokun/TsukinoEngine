//--------------------------------------------------------------
//! @file   MotionVectorSnapshotSystem.hpp
//! @brief  前フレームのトランスフォーム／ボーン行列を退避するシステムの宣言
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
#include <Tsukino/Core/ECS/System/ISystem.hpp>
// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {
    //--------------------------------------------------------------
    //! @class  MotionVectorSnapshotSystem
    //! @brief  MotionVectorComponent へ前フレームの値を退避するシステム
    //!
    //! @note   【実行順序が仕様そのものになっているので注意】
    //!         このシステムは必ず TransformSystem / AnimationSystem より
    //!         「前」に実行すること。フレームの先頭で TransformComponent と
    //!         SkeletonOutputComponent を読むと、それらはまだ今フレームの値に
    //!         更新されていない＝前フレームの値そのものになっている。
    //!         これを利用して、ダブルバッファも遅延コピーもなしに
    //!         前フレームの値を安全に取り出している。
    //!
    //!         DrawCommand は SkeletonOutputComponent を生ポインタで指すため、
    //!         「同じフレーム内で現在値を prev へコピーする」実装にすると
    //!         コマンドが指す prev が現在値で潰れてしまう。それを避けるための
    //!         設計でもある。
    //!
    //!         モーションブラーの有効・無効に関わらず毎フレーム実行する。
    //!         無効な間サボると、再有効化した瞬間に何フレームも前の値が
    //!         prev として使われ、巨大な速度で画面が1フレームだけ大きく滲む。
    //--------------------------------------------------------------
    class MotionVectorSnapshotSystem : public Tsukino::ECS::ISystem {
    public:
        //--------------------------------------------------------------
        //! @brief 更新処理
        //--------------------------------------------------------------
        void Update(Tsukino::ECS::Registry& registry, float deltaTime) override;
    };
}    // namespace Tsukino::BuiltIn::ECS
