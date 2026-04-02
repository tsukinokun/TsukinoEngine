//-------------------------------------------------------------
//! @file   FontRendererSystem.hpp
//! @brief  FontRendererSystemクラスの宣言
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once
#include <Tsukino/Core/ECS/System/ISystem.hpp>

#include <Tsukino/Renderer/Renderer.hpp>

#include <Tsukino/Engine/Asset/AssetHandle.hpp>

#include <SpriteBatch.h>
#include <SpriteFont.h>

#include <memory>
#include <unordered_map>
// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {
    //-------------------------------------------------------------
    //! @class  FontRendererSystem
    //! @brief  フォント描画を行うシステム
    //-------------------------------------------------------------
    class FontRendererSystem : public Tsukino::ECS::ISystem {
    public:
        //-------------------------------------------------------------
        //! @brief デフォルトコンストラクタ
        //-------------------------------------------------------------
        FontRendererSystem() = default;

        //-------------------------------------------------------------
        //! @brief デストラクタ
        //-------------------------------------------------------------
        ~FontRendererSystem() override = default;

        //-------------------------------------------------------------
        // システムの更新
        //! @param  registry    [in] ECS レジストリ
        //! @param  deltaTime   [in] 前フレームからの経過
        //-------------------------------------------------------------
        void Update(Tsukino::ECS::Registry& registry, float deltaTime) override;

    private:
        // スプライトバッチのキャッシュ
        std::unordered_map<Tsukino::Asset::AssetHandle, std::unique_ptr<DirectX::SpriteFont>> m_fontCache;
        std::unique_ptr<DirectX::SpriteBatch>                                                 m_spriteBatch;
    };

}    // namespace Tsukino::BuiltIn::ECS
