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

#include <Tsukino/Renderer/DX11/MeshBuffer.hpp>

#include <cstdint>
#include <deque>
#include <memory>
#include <unordered_map>
#include <vector>

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
        //-------------------------------------------------------------
        // マテリアル実体の置き場は DrawCommandQueue へ移した。
        // Renderer::AllocMaterial() / AllocMaterialData() で確保する。
        // System 側で持つと、System の Update 冒頭でクリアするのに対し
        // コマンドの破棄は Renderer::Render() の中でしか起きないため、
        // 両者の寿命が食い違ってダングリングの余地が残る
        //-------------------------------------------------------------

        //-------------------------------------------------------------
        //! @brief モデルアセットごとの GPU メッシュバッファのキャッシュ
        //! @note  以前はファイルスコープの static だったため、
        //!        (1) D3D11 デバイスより後に解放される静的破棄順序の問題
        //!        (2) シーンを作り直しても解放されず増え続ける問題
        //!        の2つを抱えていた。Scene が所有する本 System のメンバに
        //!        置くことで、Renderer より先に確実に解放される。
        //-------------------------------------------------------------
        std::unordered_map<std::uint64_t, std::vector<Tsukino::Renderer::MeshBuffer>> m_modelMeshCache;
    };
}    // namespace Tsukino::BuiltIn::ECS
