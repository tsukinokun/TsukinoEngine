//-------------------------------------------------------------
//! @file   ModelSystem.hpp
//! @brief  モデル描画システム構造体の宣言
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once

#include <Tsukino/Core/ECS/System/ISystem.hpp>
#include <Tsukino/Core/ECS/Registry/Registry.hpp>
#include <Tsukino/EngineIntegration/EngineContext.hpp>
#include <Tsukino/Engine/ECS/Scene.hpp>
#include <Tsukino/Renderer/DX11/PipelineState.hpp>
#include <Tsukino/Renderer/DrawCommand.hpp>
#include <Tsukino/Renderer/DX11/Material.hpp>
#include <Tsukino/Renderer/ConstantBuffer.hpp>

#include <memory>
#include <deque>

// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {
    //-------------------------------------------------------------
    //! @class  ModelSystem
    //! @brief  モデルを描画するシステム
    //-------------------------------------------------------------
    class ModelSystem : public Tsukino::ECS::ISystem {
    public:
        //-------------------------------------------------------------
        //! @brief システムの更新
        //! @param registry  [in] エンティティレジストリ
        //! @param deltaTime [in] デルタタイム
        //-------------------------------------------------------------
        void Update(Tsukino::ECS::Registry& registry, float deltaTime) override;

    private:
        // マテリアル実体のバッファ
        std::deque<Tsukino::Renderer::Material> m_materialBuffer;

        // 定数バッファのバッファ
        std::deque<Tsukino::Renderer::CBufferMaterial> m_cbufferMaterialBuffer;
    };
}    // namespace Tsukino::BuiltIn::ECS
