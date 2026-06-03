//-------------------------------------------------------------
//! @file   InGameCameraSystem.cpp
//! @brief  InGameCameraSystemクラスの実装
//! @author 山﨑愛
//-------------------------------------------------------------
#include <Tsukino/Sandbox/JumpGameSample/ECS/System/InGameCameraSystem.hpp>

#include <Tsukino/BuiltIn/ECS/Component/RigidbodyComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/CameraComponent.hpp>
#include <Tsukino/Sandbox/JumpGameSample/ECS/Component/PlayerComponent.hpp>

#include <Tsukino/EngineIntegration/EngineContext.hpp>

#include <Tsukino/Core/Input/InputSystem.hpp>

#include <hlsl++.h>
// 名前空間 : JumpGameSample::ECS
namespace JumpGameSample::ECS {
    //-------------------------------------------------------------
    //! @brief システムの更新
    //-------------------------------------------------------------
    void InGameCameraSystem::Update(Tsukino::ECS::Registry& registry, float deltaTime) {
        //-------------------------------------------------------------
        // コンテキストの取得
        //-------------------------------------------------------------
        Tsukino::EngineIntegration::EngineContext* ctx = registry.GetContext<Tsukino::EngineIntegration::EngineContext*>();
        if(!ctx)
            return;

        //-------------------------------------------------------------
        // 1まだプレイヤーが見つかっていない場合のみ検索する（初期化）
        //-------------------------------------------------------------
        if(playerEntity == entt::null) {
            auto view = registry.View<PlayerComponent>();    // PlayerComponentを持つエンティティを探す
            if(!view.empty()) {
                playerEntity = view.front();
            }
        }

        //-------------------------------------------------------------
        // プレイヤーが見つかっているなら更新処理を実行
        //-------------------------------------------------------------
        if(playerEntity != entt::null) {
            auto& playerTransform = registry.GetComponent<Tsukino::BuiltIn::ECS::TransformComponent>(playerEntity);
            //-------------------------------------------------------------
            // viewを取得して各パドルを更新
            //-------------------------------------------------------------
            auto cameraView = registry.View<Tsukino::BuiltIn::ECS::TransformComponent, Tsukino::BuiltIn::ECS::CameraComponent>();
            cameraView.each(
                [&](Tsukino::ECS::Entity entity, Tsukino::BuiltIn::ECS::TransformComponent& transform, Tsukino::BuiltIn::ECS::CameraComponent& camera) {
                    //-------------------------------------------------------------
                    // 主役のカメラ(3Dカメラ)なら
                    //-------------------------------------------------------------
                    if(camera.isPrimary) {
                        //-------------------------------------------------------------
                        // プレイヤーを映す
                        //-------------------------------------------------------------
                        // 座標移動
                        transform.position.y = playerTransform.position.y;
                        transform.dirty      = true;
                        // 注始点移動
                        camera.lookAtTarget.y = playerTransform.position.y;
                        camera.dirty          = true;
                    }
                });
        }
    }
}    // namespace JumpGameSample::ECS
