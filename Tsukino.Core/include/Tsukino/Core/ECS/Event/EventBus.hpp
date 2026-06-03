//-------------------------------------------------------------
//! @file   EventBus.hpp
//! @brief  型安全な即時発火イベントバス
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once
#include <Tsukino/Core/ECS/Event/ScopedConnection.hpp>

#include <any>
#include <cassert>
#include <cstdint>
#include <functional>
#include <typeindex>
#include <unordered_map>
#include <vector>
// 名前空間 : Tsukino::ECS
namespace Tsukino::ECS {

    //-------------------------------------------------------------
    //! @class  EventBus
    //! @brief  型安全な即時発火イベントバス
    //! @details
    //! - イベントは何も継承しない POD struct で定義する
    //! - std::type_index で型を識別するため基底クラスが不要
    //! - Subscribe の戻り値（ScopedConnection）をメンバで持つことで
    //!   ライフタイムを管理する
    //! - Publish は即時同期発火。ハンドラはその場で全て呼ばれる
    //!
    //! @attention
    //! - ハンドラ内で View の対象コンポーネントを追加・削除すると
    //!   EnTT のイテレータが壊れる。その場合は破棄リストに積んで
    //!   ループ後に処理すること。
    //! - 同一イベント型の再入 Publish は assert で検知する
    //-------------------------------------------------------------
    class EventBus {
    public:
        // ハンドラ識別用のID。単調増加させるだけの整数で十分
        using HandlerId = std::uint64_t;

        //-------------------------------------------------------------
        //! @brief  デフォルトコンストラクタ
        //-------------------------------------------------------------
        EventBus() = default;

        //-------------------------------------------------------------
        //! @brief  デストラクタ
        //-------------------------------------------------------------
        ~EventBus() {
            m_isDestroying = true;    // 解体
        }

        //-------------------------------------------------------------
        // Scene が所有するためコピー・ムーブ禁止
        //-------------------------------------------------------------
        EventBus(const EventBus&)            = delete;
        EventBus& operator=(const EventBus&) = delete;
        EventBus(EventBus&&)                 = delete;
        EventBus& operator=(EventBus&&)      = delete;

        //-------------------------------------------------------------
        //! @brief      イベントの購読
        //! @tparam     TEvent  購読するイベントの型（POD struct）
        //! @param      handler 発火時に呼ばれるコールバック
        //! @return     ScopedConnection を必ずメンバ変数で受け取ること。
        //! @details    戻り値を捨てると即座に Unsubscribe される。
        //! コールバックの実体は EventBus 内部に保存される。
        //! 解除権（ScopedConnection）は呼び出し側が持つ。
        //-------------------------------------------------------------
        template <typename TEvent>
        [[nodiscard]]
        ScopedConnection Subscribe(std::function<void(const TEvent&)> handler) {
            const auto typeId = std::type_index(typeid(TEvent));
            const auto id     = m_nextId++;

            //-------------------------------------------------------------
            // 型消去して内部リストに保存する
            // std::any でラップすることで EventBus 自体は
            // TEvent の型を知らなくてよい
            //-------------------------------------------------------------
            m_handlers[typeId].push_back(HandlerEntry{id, [h = std::move(handler)](const std::any& e) { h(std::any_cast<const TEvent&>(e)); }});

            //-------------------------------------------------------------
            // typeId と id をキャプチャした解除クロージャを
            // ScopedConnection に渡す
            //-------------------------------------------------------------
            return ScopedConnection([this, typeId, id] { Unsubscribe(typeId, id); });
        }

        //-------------------------------------------------------------
        //! @brief  イベントの発火（即時・同期）
        //! @tparam TEvent 発火するイベントの型
        //! @param  event  発火するイベントのインスタンス
        //! @details
        //! 購読者が存在しない場合は何もしない。
        //! 発火中に同一型を再入 Publish しようとすると assert する。
        //-------------------------------------------------------------
        template <typename TEvent>
        void Publish(const TEvent& event) {
            //-------------------------------------------------------------
            // 発火するイベントの型を識別するための type_index を作る
            //-------------------------------------------------------------
            const auto typeId = std::type_index(typeid(TEvent));

            //-------------------------------------------------------------
            // 同一型の再入 Publish は循環依存のサインなので assert で検知
            //-------------------------------------------------------------
            assert(m_dispatchingType != typeId && "EventBus: 同一イベント型の再入 Publish を検知しました");

            auto it = m_handlers.find(typeId);
            if(it == m_handlers.end())
                return;

            //-------------------------------------------------------------
            // 発火中に Unsubscribe されても壊れないよう
            // ハンドラリストのスナップショットをとってからイテレートする
            //-------------------------------------------------------------
            m_dispatchingType   = typeId;
            const auto snapshot = it->second;
            for(const auto& entry : snapshot) {
                entry.invoke(std::any(event));
            }
            m_dispatchingType = std::type_index(typeid(void));
        }

        //-------------------------------------------------------------
        //! @brief  特定イベント型のハンドラを全解除
        //! @tparam TEvent 解除するイベントの型
        //-------------------------------------------------------------
        template <typename TEvent>
        void Clear() {
            m_handlers.erase(std::type_index(typeid(TEvent)));
        }

        //! @brief  全ハンドラを解除
        void ClearAll() { m_handlers.clear(); }

    private:
        //-------------------------------------------------------------
        //! @brief  型消去済みハンドラのエントリ
        //-------------------------------------------------------------
        struct HandlerEntry {
            HandlerId                            id;
            std::function<void(const std::any&)> invoke;
        };

        //-------------------------------------------------------------
        //! @brief  ID を指定してハンドラを1件削除する
        //! @note   ScopedConnection のデストラクタから呼ばれる
        //-------------------------------------------------------------
        void Unsubscribe(std::type_index typeId, HandlerId id) {
            if(m_isDestroying)
                return;    // 破壊済みの場合は何もしない

            auto it = m_handlers.find(typeId);
            if(it == m_handlers.end())
                return;

            auto& list = it->second;
            list.erase(std::remove_if(list.begin(), list.end(), [id](const HandlerEntry& e) { return e.id == id; }), list.end());

            // ハンドラが0件になったエントリはマップから消す
            if(list.empty())
                m_handlers.erase(it);
        }

        // typeId → ハンドラリスト のマップ
        std::unordered_map<std::type_index, std::vector<HandlerEntry>> m_handlers;

        // 次に払い出す購読ID（単調増加）
        HandlerId m_nextId = 0;

        // 現在発火中のイベント型（再入検知用、発火中以外は typeid(void)）
        std::type_index m_dispatchingType{typeid(void)};

        // 解体中かのフラグ、メモリ破壊を防ぐ
        bool m_isDestroying = false;
    };

}    // namespace Tsukino::ECS
