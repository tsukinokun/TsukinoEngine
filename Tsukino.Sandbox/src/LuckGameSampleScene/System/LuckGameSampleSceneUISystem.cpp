//-------------------------------------------------------------
//! @file   LuckGameSampleSceneUISystem.cpp
//! @brief  LuckGameSampleSceneUISystemクラスの実装
//! @author 山﨑愛
//-------------------------------------------------------------
#include <Tsukino/Sandbox/LuckGameSampleScene/ECS/System/LuckGameSampleSceneUISystem.hpp>
#include <Tsukino/Sandbox/LuckGameSampleScene/ECS/Component/PlayerComponent.hpp>
#include <Tsukino/Sandbox/LuckGameSampleScene/ECS/Component/RoundComponent.hpp>
#include <Tsukino/Sandbox/LuckGameSampleScene/ECS/Component/DiceComponent.hpp>
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

#ifdef _DEBUG
        //-------------------------------------------------------------
        //! @brief  デバッグ用：3つのサイコロの実際の出目を "[2,4,4]" 形式で返す
        //!         （出目未確定のものは "?" とする）
        //-------------------------------------------------------------
        std::wstring DiceValuesDebugLabel(Tsukino::ECS::Registry& registry, const RoundComponent& round) {
            std::wstring text = L" [";
            for(std::size_t i = 0; i < round.dice.size(); ++i) {
                DiceComponent& dice = registry.GetComponent<DiceComponent>(round.dice[i]);
                text += dice.confirmed ? std::to_wstring(dice.confirmedValue) : L"?";
                if(i + 1 < round.dice.size()) {
                    text += L",";
                }
            }
            text += L"]";
            return text;
        }
#endif
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

        // お椀のどちらが自分/敵か一目で分かるよう先頭に付ける
        // （CPU=画面左のお椀、YOU=画面右のお椀。5-4節のカメラ回転の都合でこの左右になる）
        std::wstring playerLabel = L"YOU: " + HandToLabel(playerRound, player.eliminated);
        std::wstring cpuLabel    = L"CPU: " + HandToLabel(cpuRound, cpu.eliminated);

#ifdef _DEBUG
        // デバッグ時のみ、役だけでなく実際の出目も表示する（本来は隠し情報だが、
        // 検証しやすいようDebugビルド限定でCPU側も含めて表示する）
        playerLabel += DiceValuesDebugLabel(registry, playerRound);
        cpuLabel += DiceValuesDebugLabel(registry, cpuRound);
#endif

        UpdateLabelText<PlayerHandLabelTag>(registry, playerLabel);
        UpdateLabelText<CpuHandLabelTag>(registry, cpuLabel);
        UpdateLabelText<MessageLabelTag>(registry, message);
    }

}    // namespace LuckGameSampleScene::ECS
