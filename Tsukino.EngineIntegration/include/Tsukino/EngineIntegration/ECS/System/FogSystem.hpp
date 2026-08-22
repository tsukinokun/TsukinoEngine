//--------------------------------------------------------------
//! @file   FogSystem.hpp
//! @brief  フォグシステムの宣言
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
#include <Tsukino/Core/ECS/System/ISystem.hpp>
// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {
    //--------------------------------------------------------------
    //! @class  FogSystem
    //! @brief  FogComponentのパラメータをRendererへ転送するシステム
    //--------------------------------------------------------------
    class FogSystem : public Tsukino::ECS::ISystem {
    public:
        //--------------------------------------------------------------
        //! @brief 更新処理
        //--------------------------------------------------------------
        void Update(Tsukino::ECS::Registry& registry, float deltaTime) override;

    private:
        //! @brief ノイズを流すための経過時間
        //! @note  Rendererに時間を持たせる（UpdateWaterTimeのような）APIを増やさず、
        //!        時間を必要とする本システムが自分で累積する。
        //!        ヒットストップなどで deltaTime が 0 のときは霧も止まる。
        float m_time = 0.0f;
    };
}    // namespace Tsukino::BuiltIn::ECS
