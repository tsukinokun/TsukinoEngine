//-------------------------------------------------------------
//! @file   LightStressTestSystem.hpp
//! @brief  多光源ストレステストシステムの宣言
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once
#include <Tsukino/Core/ECS/System/ISystem.hpp>
#include <Tsukino/Core/ECS/Entity/Entity.hpp>

#include <vector>

// 名前空間 : Tsukino::Sandbox::DebugTools::ECS
namespace Tsukino::Sandbox::DebugTools::ECS {
    //-------------------------------------------------------------
    //! @class  LightStressTestSystem
    //! @brief  F1キーで点光源の数を 0 → 1 → 16 → 64 → 0 と切り替え、
    //!         ディファードレンダリングのライトコストを実測するためのデバッグシステム
    //! @note   「ライトのコストがジオメトリ数と独立している」ことの確認が目的。
    //!         生成した点光源は自前で保持し、切り替え時にQueueDestroyで破棄する。
    //!         登録優先度はTransformSystemより前にすること。生成した
    //!         エンティティのworldMatrixが同じフレームで埋まらないと、
    //!         LightSystemが原点にライトがあるものとして扱ってしまう。
    //-------------------------------------------------------------
    class LightStressTestSystem : public Tsukino::ECS::ISystem {
    public:
        //-------------------------------------------------------------
        //! @brief  システムの更新
        //! @param  registry  [in] ECSレジストリ
        //! @param  deltaTime [in] 前フレームからの経過時間
        //-------------------------------------------------------------
        void Update(Tsukino::ECS::Registry& registry, float deltaTime) override;

    private:
        //-------------------------------------------------------------
        //! @brief  現在のライトをすべて破棄する
        //-------------------------------------------------------------
        void DestroyLights(Tsukino::ECS::Registry& registry);

        //-------------------------------------------------------------
        //! @brief  指定個数の点光源を球面上に生成する
        //-------------------------------------------------------------
        void SpawnLights(Tsukino::ECS::Registry& registry, int count);

        //! @brief 切り替えるライト数の並び。F1を押すたびに次へ進む
        static constexpr int kLightCountSteps[] = {0, 1, 16, 64};

        std::vector<Tsukino::ECS::Entity> m_lights;             //!< 生成した点光源エンティティ
        int                               m_stepIndex   = 0;    //!< kLightCountSteps のインデックス
        float                             m_orbitPhase  = 0.0f; //!< 公転アニメーションの位相
        bool                              m_initialized = false;//!< HUD初期化済みか

        //---------------------------------------------------------
        // フレーム時間の移動平均（直近1秒）
        //---------------------------------------------------------
        float m_frameTimeAccum = 0.0f;
        int   m_frameCount     = 0;
        float m_avgFrameMs     = 0.0f;
    };

}    // namespace Tsukino::Sandbox::DebugTools::ECS
