//-------------------------------------------------------------
//! @file   SpriteRenderSystem.hpp
//! @brief  SpriteRenderSystemクラスの宣言
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once
#include <Tsukino/Core/ECS/System/ISystem.hpp>

#include <Tsukino/Renderer/DX11/Material.hpp>
#include <Tsukino/Renderer/ConstantBuffer.hpp>

#include <Tsukino/Engine/Asset/AssetHandle.hpp>

#include <Tsukino/Renderer/DrawCommand.hpp>

#include <memory>
#include <deque>
#include <unordered_map>
#include <vector>
namespace Tsukino::Renderer {
    struct PipelineState;    // 前方宣言
}

// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {
    //-------------------------------------------------------------
    //! @class  SpriteRenderSystem
    //! @brief  SpriteComponentとTransformComponentを持つエンティティを描画するシステム
    //-------------------------------------------------------------
    class SpriteRenderSystem : public Tsukino::ECS::ISystem {
    public:
        //-------------------------------------------------------------
        //! @brief  デフォルトコンストラクタ
        //-------------------------------------------------------------
        SpriteRenderSystem() = default;

        //-------------------------------------------------------------
        //! @brief  デストラクタ
        //-------------------------------------------------------------
        ~SpriteRenderSystem() override = default;

        //-------------------------------------------------------------
        //! @brief  システムの更新（ここでRendererに描画コマンドを送る）
        //! @param  registry    [in] ECS レジストリ
        //! @param  deltaTime   [in] 前フレームからの経過時間
        //-------------------------------------------------------------
        void Update(Tsukino::ECS::Registry& registry, float deltaTime) override;

    private:
        std::shared_ptr<Tsukino::Renderer::PipelineState> m_pipelineCache;            // パイプラインステートのキャッシュ（Screen空間・Alpha）
        std::shared_ptr<Tsukino::Renderer::PipelineState> m_additivePipelineCache;    // パイプラインステートのキャッシュ（Screen空間・Additive。発光表現用）
        std::shared_ptr<Tsukino::Renderer::PipelineState> m_worldPipelineCache;            // パイプラインステートのキャッシュ（World空間・Alpha。深度テストあり）
        std::shared_ptr<Tsukino::Renderer::PipelineState> m_worldAdditivePipelineCache;    // パイプラインステートのキャッシュ（World空間・Additive。深度テストあり）
        //-------------------------------------------------------------
        // マテリアル実体と定数データの置き場は DrawCommandQueue へ移した。
        // Renderer::AllocMaterial() / AllocMaterialData() で確保する
        //-------------------------------------------------------------
        std::unordered_map<Tsukino::Asset::AssetHandle, ID3D11ShaderResourceView*>
            m_textureCache;    // ハンドルをキーにしてテクスチャのSRVをキャッシュ（毎フレームのアセット検索を回避）

        //-------------------------------------------------------------
        //! @struct SpriteEntry
        //! @brief  sortOrderで並べ替えるまで描画コマンドを一時的に保持する要素
        //-------------------------------------------------------------
        struct SpriteEntry {
            int                            sortOrder = 0;
            Tsukino::Renderer::DrawCommand cmd;
        };

        //! 並べ替え用の一時バッファ。
        //! 以前はUpdate内のローカル変数だったため、毎フレーム確保し直していた。
        //! メンバにしてclear()で使い回すことで、ウォームアップ後は追加確保が起きない
        std::vector<SpriteEntry> m_entries;
    };
}    // namespace Tsukino::BuiltIn::ECS
