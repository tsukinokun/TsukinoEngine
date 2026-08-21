//--------------------------------------------------------------
//! @file   MotionBlurSystem.hpp
//! @brief  モーションブラーシステムの宣言
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
#include <Tsukino/Core/ECS/System/ISystem.hpp>
// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {
    //--------------------------------------------------------------
    //! @class  MotionBlurSystem
    //! @brief  MotionBlurComponent のパラメータを Renderer へ転送するシステム
    //!
    //! @note   MotionVectorComponent の自動アタッチも担当するため、
    //!         ModelSystem（描画コマンド構築）より「前」に実行すること。
    //!         またパラメータを毎フレーム転送する都合上、アプリ側が
    //!         strength を書き換えるシステム（攻撃演出など）より「後」に
    //!         実行する必要がある。
    //--------------------------------------------------------------
    class MotionBlurSystem : public Tsukino::ECS::ISystem {
    public:
        //--------------------------------------------------------------
        //! @brief 更新処理
        //--------------------------------------------------------------
        void Update(Tsukino::ECS::Registry& registry, float deltaTime) override;
    };
}    // namespace Tsukino::BuiltIn::ECS
