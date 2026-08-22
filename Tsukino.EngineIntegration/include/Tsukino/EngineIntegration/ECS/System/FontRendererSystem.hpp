//-------------------------------------------------------------
//! @file   FontRendererSystem.hpp
//! @brief  FontRendererSystemクラスの宣言
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once
#include <Tsukino/Core/ECS/System/ISystem.hpp>

#include <Tsukino/Renderer/Renderer.hpp>
#include <Tsukino/Renderer/Text/DynamicFontAtlas.hpp>

#include <Tsukino/Engine/Asset/AssetHandle.hpp>

#include <hlsl++.h>

#include <SpriteBatch.h>
#include <SpriteFont.h>

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {
    //-------------------------------------------------------------
    //! @class  FontRendererSystem
    //! @brief  フォント描画を行うシステム
    //! @note   1フレーム分の描画情報をまとめてsortOrder順に並べ、
    //!         SpriteBatchのBegin/End 1組・DrawCommand 1個で描画する
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
        //-------------------------------------------------------------
        //! @struct DrawEntry
        //! @brief  1エンティティ分の描画情報。1フレーム分を集めてまとめて描く
        //! @note   atlasとspriteFontはどちらか一方だけが非nullになる
        //-------------------------------------------------------------
        struct DrawEntry {
            Tsukino::Renderer::DynamicFontAtlas* atlas      = nullptr;    // 動的フォントアトラス経路
            DirectX::SpriteFont*                 spriteFont = nullptr;    // ベイク済みSpriteFont経路

            std::wstring   text;                            // 描画する文字列
            hlslpp::float2 position     = {0, 0};           // スクリーン座標（左上原点、ピクセル）
            hlslpp::float2 origin       = {0, 0};           // 揃えを解決済みの原点（スケール適用前）
            hlslpp::float4 color        = {1, 1, 1, 1};     // 文字色
            hlslpp::float4 outlineColor = {0, 0, 0, 1};     // 縁取りの色
            float          outlineWidth = 0.0f;             // 縁取りの太さ（ピクセル）
            float          scale        = 1.0f;             // 拡大率
            int            sortOrder    = 0;                // 描画順
        };

        // スプライトバッチのキャッシュ
        std::unordered_map<Tsukino::Asset::AssetHandle, std::unique_ptr<DirectX::SpriteFont>>              m_fontCache;
        // 動的フォントアトラスのキャッシュ（オンデマンドグリフラスタライズ用）
        std::unordered_map<Tsukino::Asset::AssetHandle, std::unique_ptr<Tsukino::Renderer::DynamicFontAtlas>> m_dynamicFontCache;
        std::unique_ptr<DirectX::SpriteBatch>                                                               m_spriteBatch;

        // 1フレーム分の描画情報。毎フレーム冒頭でclearして集め直す
        std::vector<DrawEntry> m_drawEntries;
        // m_drawEntriesをsortOrder順に読むための添字。DrawEntry自体はhlsl++の
        // 16バイトアラインメント型を含み、std::stable_sortの一時バッファが
        // 拡張アラインメントで弾かれるため、並べ替えは添字側で行う
        std::vector<std::uint32_t> m_drawOrder;
    };

}    // namespace Tsukino::BuiltIn::ECS
