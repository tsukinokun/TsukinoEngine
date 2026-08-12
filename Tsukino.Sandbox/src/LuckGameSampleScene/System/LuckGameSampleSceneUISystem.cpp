//-------------------------------------------------------------
//! @file   LuckGameSampleSceneUISystem.cpp
//! @brief  LuckGameSampleSceneUISystemクラスの実装
//! @author 山﨑愛
//-------------------------------------------------------------
#include <Tsukino/Sandbox/LuckGameSampleScene/ECS/System/LuckGameSampleSceneUISystem.hpp>
#include <Tsukino/Sandbox/LuckGameSampleScene/ECS/Component/PlayerComponent.hpp>
#include <Tsukino/Sandbox/LuckGameSampleScene/ECS/Component/RoundComponent.hpp>
#include <Tsukino/Sandbox/LuckGameSampleScene/ECS/Component/CPUControllerComponent.hpp>
#include <Tsukino/Sandbox/LuckGameSampleScene/ECS/Component/GameStateComponent.hpp>
#include <Tsukino/Sandbox/LuckGameSampleScene/ECS/Component/UILabelTags.hpp>

#include <Tsukino/BuiltIn/ECS/Component/FontComponent.hpp>

#include <string>

// 名前空間 : LuckGameSampleScene::ECS
namespace LuckGameSampleScene::ECS {
    namespace {
        //-------------------------------------------------------------
        //! @brief  役の判定結果を表示用の文字列に変換する
        //! @note   FontRendererSystemが使うデフォルトフォント(Arial.spritefont)は
        //!         日本語グリフを含んでおらず、和文を渡すとDirectX::SpriteFont::DrawStringが
        //!         「Character not in font」例外を投げてクラッシュする。
        //!         日本語対応フォントアセットを別途用意するまでは英数字のみを使うこと。
        //-------------------------------------------------------------
        std::wstring HandToLabel(const RoundComponent& round, bool eliminated) {
            if(eliminated) {
                return L"Eliminated";
            }

            switch(round.kind) {
            case Hand::PinZoro:
                return L"PinZoro!!";
            case Hand::Arashi:
                return L"Arashi(" + std::to_wstring(round.subValue) + L")";
            case Hand::Shigoro:
                return L"Shigoro";
            case Hand::Me:
                return L"Me " + std::to_wstring(round.subValue);
            case Hand::HiFuMi:
                return L"HiFuMi";
            case Hand::MeNashi:
                return L"MeNashi";
            default:
                return L"";
            }
        }

        //-------------------------------------------------------------
        //! @brief  タグを持つラベルエンティティの FontComponent::text を更新する
        //-------------------------------------------------------------
        template <typename TagComponent>
        void UpdateLabelText(Tsukino::ECS::Registry& registry, const std::wstring& text) {
            auto view = registry.View<TagComponent, Tsukino::BuiltIn::ECS::FontComponent>();
            view.each([&](entt::entity, TagComponent&, Tsukino::BuiltIn::ECS::FontComponent& font) { font.text = text; });
        }
    }    // namespace

    //-------------------------------------------------------------
    //! @brief  システムの更新
    //-------------------------------------------------------------
    void LuckGameSampleSceneUISystem::Update(Tsukino::ECS::Registry& registry, float deltaTime) {
        (void)deltaTime;

        if(!registry.HasContext<GameStateComponent>()) {
            return;
        }

        GameStateComponent& state = registry.GetContext<GameStateComponent>();
        if(state.player == entt::null || state.cpu == entt::null) {
            return;
        }

        PlayerComponent& player = registry.GetComponent<PlayerComponent>(state.player);
        PlayerComponent& cpu    = registry.GetComponent<PlayerComponent>(state.cpu);

        RoundComponent& playerRound = registry.GetComponent<RoundComponent>(player.roundEntity);
        RoundComponent& cpuRound    = registry.GetComponent<RoundComponent>(cpu.roundEntity);

        CPUControllerComponent* cpuController = registry.try_get<CPUControllerComponent>(state.cpu);
        const bool               isCpuThinking = (cpuController != nullptr) && (cpuController->rerollDelayTimer > 0.0f);

        std::wstring message;
        switch(state.phase) {
        case GamePhase::Ready:
            message = L"Press SPACE to start";
            break;
        case GamePhase::Rolling:
            if(isCpuThinking) {
                message = L"CPU is thinking...";
            } else if(player.phase == TurnPhase::Waiting) {
                message = L"No hand! Press SPACE to reroll";
            } else {
                message = L"Rolling...";
            }
            break;
        case GamePhase::Compare:
            message = L"Judging...";
            break;
        case GamePhase::Result:
            switch(state.outcome) {
            case RoundOutcome::PlayerWin:
                message = L"You Win! Press SPACE to play again";
                break;
            case RoundOutcome::CpuWin:
                message = L"You Lose... Press SPACE to play again";
                break;
            case RoundOutcome::Draw:
                message = L"Draw! Press SPACE to play again";
                break;
            default:
                message = L"";
                break;
            }
            break;
        }

        UpdateLabelText<PlayerHandLabelTag>(registry, HandToLabel(playerRound, player.eliminated));
        UpdateLabelText<CpuHandLabelTag>(registry, HandToLabel(cpuRound, cpu.eliminated));
        UpdateLabelText<MessageLabelTag>(registry, message);
    }

}    // namespace LuckGameSampleScene::ECS
