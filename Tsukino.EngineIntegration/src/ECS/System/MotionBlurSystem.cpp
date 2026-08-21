//--------------------------------------------------------------
//! @file   MotionBlurSystem.cpp
//! @brief  モーションブラーシステムの実装
//! @author 山﨑愛
//--------------------------------------------------------------
#include <Tsukino/EngineIntegration/ECS/System/MotionBlurSystem.hpp>
#include <Tsukino/EngineIntegration/EngineContext.hpp>
#include <Tsukino/BuiltIn/ECS/Component/MotionBlurComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/MotionVectorComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/ModelComponent.hpp>
#include <Tsukino/Renderer/Renderer.hpp>
#include <Tsukino/Renderer/ConstantBuffer.hpp>

#include <entt/entt.hpp>

#include <algorithm>
#include <vector>

// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {
    //--------------------------------------------------------------
    //! @brief システムの更新
    //--------------------------------------------------------------
    void MotionBlurSystem::Update(Tsukino::ECS::Registry& registry, float deltaTime) {
        Tsukino::EngineIntegration::EngineContext* ctx = registry.GetContext<Tsukino::EngineIntegration::EngineContext*>();
        if(!ctx || !ctx->renderer)
            return;

        //--------------------------------------------------------------
        // MotionBlurComponent を探す
        // 複数あった場合は最初の1つだけ使用する（SkyAtmosphereSystemと同じ流儀）
        //--------------------------------------------------------------
        const MotionBlurComponent* activeBlur = nullptr;

        auto blurView = registry.View<MotionBlurComponent>();
        blurView.each([&](entt::entity entity, const MotionBlurComponent& blur) {
            if(!activeBlur && blur.enabled)
                activeBlur = &blur;
        });

        //--------------------------------------------------------------
        // コンポーネントが無い／無効なら明示的に切る
        //
        // Renderer 側のフラグはフレーム単位で自動的に false へ戻るので
        // 本来ここは省略できるが、「有効化しない＝無効」という暗黙の
        // 挙動に頼らず意思を明示しておく。
        //--------------------------------------------------------------
        if(!activeBlur) {
            ctx->renderer->SetMotionBlurEnabled(false);
            return;
        }

        //--------------------------------------------------------------
        // パラメータを Renderer へ転送
        //--------------------------------------------------------------
        Tsukino::Renderer::CBufferMotionBlur params{};
        params.strength      = activeBlur->strength;
        params.maxBlurRadius = activeBlur->maxBlurRadius;
        params.sampleCount   = std::clamp(activeBlur->sampleCount, 1, Tsukino::Renderer::kMotionBlurMaxSamples);

        //--------------------------------------------------------------
        // シャッター補正
        //
        // 速度バッファには「1フレームあたりの移動量」しか入っていないため、
        // フレームレートが変わるとブラーの長さも変わってしまう。
        // targetFps * deltaTime を掛けて基準fpsでの見た目に揃える。
        // deltaTime が 0（ヒットストップで完全停止など）のときは
        // ブラーも止まるのが正しい。
        //--------------------------------------------------------------
        params.shutterScale = activeBlur->targetFps * deltaTime;

        ctx->renderer->SetMotionBlurParameters(params);
        ctx->renderer->SetMotionBlurEnabled(true);

        //--------------------------------------------------------------
        // 速度を出すエンティティに MotionVectorComponent を自動アタッチする
        //
        // ビューの走査中に emplace するとイテレータが無効化される恐れが
        // あるため、対象を集めてから走査完了後にまとめて追加する。
        //--------------------------------------------------------------
        std::vector<entt::entity> pendingAttach;

        auto modelView = registry.View<ModelComponent>();
        modelView.each([&](entt::entity entity, const ModelComponent&) {
            if(!registry.HasComponent<MotionVectorComponent>(entity))
                pendingAttach.push_back(entity);
        });

        for(entt::entity entity : pendingAttach) {
            // アタッチ直後は valid == false なので、最初の1フレームは速度ゼロ。
            // 次フレームの MotionVectorSnapshotSystem が値を埋める。
            (void)registry.AddComponent<MotionVectorComponent>(entity);
        }
    }

}    // namespace Tsukino::BuiltIn::ECS
