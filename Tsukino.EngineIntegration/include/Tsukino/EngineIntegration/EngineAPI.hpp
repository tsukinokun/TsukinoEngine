//------------------------------------------------------------
//! @file    EngineAPI.hpp
//! @brief   エンジンからAPIを提供するクラスの宣言
//! @author  山﨑愛
//------------------------------------------------------------
#pragma once
#include <Tsukino/Core/ECS/Registry/Registry.hpp>
// 前方宣言
namespace Tsukino {
    namespace ECS {
        class ISystem;
    }
}

// 名前空間 : Tsukino::EngineIntegration
namespace Tsukino::EngineIntegration {
    struct EngineContext;    // 前方宣言

    //------------------------------------------------------------
    //! @class   EngineAPI
    //! @brief   エンジンからAPIを提供するクラス
    //------------------------------------------------------------
    class EngineAPI {
    public:
        //------------------------------------------------------------
        // コンストラクタ
        //! @param   context エンジン全体で共有されるクラスのポインタを集めた構造体への参照
        //------------------------------------------------------------
        explicit EngineAPI(EngineContext& context, Tsukino::ECS::Registry& registry);

        //------------------------------------------------------------
        // メッセージ処理関数
        //! @return メッセージ処理が成功した場合は true、ウィンドウが閉じられた場合は false
        //------------------------------------------------------------
        [[nodiscard]]
        bool ProcessMessages();

        //------------------------------------------------------------
        //! @brief  現在のシーンの Registry (ECS) を取得する
        //! @return Registry への参照
        //------------------------------------------------------------
        [[nodiscard]]
        Tsukino::ECS::Registry& GetRegistry() {
            return m_registry;
        }

        //------------------------------------------------------------
        // システムの追加関数
        //! @param  system      [in] 追加するシステム
        //! @param  priority    [in] システムの優先度
        //------------------------------------------------------------
        void AddSystem(std::shared_ptr<Tsukino::ECS::ISystem> system, int priority);

        //------------------------------------------------------------
        // 更新関数
        //------------------------------------------------------------
        void Update(float deltaTime);

        //------------------------------------------------------------
        // 描画関数
        //------------------------------------------------------------
        void Render();

    private:
        EngineContext&          m_context;     // コンテキストへの参照
        Tsukino::ECS::Registry& m_registry;    // ECSレジストリへの参照
    };

}    // namespace Tsukino::EngineIntegration
