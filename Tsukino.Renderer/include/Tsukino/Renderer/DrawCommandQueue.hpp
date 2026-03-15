//--------------------------------------------------------------
//! @file   DrawCommandQueue.hpp
//! @brief  描画コマンドキューの宣言
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
#include <vector>
#include <Tsukino/Renderer/DrawCommand.hpp>
// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {
    //--------------------------------------------------------------
    //! @class DrawCommandQueue
    //! @brief  描画コマンドキュークラス
    //--------------------------------------------------------------
    class DrawCommandQueue {
    public:
        //----------------------------------------------------------
        //! @brief コマンドを追加
        //! @param cmd [in] 追加する描画コマンド
        //----------------------------------------------------------
        void Push(const DrawCommand& cmd) { m_commands.push_back(cmd); }

        //----------------------------------------------------------
        //! @brief コマンド一覧を取得
        //! @return コマンドのリスト
        //----------------------------------------------------------
        [[nodiscard]]
        const std::vector<DrawCommand>& GetCommands() const {
            return m_commands;
        }

        //----------------------------------------------------------
        //! @brief コマンドをクリア
        //----------------------------------------------------------
        void Clear() { m_commands.clear(); }

        //----------------------------------------------------------
        //! @brief  コマンド数
        //! @return コマンドの数
        //----------------------------------------------------------
        size_t Size() const { return m_commands.size(); }

    private:
        std::vector<DrawCommand> m_commands;    // 描画コマンドの格納場所
    };

}    // namespace Tsukino::Renderer
