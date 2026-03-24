//-------------------------------------------------------------
//! @file   SpriteRenderSystem.hpp
//! @brief  SpriteRenderSystemクラスの宣言
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once
#include <Tsukino/Core/ECS/System/ISystem.hpp>

#include <memory>
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
    };
}    // namespace Tsukino::BuiltIn::ECS
