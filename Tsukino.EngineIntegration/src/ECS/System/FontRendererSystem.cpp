//-------------------------------------------------------------
//! @file   FontRendererSystem.cpp
//! @brief  FontRendererSystemクラスの実装
//! @author 山﨑愛
//-------------------------------------------------------------
#include <Tsukino/EngineIntegration/ECS/System/FontRendererSystem.hpp>
#include <Tsukino/EngineIntegration/EngineContext.hpp>

#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/FontComponent.hpp>

#include <Tsukino/Engine/Asset/AssetManager.hpp>
#include <Tsukino/Engine/Asset/Font/FontAsset.hpp>

#include <Tsukino/Renderer/DrawCommand.hpp>

#include <Tsukino/Core/ECS/Registry/Registry.hpp>
#include <Tsukino/Core/Math/Matrix.hpp>

#include <SpriteFont.h>
// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {
    //-------------------------------------------------------------
    //! @brief 更新関数
    //-------------------------------------------------------------
    void FontRendererSystem::Update(Tsukino::ECS::Registry& registry, float deltaTime) {
        Tsukino::EngineIntegration::EngineContext* ctx = registry.GetContext<Tsukino::EngineIntegration::EngineContext*>();

        if(!ctx || !ctx->renderer)
            return;

        // Transform と Font を両方持つエンティティを走査
        auto view = registry.View<TransformComponent, FontComponent>();

        view.each([&](entt::entity entity, const TransformComponent& transform, const FontComponent& font) {
            if(font.text.empty())
                return;

            // --- 1. キャッシュから SpriteFont を取得、なければ生成 ---
            auto& spriteFont = m_fontCache[font.fontHandle];

            if(!spriteFont) {
                // アセットマネージャーからバイナリを取得
                Tsukino::Core::Ref<Tsukino::Asset::IAsset>    asset     = ctx->assetManager->Get(font.fontHandle);
                Tsukino::Core::Ref<Tsukino::Asset::FontAsset> fontAsset = std::static_pointer_cast<Tsukino::Asset::FontAsset>(asset);

                if(!fontAsset)
                    return;

                //-------------------------------------------------------------
                // フォントを生成してキャッシュに保存
                //-------------------------------------------------------------
                spriteFont = ctx->renderer->CreateSpriteFont(fontAsset->m_binaryData.data(), fontAsset->m_binaryData.size());
            }

            // --- 2. Transformから情報を抽出 ---
            // ※親がいるなら .position より worldMatrix.row3.xyz を使うのがプロの設計！
            hlslpp::float3 worldPos   = transform.worldMatrix[3].xyz;    // 行列の4列目がワールド空間での位置
            float          finalScale = hlslpp::length(transform.worldMatrix[0].xyz);

            // --- 3. 描画コマンドの作成 ---
            Tsukino::Renderer::DrawCommand cmd{};

            // キャプチャ：SpriteFont の生ポインタを渡す（このフレーム内なら安全）
            DirectX::SpriteFont* nativeFont = spriteFont.get();

            cmd.customDraw =
                [this, nativeFont, text = font.text, color = font.color, origin = font.origin, worldPos, finalScale](ID3D11DeviceContext* context) {
                    m_spriteBatch->Begin();

                    DirectX::XMFLOAT4 dxColor(color.x, color.y, color.z, color.w);
                    nativeFont->DrawString(m_spriteBatch.get(),
                                           text.c_str(),
                                           DirectX::XMFLOAT2(worldPos.x, worldPos.y),
                                           DirectX::XMLoadFloat4(&dxColor),
                                           0.0f,
                                           DirectX::XMFLOAT2(origin.x, origin.y),
                                           finalScale);

                    m_spriteBatch->End();
                };

            ctx->renderer->PushDrawCommand(cmd);
        });
    }

}    // namespace Tsukino::BuiltIn::ECS
