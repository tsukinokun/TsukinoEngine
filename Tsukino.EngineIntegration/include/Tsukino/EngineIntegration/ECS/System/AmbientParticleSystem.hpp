//--------------------------------------------------------------
//! @file   AmbientParticleSystem.hpp
//! @brief  環境パーティクルシステムの宣言
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
#include <Tsukino/Core/ECS/System/ISystem.hpp>
// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {
    //--------------------------------------------------------------
    //! AmbientParticleComponentのパラメータをRendererへ転送するシステム
    //! @note カメラの位置も向きも頂点シェーダーがb0（CBufferScene）から
    //!       直接読むため、このシステムはCameraComponentを一切参照しない。
    //!       やることは時間の累積とパラメータの詰め替えだけ。
    //--------------------------------------------------------------
    class AmbientParticleSystem : public Tsukino::ECS::ISystem {
    public:
        //--------------------------------------------------------------
        //! 更新処理を行います。
        //! @param [in,out] registry  対象のレジストリ
        //! @param [in]     deltaTime 前フレームからの経過秒
        //--------------------------------------------------------------
        void Update(Tsukino::ECS::Registry& registry, float deltaTime) override;

    private:
        //! 粒子を漂わせるための経過時間
        //! @note Rendererに時間を持たせるAPIを増やさず、時間を必要とする
        //!       本システムが自分で累積する（FogSystemと同じ方針）。
        //!       ヒットストップなどで deltaTime が 0 のときは粒子も止まる。
        float m_time = 0.0f;

        //! 粒子数の上限超過を1回だけ警告するためのフラグ
        bool m_countOverflowWarned = false;
    };
}    // namespace Tsukino::BuiltIn::ECS
