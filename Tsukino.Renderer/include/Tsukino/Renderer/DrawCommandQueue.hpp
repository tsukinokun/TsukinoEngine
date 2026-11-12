//--------------------------------------------------------------
//! @file   DrawCommandQueue.hpp
//! @brief  描画コマンドキューの宣言
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
#include <Tsukino/Renderer/ConstantBuffer.hpp>
#include <Tsukino/Renderer/Material.hpp>
#include <Tsukino/Renderer/DrawCommand.hpp>

#include <Tsukino/GraphicsCommon/Vertex/DebugVertex.hpp>

#include <deque>
#include <span>
#include <vector>
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
        // スプライトの並びを描くコマンドを追加する
        //! @param cmd     [in] パス・sortOrder・切り取りなどを設定したコマンド（kind と batch* はここで上書きする）
        //! @param sprites [in] 描くスプライト。並べた順に描かれる（キューへ写すので、呼んだ後に捨ててよい）
        //! @note   sprites が空なら何も積まない
        //----------------------------------------------------------
        void PushSprites(const DrawCommand& cmd, std::span<const SpriteQuad> sprites) {
            if(sprites.empty())
                return;

            DrawCommand& pushed = m_commands.emplace_back(cmd);
            pushed.kind         = DrawKind::Sprites;
            pushed.batchFirst   = static_cast<u32>(m_sprites.size());
            pushed.batchCount   = static_cast<u32>(sprites.size());
            m_sprites.insert(m_sprites.end(), sprites.begin(), sprites.end());
        }

        //----------------------------------------------------------
        // 線の頂点の列を描くコマンドを追加する
        //! @param cmd      [in] パスなどを設定したコマンド（kind と batch* はここで上書きする）
        //! @param vertices [in] 線の頂点。2頂点で1本（キューへ写すので、呼んだ後に捨ててよい）
        //! @note   深度テストあり・深度書き込みあり・不透明で描く。端数の1頂点は捨てる。
        //!         頂点が無ければ何も積まない
        //----------------------------------------------------------
        void PushLines(const DrawCommand& cmd, std::span<const Tsukino::GraphicsCommon::DebugVertex> vertices) {
            const size_t count = vertices.size() & ~size_t(1);
            if(count == 0)
                return;

            DrawCommand& pushed = m_commands.emplace_back(cmd);
            pushed.kind         = DrawKind::Lines;
            pushed.batchFirst   = static_cast<u32>(m_lineVertices.size());
            pushed.batchCount   = static_cast<u32>(count);
            m_lineVertices.insert(m_lineVertices.end(), vertices.begin(), vertices.begin() + count);
        }

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
        // このフレームで使うシーン定数（カメラの差し替え用）を1つ確保する
        //! @return 確保したシーン定数への参照
        //! @note   DrawCommand::cameraOverride が指す実体はここから取ること
        //----------------------------------------------------------
        [[nodiscard]]
        CBufferScene& AllocSceneData() { return m_sceneDataArena.emplace_back(); }

        //----------------------------------------------------------
        //! @brief コマンド一覧を取得
        //! @return コマンドのリスト
        //----------------------------------------------------------
        [[nodiscard]]
        const std::vector<DrawCommand>& GetCommands() const {
            return m_commands;
        }

        //----------------------------------------------------------
        //! @brief  PushSprites で積んだスプライトの並びを取得
        //! @return このフレームのスプライト（DrawCommand::batchFirst / batchCount で区切って使う）
        //----------------------------------------------------------
        [[nodiscard]]
        const std::vector<SpriteQuad>& GetSprites() const {
            return m_sprites;
        }

        //----------------------------------------------------------
        //! @brief  PushLines で積んだ線の頂点の列を取得
        //! @return このフレームの線の頂点（DrawCommand::batchFirst / batchCount で区切って使う）
        //----------------------------------------------------------
        [[nodiscard]]
        const std::vector<Tsukino::GraphicsCommon::DebugVertex>& GetLineVertices() const {
            return m_lineVertices;
        }

        //----------------------------------------------------------
        // コマンドと、コマンドが指すマテリアル実体・シーン定数をまとめて破棄する
        //! @note   すべてを必ず同時に捨てること。片方だけ捨てると
        //!         DrawCommand の生ポインタがダングリングになる
        //----------------------------------------------------------
        void Clear() {
            m_commands.clear();
            m_materialArena.clear();
            m_materialDataArena.clear();
            m_sceneDataArena.clear();
            m_sprites.clear();
            m_lineVertices.clear();
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
        std::deque<CBufferScene>    m_sceneDataArena;       // カメラを差し替えるシーン定数

        //----------------------------------------------------------
        // Sprites / Lines のコマンドが描く中身。コマンドは添字の区間で指すので、
        // 追加で並びが伸びて作り直されても区間は変わらない
        //----------------------------------------------------------
        std::vector<SpriteQuad>                           m_sprites;         // スプライト
        std::vector<Tsukino::GraphicsCommon::DebugVertex> m_lineVertices;    // 線の頂点
    };

}    // namespace Tsukino::Renderer
