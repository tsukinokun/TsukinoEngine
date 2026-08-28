//-------------------------------------------------------------
//! @file   InteractionSystem.cpp
//! @brief  InteractionSystemクラスの実装
//! @author 山﨑愛
//-------------------------------------------------------------
#include <Tsukino/EngineIntegration/ECS/System/InteractionSystem.hpp>
#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/DraggableComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/SpriteComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Transform/TransformUtility.hpp>

#include <Tsukino/EngineIntegration/EngineContext.hpp>

#include <Tsukino/Engine/Asset/AssetManager.hpp>
#include <Tsukino/Engine/Asset/Texture/TextureAsset.hpp>

#include <Tsukino/Core/Input/InputSystem.hpp>
#include <Tsukino/Core/ECS/Registry/Registry.hpp>
#include <Tsukino/Core/Math/MathHelper.hpp>

#include <hlsl++.h>
#include <entt/entt.hpp>

#include <limits>

// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {
    namespace {
        namespace TU = Tsukino::BuiltIn::ECS::TransformUtility;

        //-------------------------------------------------------------
        //! @brief  スプライトの実サイズ（ワールドスケール考慮）を求め、指定座標が
        //!         その矩形内にあるかを判定する
        //! @note   位置・スケールをworldMatrixから取るため、親に付いた子スプライトでも
        //!         正しく判定できる。回転は無視した軸並行矩形（AABB）判定
        //! @return テクスチャアセットが取得できた場合は true
        //-------------------------------------------------------------
        bool ComputeSpriteHit(Tsukino::EngineIntegration::EngineContext* ctx, const TransformComponent& transform, const SpriteComponent& sprite,
                              const hlslpp::float2& point, bool* outInside) {
            auto textureAsset = std::static_pointer_cast<Tsukino::Asset::TextureAsset>(ctx->assetManager->Get(sprite.textureHandle));
            if(!textureAsset)
                return false;

            const hlslpp::float3 worldPosition = TU::GetWorldPosition(transform);
            const hlslpp::float2 worldScale    = TU::GetWorldScale2D(transform);

            // スケールを考慮した実際の描画サイズを計算
            hlslpp::float2 spriteSize = {static_cast<float>(textureAsset->width) * worldScale.x, static_cast<float>(textureAsset->height) * worldScale.y};
            hlslpp::float2 spritePos  = {worldPosition.x, worldPosition.y};

            *outInside = Tsukino::Core::Math::IsPointInRect(point, spritePos, spriteSize);
            return true;
        }

        //-------------------------------------------------------------
        //! @brief  指定座標の下にあるスプライトのうち、最前面のものを1つ返す
        //! @note   以前はViewをeachで回して「重なっているdraggable全部」の
        //!         ドラッグを同時に開始していたため、複数スプライトが同じ座標に
        //!         あると偶然まとめて動いていた。1つだけ選ぶように変更している
        //! @return 見つからなければ entt::null
        //-------------------------------------------------------------
        Tsukino::ECS::Entity PickTopmostSprite(Tsukino::ECS::Registry& registry, Tsukino::EngineIntegration::EngineContext* ctx, const hlslpp::float2& point) {
            Tsukino::ECS::Entity picked      = entt::null;
            int                  pickedOrder = std::numeric_limits<int>::min();

            registry.View<TransformComponent, SpriteComponent>().each([&](auto entity, const TransformComponent& transform, const SpriteComponent& sprite) {
                bool isInside = false;
                if(!ComputeSpriteHit(ctx, transform, sprite, point, &isInside) || !isInside)
                    return;

                // sortOrderが大きいものほど手前に描かれる。同値なら後に見つかった方を採用
                if(sprite.sortOrder >= pickedOrder) {
                    pickedOrder = sprite.sortOrder;
                    picked      = entity;
                }
            });

            return picked;
        }

        //-------------------------------------------------------------
        //! @brief  指定エンティティから親を遡り、最も近いDraggableComponent持ちを返す
        //! @note   これにより「手やカウンターを掴んでも、その親であるペンギン本体
        //!         （＝ドラッグの根）が動く」という挙動になる。子はTransformSystemが
        //!         ワールド行列を伝播させるので追従コードは要らない
        //! @return 見つからなければ entt::null
        //-------------------------------------------------------------
        Tsukino::ECS::Entity FindDragRoot(Tsukino::ECS::Registry& registry, Tsukino::ECS::Entity entity) {
            Tsukino::ECS::Entity current = entity;

            for(int depth = 0; depth < TU::kMaxHierarchyDepth; ++depth) {
                if(current == entt::null || !registry.HasComponent<TransformComponent>(current))
                    return entt::null;

                if(registry.HasComponent<DraggableComponent>(current))
                    return current;

                current = registry.GetComponent<TransformComponent>(current).parent;
            }

            // 深すぎる（または循環している）。TransformSystem側で警告を出しているのでここでは黙って諦める
            return entt::null;
        }
    }    // namespace

    //-------------------------------------------------------------
    //! @brief システムの更新
    //-------------------------------------------------------------
    void InteractionSystem::Update(Tsukino::ECS::Registry& registry, float deltaTime) {
        Tsukino::EngineIntegration::EngineContext* ctx = registry.GetContext<Tsukino::EngineIntegration::EngineContext*>();
        if(!ctx || !ctx->renderer)
            return;

        auto& input = ctx->inputSystem;
        i32   mouseX, mouseY;
        input->GetMousePosition(&mouseX, &mouseY);
        hlslpp::float2 mousePos = {(float)mouseX, (float)mouseY};

        //-------------------------------------------------------------
        // ドラッグ中のエンティティの更新。
        //
        // 当たり判定より先に、かつ当たり判定の成否と無関係に処理する。
        // 以前はテクスチャ未ロード時に早期returnしていたため、その間にボタンを
        // 離すと isDragging が true のまま張り付いていた
        //-------------------------------------------------------------
        bool isAnyDragging = false;

        registry.View<TransformComponent, DraggableComponent>().each([&](auto entity, const TransformComponent& transform, DraggableComponent& draggable) {
            if(!draggable.isDragging)
                return;

            if(!input->IsKeyDown(Input::KeyCode::LButton)) {
                draggable.isDragging = false;
                return;
            }

            isAnyDragging = true;

            //-----------------------------------------------------
            // ワールド座標で指定する。SetWorldPositionが「親の逆行列を通した
            // ローカル座標への変換」と「dirtyの設定」をまとめて行うため、
            // ここで dirty を立て忘れる事故が起きない
            // （Zは掴んだときの奥行きをそのまま維持する）
            //-----------------------------------------------------
            const hlslpp::float3 currentWorld = TU::GetWorldPosition(transform);
            const hlslpp::float3 targetWorld  = {mousePos.x - draggable.dragOffset.x, mousePos.y - draggable.dragOffset.y, currentWorld.z};

            TU::SetWorldPosition(registry, entity, targetWorld);
        });

        //-------------------------------------------------------------
        // 新規ドラッグの開始（既にドラッグ中なら何もしない）
        //-------------------------------------------------------------
        if(isAnyDragging || !input->IsKeyPressed(Input::KeyCode::LButton))
            return;

        const Tsukino::ECS::Entity hitEntity = PickTopmostSprite(registry, ctx, mousePos);
        if(hitEntity == entt::null)
            return;

        const Tsukino::ECS::Entity dragRoot = FindDragRoot(registry, hitEntity);
        if(dragRoot == entt::null)
            return;

        auto&                dragTransform = registry.GetComponent<TransformComponent>(dragRoot);
        auto&                draggable     = registry.GetComponent<DraggableComponent>(dragRoot);
        const hlslpp::float3 worldPosition = TU::GetWorldPosition(dragTransform);

        draggable.isDragging = true;
        draggable.dragOffset = mousePos - hlslpp::float2(worldPosition.x, worldPosition.y);
    }

    //-------------------------------------------------------------
    //! @brief  指定座標がドラッグ可能なスプライト上、またはドラッグ中の
    //!         スプライトが存在するかを調べる関数
    //-------------------------------------------------------------
    bool InteractionSystem::HitTest(Tsukino::ECS::Registry& registry, float x, float y) {
        Tsukino::EngineIntegration::EngineContext* ctx = registry.GetContext<Tsukino::EngineIntegration::EngineContext*>();
        if(!ctx)
            return false;

        //-------------------------------------------------------------
        // ドラッグ中はカーソルがドラッグオフセット分ずれて矩形外に出ることが
        // あるため、そのままクリック透過扱いになるのを防ぐ
        //-------------------------------------------------------------
        bool isAnyDragging = false;
        registry.View<DraggableComponent>().each([&](const DraggableComponent& draggable) {
            if(draggable.isDragging)
                isAnyDragging = true;
        });

        if(isAnyDragging)
            return true;

        //-------------------------------------------------------------
        // 掴める塊の一部（＝親を辿るとDraggableに行き着くスプライト）の上なら
        // クリックを受け取る。子スプライト（手やカウンター背景）自身が
        // DraggableComponentを持っていなくても透過しない
        //-------------------------------------------------------------
        const hlslpp::float2       point     = {x, y};
        const Tsukino::ECS::Entity hitEntity = PickTopmostSprite(registry, ctx, point);
        if(hitEntity == entt::null)
            return false;

        return FindDragRoot(registry, hitEntity) != entt::null;
    }

}    // namespace Tsukino::BuiltIn::ECS
