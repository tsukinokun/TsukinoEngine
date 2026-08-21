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

#include <memory>
#include <deque>
#include <unordered_map>
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
        std::shared_ptr<Tsukino::Renderer::PipelineState> m_pipelineCache;    // パイプラインステートのキャッシュ
        std::deque<Tsukino::Renderer::Material> m_materialBuffer;    // 描画コマンド実行までポインタを維持しつつ、フレーム毎にリサイクルするためのバッファ
        std::deque<Tsukino::Renderer::CBufferMaterial>
            m_materialDataBuffer;    // SpriteComponent::tintColorをb2(CBufferMaterial::baseColor)へ渡すためのバッファ（m_materialBufferと同じ理由でdeque）
        std::unordered_map<Tsukino::Asset::AssetHandle, ID3D11ShaderResourceView*>
            m_textureCache;    // ハンドルをキーにしてテクスチャのSRVをキャッシュ（毎フレームのアセット検索を回避）
    };
}    // namespace Tsukino::BuiltIn::ECS
