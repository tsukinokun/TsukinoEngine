//-------------------------------------------------------------
//! @file   ScopedConnection.hpp
//! @brief  イベント購読のRAIIスコープ管理
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once
#include <functional>
// 名前空間 : Tsukino::ECS
namespace Tsukino::ECS {

    //-------------------------------------------------------------
    //! @class  ScopedConnection
    //! @brief  購読トークンのRAIIラッパー
    //!
    //! EventBus::Subscribe() の戻り値として受け取り、
    //! メンバ変数として持つことでライフタイムを管理する。
    //!
    //! このオブジェクトが破棄されると、デストラクタが
    //! Unsubscribe を自動で呼ぶため、購読解除し忘れによる
    //! 破棄済みオブジェクトへのアクセスを防ぐ。
    //!
    //! @code
    //! class AudioSystem : public ISystem {
    //! public:
    //!     explicit AudioSystem(EventBus& bus) {
    //!         // Subscribe の戻り値を必ずメンバで受け取る
    //!         m_conn = bus.Subscribe<CollisionEvent>(
    //!             [this](const CollisionEvent& e) { ... });
    //!     }
    //!     // AudioSystem が破棄されると m_conn のデストラクタが走り
    //!     // 自動で Unsubscribe される
    //! private:
    //!     ScopedConnection m_conn;
    //! };
    //! @endcode
    //-------------------------------------------------------------
    class ScopedConnection {
    public:
        //-------------------------------------------------------------
        //! @brief  デフォルトコンストラクタ
        //-------------------------------------------------------------
        ScopedConnection() = default;

        //-------------------------------------------------------------
        //! @brief  購読解除関数を受け取るコンストラクタ
        //! @note   EventBus::Subscribe() が内部で呼ぶ。直接呼ばない。
        //-------------------------------------------------------------
        explicit ScopedConnection(std::function<void()> disconnector)
            : m_disconnector(std::move(disconnector)) {}

        //-------------------------------------------------------------
        // コピー禁止（解除権の所有者は常にひとつ）
        //-------------------------------------------------------------
        ScopedConnection(const ScopedConnection&)            = delete;
        ScopedConnection& operator=(const ScopedConnection&) = delete;

        //-------------------------------------------------------------
        // ムーブ許可（vector に格納したい場合などに使う）
        //-------------------------------------------------------------
        ScopedConnection(ScopedConnection&& other) noexcept
            : m_disconnector(std::move(other.m_disconnector)) {
            other.m_disconnector = nullptr;
        }

        //-------------------------------------------------------------
        //! @brief  ムーブ代入許可
        //! @param  other ムーブ元
        //! @return ScopedConnectionの参照
        //! @note   既に有効な購読がある場合は先に
        //-------------------------------------------------------------
        ScopedConnection& operator=(ScopedConnection&& other) noexcept {
            if(this != &other) {
                Disconnect();
                m_disconnector       = std::move(other.m_disconnector);
                other.m_disconnector = nullptr;
            }
            return *this;
        }

        //-------------------------------------------------------------
        //! @brief   デストラクタ
        //! @detail  デストラクタが呼ばれると自動で購読解除される
        //-------------------------------------------------------------
        ~ScopedConnection() { Disconnect(); }

        //-------------------------------------------------------------
        //! @brief  明示的に購読解除する
        //! @note   デストラクタより先に解除したい場合に呼ぶ
        //!         呼んだ後は IsConnected() が false になる
        //-------------------------------------------------------------
        void Disconnect() {
            if(m_disconnector) {
                m_disconnector();
                m_disconnector = nullptr;
            }
        }

        //-------------------------------------------------------------
        //! @brief  有効な購読かを確認する
        //! @return 有効な購読なら true。すでに解除されている場合は false。
        //! //-------------------------------------------------------------
        [[nodiscard]] bool IsConnected() const { return m_disconnector != nullptr; }

    private:
        //! Unsubscribe を呼ぶクロージャ
        //! EventBus::Subscribe() が typeId と id をキャプチャして生成する
        std::function<void()> m_disconnector;
    };

}    // namespace Tsukino::ECS
