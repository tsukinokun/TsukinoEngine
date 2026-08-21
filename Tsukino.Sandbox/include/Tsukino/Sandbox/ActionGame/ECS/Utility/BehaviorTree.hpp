//-------------------------------------------------------------
//! @file   BehaviorTree.hpp
//! @brief  ビヘイビアツリーのノードテンプレート群の宣言
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once
#include <Tsukino/Core/ECS/Registry/Registry.hpp>
#include <Tsukino/Core/ECS/Entity/Entity.hpp>

#include <functional>
#include <memory>
#include <vector>
// 名前空間 : ActionGame::ECS
namespace ActionGame::ECS {
    //-------------------------------------------------------------
    //! @enum   NodeStatus
    //! @brief  ノードのTick結果
    //-------------------------------------------------------------
    enum class NodeStatus {
        Success,    //!< 成功して完了した
        Failure,    //!< 失敗して完了した
        Running     //!< まだ完了していない（次フレームも継続する）
    };

    //-------------------------------------------------------------
    //! @struct BehaviorContext
    //! @brief  ノードのTickへ渡す実行コンテキスト
    //! @tparam TBlackboard ツリーが参照する黒板データの型
    //-------------------------------------------------------------
    template <typename TBlackboard>
    struct BehaviorContext {
        Tsukino::ECS::Registry& registry;      //!< ECSレジストリ
        Tsukino::ECS::Entity    entity;         //!< ツリーの所有者エンティティ
        TBlackboard&             blackboard;     //!< 条件判定・行動が読み書きする黒板データ
        float                    deltaTime;      //!< 前フレームからの経過時間（秒）
    };

    //-------------------------------------------------------------
    //! @class  BehaviorNode
    //! @brief  ビヘイビアツリーのノード基底クラス
    //! @tparam TBlackboard ツリーが参照する黒板データの型
    //-------------------------------------------------------------
    template <typename TBlackboard>
    class BehaviorNode {
    public:
        virtual ~BehaviorNode() = default;

        //-------------------------------------------------------------
        //! @brief  ノードを1回進行させる
        //! @param  context [in] 実行コンテキスト
        //! @return このノードの結果
        //-------------------------------------------------------------
        virtual NodeStatus Tick(BehaviorContext<TBlackboard>& context) = 0;
    };

    //-------------------------------------------------------------
    //! @class  CompositeNode
    //! @brief  Sequence/Selectorの共通基底。子の実行位置を記憶する（memory付き）。
    //! @details
    //! 子がRunningを返した場合、そのインデックスを保持して次のTickをそこから再開する。
    //! SuccessまたはFailureで完了した場合はインデックスを0へ戻す（外部からのReset呼び出しは不要）。
    //! Running中は自分より前段の条件を再評価しないため、途中終了の判定は
    //! 子（Action）自身がTick内で行う責務を持つ。
    //! @tparam TBlackboard ツリーが参照する黒板データの型
    //-------------------------------------------------------------
    template <typename TBlackboard>
    class CompositeNode : public BehaviorNode<TBlackboard> {
    public:
        //-------------------------------------------------------------
        //! @brief  子ノードを末尾へ追加する
        //! @param  child [in] 追加する子ノード
        //-------------------------------------------------------------
        void AddChild(std::shared_ptr<BehaviorNode<TBlackboard>> child) {
            m_children.push_back(std::move(child));
        }

    protected:
        std::vector<std::shared_ptr<BehaviorNode<TBlackboard>>> m_children;             //!< 子ノード一覧
        size_t                                                    m_runningIndex = 0;    //!< Running中に再開する子のインデックス
    };

    //-------------------------------------------------------------
    //! @class  Sequence
    //! @brief  子を順番に実行し、すべてSuccessならSuccess。1つでもFailureならFailureで打ち切る
    //! @tparam TBlackboard ツリーが参照する黒板データの型
    //-------------------------------------------------------------
    template <typename TBlackboard>
    class Sequence : public CompositeNode<TBlackboard> {
    public:
        NodeStatus Tick(BehaviorContext<TBlackboard>& context) override {
            auto& children = this->m_children;
            for(size_t i = this->m_runningIndex; i < children.size(); ++i) {
                NodeStatus status = children[i]->Tick(context);
                if(status == NodeStatus::Running) {
                    this->m_runningIndex = i;
                    return NodeStatus::Running;
                }
                if(status == NodeStatus::Failure) {
                    this->m_runningIndex = 0;
                    return NodeStatus::Failure;
                }
                // Successなら次の子へ進む
            }
            this->m_runningIndex = 0;
            return NodeStatus::Success;
        }
    };

    //-------------------------------------------------------------
    //! @class  Selector
    //! @brief  子を順番に実行し、最初にSuccessまたはRunningを返した子で確定する。
    //!         すべてFailureならFailure
    //! @tparam TBlackboard ツリーが参照する黒板データの型
    //-------------------------------------------------------------
    template <typename TBlackboard>
    class Selector : public CompositeNode<TBlackboard> {
    public:
        NodeStatus Tick(BehaviorContext<TBlackboard>& context) override {
            auto& children = this->m_children;
            for(size_t i = this->m_runningIndex; i < children.size(); ++i) {
                NodeStatus status = children[i]->Tick(context);
                if(status == NodeStatus::Running) {
                    this->m_runningIndex = i;
                    return NodeStatus::Running;
                }
                if(status == NodeStatus::Success) {
                    this->m_runningIndex = 0;
                    return NodeStatus::Success;
                }
                // Failureなら次の子へ進む
            }
            this->m_runningIndex = 0;
            return NodeStatus::Failure;
        }
    };

    //-------------------------------------------------------------
    //! @class  ConditionNode
    //! @brief  述語関数の結果をSuccess/Failureへ変換するだけの葉ノード（Runningは返さない）
    //! @tparam TBlackboard ツリーが参照する黒板データの型
    //-------------------------------------------------------------
    template <typename TBlackboard>
    class ConditionNode : public BehaviorNode<TBlackboard> {
    public:
        using Predicate = std::function<bool(BehaviorContext<TBlackboard>&)>;

        explicit ConditionNode(Predicate predicate) : m_predicate(std::move(predicate)) {}

        NodeStatus Tick(BehaviorContext<TBlackboard>& context) override {
            return m_predicate(context) ? NodeStatus::Success : NodeStatus::Failure;
        }

    private:
        Predicate m_predicate;    //!< 判定関数
    };

    //-------------------------------------------------------------
    //! @class  ActionNode
    //! @brief  任意の処理を行い、その場でNodeStatusを返す葉ノード
    //! @tparam TBlackboard ツリーが参照する黒板データの型
    //-------------------------------------------------------------
    template <typename TBlackboard>
    class ActionNode : public BehaviorNode<TBlackboard> {
    public:
        using Action = std::function<NodeStatus(BehaviorContext<TBlackboard>&)>;

        explicit ActionNode(Action action) : m_action(std::move(action)) {}

        NodeStatus Tick(BehaviorContext<TBlackboard>& context) override {
            return m_action(context);
        }

    private:
        Action m_action;    //!< 実行関数
    };
}    // namespace ActionGame::ECS
