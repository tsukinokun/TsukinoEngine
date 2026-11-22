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
#include <Tsukino/Renderer/DrawCommand.hpp>
#include <Tsukino/Renderer/Material.hpp>
#include <Tsukino/Renderer/ConstantBuffer.hpp>

#include <Tsukino/Renderer/MeshBuffer.hpp>
#include <Tsukino/Renderer/GpuHandles.hpp>

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
        //! @brief 自作シェーダー用の定数バッファを返し、破棄シグナルの購読を解除します。
        //-------------------------------------------------------------
        ~ModelSystem() override;

        //-------------------------------------------------------------
        //! @brief システムの更新
        //! @param registry  [in] エンティティレジストリ
        //! @param deltaTime [in] デルタタイム
        //-------------------------------------------------------------
        void Update(Tsukino::ECS::Registry& registry, float deltaTime) override;

    private:
        //-------------------------------------------------------------
        //! @brief CustomShaderComponent のエンティティの定数バッファを取得します（無ければ作る）。
        //! @param ctx    [in] エンジンのコンテキスト
        //! @param entity [in] エンティティ
        //! @return 定数バッファ（作れなければ無効なハンドル）
        //-------------------------------------------------------------
        Tsukino::Renderer::BufferHandle AcquireCustomShaderConstants(Tsukino::EngineIntegration::EngineContext& ctx, entt::entity entity);

        //-------------------------------------------------------------
        //! @brief CustomShaderComponent が外されたとき（エンティティの破棄を含む）に定数バッファを返します。
        //! @param registry [in] EnTT のレジストリ
        //! @param entity   [in] 外されたエンティティ
        //-------------------------------------------------------------
        void OnCustomShaderDestroyed(entt::registry& registry, entt::entity entity);

        //-------------------------------------------------------------
        //! @brief 自作シェーダーの params（u_user0）を渡す、エンティティごとの定数バッファ
        //! @note  描画は Renderer::Render() でまとめて行われるので、エンティティごとに別のバッファが要る
        //!        （1つを共有すると、最後に書いた値で全部が描かれる）。
        //!        回収は OnDestroy（どの破棄の経路でも発火する）とデストラクタで行う
        //-------------------------------------------------------------
        std::unordered_map<entt::entity, Tsukino::Renderer::BufferHandle> m_customShaderConstants;
        Tsukino::ECS::Registry*                    m_registry = nullptr;    // 破棄シグナルを購読したレジストリ
        Tsukino::EngineIntegration::EngineContext* m_context  = nullptr;    // バッファの返却に使うコンテキスト

        //-------------------------------------------------------------
        // マテリアル実体の置き場は DrawCommandQueue へ移した。
        // Renderer::GetDrawQueue().AllocMaterial() / AllocMaterialData() で確保する。
        // System 側で持つと、System の Update 冒頭でクリアするのに対し
        // コマンドの破棄は Renderer::Render() の中でしか起きないため、
        // 両者の寿命が食い違ってダングリングの余地が残る
        //-------------------------------------------------------------

        //-------------------------------------------------------------
        //! @brief モデルアセットごとの GPU メッシュバッファのキャッシュ
        //! @note  以前はファイルスコープの static だったため、
        //!        (1) 描画デバイスより後に解放される静的破棄順序の問題
        //!        (2) シーンを作り直しても解放されず増え続ける問題
        //!        の2つを抱えていた。Scene が所有する本 System のメンバに
        //!        置くことで、Renderer より先に確実に解放される。
        //-------------------------------------------------------------
        std::unordered_map<std::uint64_t, std::vector<Tsukino::Renderer::MeshBuffer>> m_modelMeshCache;
    };
}    // namespace Tsukino::BuiltIn::ECS
