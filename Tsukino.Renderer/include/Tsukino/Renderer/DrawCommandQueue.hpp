//--------------------------------------------------------------
//! @file   DrawCommandQueue.hpp
//! @brief  描画コマンドキューの宣言
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
#include <deque>
#include <vector>

#include <Tsukino/Renderer/ConstantBuffer.hpp>
#include <Tsukino/Renderer/DX11/Material.hpp>
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
        // このフレームで使うマテリアル実体を1つ確保する
        //! @return 確保したマテリアルへの参照
        //! @note   DrawCommand::material が指す実体はここから取ること。
        //!         コマンドと同じオブジェクトが所有するので、Clear() が
        //!         呼ばれるまでポインタは必ず有効
        //----------------------------------------------------------
        [[nodiscard]]
        Material& AllocMaterial() { return m_materialArena.emplace_back(); }

        //----------------------------------------------------------
        // このフレームで使うマテリアル定数データを1つ確保する
        //! @return 確保した定数データへの参照
        //! @note   DrawCommand::materialData が指す実体はここから取ること
        //----------------------------------------------------------
        [[nodiscard]]
        CBufferMaterial& AllocMaterialData() { return m_materialDataArena.emplace_back(); }

        //----------------------------------------------------------
        //! @brief コマンド一覧を取得
        //! @return コマンドのリスト
        //----------------------------------------------------------
        [[nodiscard]]
        const std::vector<DrawCommand>& GetCommands() const {
            return m_commands;
        }

        //----------------------------------------------------------
        // コマンドと、コマンドが指すマテリアル実体をまとめて破棄する
        //! @note   3つを必ず同時に捨てること。片方だけ捨てると
        //!         DrawCommand の生ポインタがダングリングになる
        //----------------------------------------------------------
        void Clear() {
            m_commands.clear();
            m_materialArena.clear();
            m_materialDataArena.clear();
        }

        //----------------------------------------------------------
        //! @brief  コマンド数
        //! @return コマンドの数
        //----------------------------------------------------------
        size_t Size() const { return m_commands.size(); }

    private:
        std::vector<DrawCommand> m_commands;    // 描画コマンドの格納場所

        //----------------------------------------------------------
        // DrawCommand の material / materialData が指す実体の置き場。
        //
        // 以前は各 System が同じ deque を持っており、System 側の Update 冒頭で
        // clear するのに対し、コマンド側の破棄は Renderer::Render() の中でしか
        // 起きなかった。「毎フレーム必ず Update → Render の順で回る」という
        // 暗黙の契約に頼っており、Render を1フレーム飛ばすと次の Update 冒頭の
        // clear で即ダングリングになる状態だった。
        // コマンドと同じ場所へ移すことで、寿命の管理点を Clear() の1つに集約している。
        //
        // vector ではなく deque なのは、追加が続いても既存要素への参照が
        // 無効化されないため（コマンドは実体へのポインタを保持し続ける）
        //----------------------------------------------------------
        std::deque<Material>        m_materialArena;        // マテリアル実体
        std::deque<CBufferMaterial> m_materialDataArena;    // マテリアル定数データ
    };

}    // namespace Tsukino::Renderer
