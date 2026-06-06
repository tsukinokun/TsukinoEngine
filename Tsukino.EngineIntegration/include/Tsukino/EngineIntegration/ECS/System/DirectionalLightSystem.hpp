//--------------------------------------------------------------
//! @file   DirectionalLightSystem.hpp
//! @brief  ディレクショナルライトシステムの宣言
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
#include <Tsukino/Core/ECS/System/ISystem.hpp>
// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {
    //--------------------------------------------------------------
    //! @class  DirectionalLightSystem
    //! @brief  ディレクショナルライトコンポーネントを読んで
    //!         Rendererにライト情報とシャドウパイプラインを渡すシステム
    //--------------------------------------------------------------
    class DirectionalLightSystem : public Tsukino::ECS::ISystem {
    public:
        //--------------------------------------------------------------
        //! @brief システムの更新
        //! @param registry  [in] ECSレジストリ
        //! @param deltaTime [in] 前フレームからの経過時間
        //--------------------------------------------------------------
        void Update(Tsukino::ECS::Registry& registry, float deltaTime) override;

    private:
        bool m_pipelineInitialized = false;    //!< パイプライン初期化済みフラグ
    };
}    // namespace Tsukino::BuiltIn::ECS
