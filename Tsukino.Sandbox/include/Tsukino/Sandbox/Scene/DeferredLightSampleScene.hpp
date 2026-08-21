//-------------------------------------------------------------
//! @file    DeferredLightSampleScene.hpp
//! @brief   ディファードレンダリングの多光源ショーケースシーンの宣言
//! @author  山﨑愛
//-------------------------------------------------------------
#pragma once
#include <Tsukino/EngineIntegration/Scene/GameSceneBase.hpp>
#include <Tsukino/Core/ECS/Entity/Entity.hpp>
// 名前空間 : Tsukino::Sandbox
namespace Tsukino::Sandbox {
    //-------------------------------------------------------------
    //! @class   DeferredLightSampleScene
    //! @brief   ディファードレンダリングの多光源を見せるためのサンプルシーン
    //! @note    ゲームロジックを持たず、床と数体のモデルに対して
    //!          色とりどりの点光源・スポットライトを当てるだけの構成。
    //!          F1キー（LightStressTestSystem）でライト数を
    //!          0 → 1 → 16 → 64 と切り替えて負荷を確認できる。
    //!          カメラはデバッグカメラなので右ドラッグ＋WASDで見回せる。
    //-------------------------------------------------------------
    class DeferredLightSampleScene : public Tsukino::EngineIntegration::GameSceneBase {
    public:
        //-------------------------------------------------------------
        //! @brief  コンストラクタ
        //-------------------------------------------------------------
        DeferredLightSampleScene() = default;

        //-------------------------------------------------------------
        //! @brief  デストラクタ
        //-------------------------------------------------------------
        ~DeferredLightSampleScene() override = default;

        //-------------------------------------------------------------
        //! @brief  シーンの更新
        //! @param  api       [in] エンジンから提供されるAPIへの参照
        //! @param  deltaTime [in] 前フレームからの経過時間
        //-------------------------------------------------------------
        void OnUpdate(Tsukino::EngineIntegration::EngineAPI& api, float deltaTime) override;

        //-------------------------------------------------------------
        //! @brief  シーンの終了処理
        //-------------------------------------------------------------
        void OnExit() override;

    private:
        //-------------------------------------------------------------
        //! @brief  シーン固有の初期化処理
        //! @param  api [in] エンジンから提供されるAPIへの参照
        //-------------------------------------------------------------
        void OnInitialize(Tsukino::EngineIntegration::EngineAPI& api) override;

        //! @brief カメラエンティティ。OnUpdateで中心の周りを自動公転させる
        //! @note  DebugCameraSystemは_DEBUGビルドにしか存在しないため、
        //!        Release/Debugどちらでも同じ絵が出るよう自前で回す。
        //!        全方位から光の当たり方が見えるのでショーケースにも都合がよい。
        Tsukino::ECS::Entity m_cameraEntity{entt::null};
        float                m_cameraAngle = 0.0f;
    };
}    // namespace Tsukino::Sandbox
