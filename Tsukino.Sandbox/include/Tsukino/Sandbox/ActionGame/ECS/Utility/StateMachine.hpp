//-------------------------------------------------------------
//! @file   StateMachine.hpp
//! @brief  StateMachineクラステンプレートの宣言
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once
#include <Tsukino/Core/ECS/Registry/Registry.hpp>
#include <Tsukino/Core/ECS/Entity/Entity.hpp>

#include <functional>
#include <unordered_map>
#include <utility>
// 名前空間 : ActionGame::ECS
namespace ActionGame::ECS {
    //-------------------------------------------------------------
    //! @class  StateMachine
    //! @brief  ステートごとのOnEnter/OnExitコールバックを保持する汎用ステートマシン。
    //!         現在のステート値はコンポーネント側（呼び出し元）が保持し、本クラスは
    //!         ステート定義（コールバック登録）と遷移処理のみを担う。
    //!         1つのシステムインスタンスにつき一度だけRegisterStateで構築し、
    //!         毎フレームはTransitionToを呼ぶだけの使い方を想定している。
    //! @tparam TState ステートを表す型（enum class推奨）
    //-------------------------------------------------------------
    template <typename TState>
    class StateMachine {
    public:
        //! @brief ステート遷移時に呼ばれるコールバック（対象エンティティのコンポーネントを更新する）
        using Callback = std::function<void(Tsukino::ECS::Registry&, Tsukino::ECS::Entity)>;

        //-------------------------------------------------------------
        //! @brief  ステートとその遷移コールバックを登録する
        //! @param  state   [in] 登録するステート
        //! @param  onEnter [in] そのステートに入った時に呼ばれるコールバック（不要ならnullptr）
        //! @param  onExit  [in] そのステートから出る時に呼ばれるコールバック（不要ならnullptr）
        //-------------------------------------------------------------
        void RegisterState(TState state, Callback onEnter = nullptr, Callback onExit = nullptr) {
            m_states[state] = StateCallbacks{std::move(onEnter), std::move(onExit)};
        }

        //-------------------------------------------------------------
        //! @brief  ステートの遷移を試みる。currentStateとdesiredStateが同じ場合は何もしない
        //! @param  currentState [in,out] 呼び出し元（コンポーネント）が保持する現在のステート。遷移時に書き換えられる
        //! @param  desiredState [in]     今フレームであるべきステート
        //! @param  registry     [in]     コールバックへ渡すECSレジストリ
        //! @param  entity       [in]     コールバックへ渡す対象エンティティ
        //! @return 遷移が発生した場合はtrue
        //-------------------------------------------------------------
        bool TransitionTo(TState& currentState, TState desiredState, Tsukino::ECS::Registry& registry, Tsukino::ECS::Entity entity) const {
            if(desiredState == currentState)
                return false;

            if(auto it = m_states.find(currentState); it != m_states.end() && it->second.onExit)
                it->second.onExit(registry, entity);

            currentState = desiredState;

            if(auto it = m_states.find(desiredState); it != m_states.end() && it->second.onEnter)
                it->second.onEnter(registry, entity);

            return true;
        }

    private:
        //! @brief 1ステート分のコールバック一式
        struct StateCallbacks {
            Callback onEnter;
            Callback onExit;
        };

        std::unordered_map<TState, StateCallbacks> m_states;    //!< ステートごとのコールバック登録先
    };
}    // namespace ActionGame::ECS
