//--------------------------------------------------------------
//! @file   SkyAtmosphereSystem.hpp
//! @brief  大気散乱システムの宣言
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
#include <Tsukino/Core/ECS/System/ISystem.hpp>
// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {
    //--------------------------------------------------------------
    //! @class  SkyAtmosphereSystem
    //! @brief  SkyAtmosphereComponentのパラメータをRendererへ転送するシステム
    //--------------------------------------------------------------
    class SkyAtmosphereSystem : public Tsukino::ECS::ISystem {
    public:
        //--------------------------------------------------------------
        //! @brief 更新処理
        //--------------------------------------------------------------
        void Update(Tsukino::ECS::Registry& registry, float deltaTime) override;

    private:
        bool m_pipelineInitialized = false;    // スカイパイプライン初期化済みフラグ
    };
}    // namespace Tsukino::BuiltIn::ECS
