//--------------------------------------------------------------
//! @file   BloomSystem.hpp
//! @brief  ブルームシステムの宣言
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
#include <Tsukino/Core/ECS/System/ISystem.hpp>
// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {
    //--------------------------------------------------------------
    //! @class  BloomSystem
    //! @brief  BloomComponent のパラメータを Renderer へ転送するシステム
    //! @note   パラメータを毎フレーム転送する都合上、アプリ側が
    //!         intensity などを書き換えるシステム（演出）より「後」に実行すること
    //--------------------------------------------------------------
    class BloomSystem : public Tsukino::ECS::ISystem {
    public:
        //--------------------------------------------------------------
        //! @brief 更新処理
        //--------------------------------------------------------------
        void Update(Tsukino::ECS::Registry& registry, float deltaTime) override;
    };
}    // namespace Tsukino::BuiltIn::ECS
