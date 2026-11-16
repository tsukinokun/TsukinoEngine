//--------------------------------------------------------------
//! @file   PipelineFactory.hpp
//! @brief  パイプラインファクトリークラスの宣言
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
#include <Tsukino/Renderer/DepthMode.hpp>
#include <Tsukino/Renderer/CullMode.hpp>

#include <Tsukino/Renderer/BlendMode.hpp>

#include <Tsukino/GraphicsCommon/Vertex/VertexFormat.hpp>

#include <Tsukino/Core/typedef.hpp>

#include <memory>
#include <unordered_map>
#include <tuple>

namespace Tsukino::Asset {
    class ShaderAsset;    // 前方宣言
}

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {
    class GraphicsContext;    // 前方宣言（描画デバイスの持ち主。Renderer の内部専用）
    struct PipelineState;     // 前方宣言（描画 API ごとの中身。Renderer の内部で定義）

    // パイプラインステートのキャッシュ用キー
    using PipelineKey = std::tuple<u64, u64, Tsukino::GraphicsCommon::VertexFormat, DepthMode, BlendMode, CullMode>;

    //--------------------------------------------------------------
    //! @struct PipelineHash
    //! @brief パイプラインステートのハッシュ関数
    //--------------------------------------------------------------
    struct PipelineHash {
        std::size_t operator()(const PipelineKey& key) const noexcept {
            std::size_t seed = 0;
            const auto  combine = [&seed](std::size_t value) {
                // 要素ごとに混ぜてからずらして足す（XOR を並べるだけでは、似た値の組がぶつかりやすい）
                seed ^= value + 0x9e3779b97f4a7c15ull + (seed << 6) + (seed >> 2);
            };
            combine(std::hash<Tsukino::u64>()(std::get<0>(key)));
            combine(std::hash<Tsukino::u64>()(std::get<1>(key)));
            combine(static_cast<std::size_t>(std::get<2>(key)));
            combine(static_cast<std::size_t>(std::get<3>(key)));
            combine(static_cast<std::size_t>(std::get<4>(key)));
            combine(static_cast<std::size_t>(std::get<5>(key)));
            return seed;
        }
    };

    //--------------------------------------------------------------
    //! @class PipelineFactory
    //! @brief パイプラインファクトリークラス
    //--------------------------------------------------------------
    class PipelineFactory {
    public:
        //--------------------------------------------------------------
        //! @brief 引数付きコンストラクタ
        //! @param graphicsContext [in] パイプラインを作る描画デバイスの持ち主（借りる）
        //--------------------------------------------------------------
        explicit PipelineFactory(const GraphicsContext& graphicsContext)
            : m_graphicsContext(&graphicsContext) {}

        //--------------------------------------------------------------
        // パイプラインステートを作成する関数
        //! @param  vs           [in] 頂点シェーダーアセット
        //! @param  ps           [in] ピクセルシェーダーアセット
        //! @param  format       [in] 頂点の形式
        //! @param  depthMode    [in] デプスステンシルステートの設定
        //! @param  blendMode    [in] ブレンドステートの設定
        //! @param  cullMode     [in] 面カリングの設定（既定は両面描画）
        //! @return パイプラインステートのポインタ
        //--------------------------------------------------------------
        [[nodiscard]]
        std::shared_ptr<PipelineState> Create(const Tsukino::Asset::ShaderAsset&    vs,
                                              const Tsukino::Asset::ShaderAsset&    ps,
                                              Tsukino::GraphicsCommon::VertexFormat format,
                                              DepthMode                             depthMode,
                                              BlendMode                             blendMode = BlendMode::Opaque,
                                              CullMode                              cullMode  = CullMode::None);

    private:
        const GraphicsContext* m_graphicsContext = nullptr;    // パイプラインを作る描画デバイスの持ち主（借りている）

        // キャッシュ（キー: シェーダー2つと頂点の形式とステート、値: PipelineState）
        std::unordered_map<PipelineKey, std::shared_ptr<PipelineState>, PipelineHash> m_cache;
    };

}    // namespace Tsukino::Renderer
