//--------------------------------------------------------------
//! @file   PipelineFactory.hpp
//! @brief  パイプラインファクトリークラスの宣言
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
#include <Tsukino/Renderer/DX11/PipelineState.hpp>
#include <Tsukino/Renderer/DX11/DepthMode.hpp>

#include <Tsukino/Core/typedef.hpp>

#include <memory>
#include <unordered_map>

namespace Tsukino::Asset {
    class ShaderAsset;    // 前方宣言
}

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {
    // パイプラインステートのキャッシュ用キー 
    using PipelineKey = std::tuple<u64, u64, DepthMode>;
    //--------------------------------------------------------------
    //! @struct PipelineHash
    //! @brief パイプラインステートのハッシュ関数
    //--------------------------------------------------------------
    struct PipelineHash {
        //--------------------------------------------------------------
        //! @brief ハッシュ関数
        //--------------------------------------------------------------
        std::size_t operator()(const PipelineKey& key) const {
            auto h1 = std::hash<u64>()(std::get<0>(key));
            auto h2 = std::hash<u64>()(std::get<1>(key));
            auto h3 = std::hash<int>()(static_cast<int>(std::get<2>(key)));
            return h1 ^ (h2 << 1) ^ (h3 << 2);
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
        //! @param device [in] DirectXのデバイス
        //--------------------------------------------------------------
        PipelineFactory(ID3D11Device* device)
            : m_device(device) {}

        //--------------------------------------------------------------
        // パイプラインステートを作成する関数
        //! @param  vsAsset     [in] 頂点シェーダーアセット
        //! @param  psAsset     [in] ピクセルシェーダーアセット
        //! @param  layout      [in] 入力レイアウトの配列
        //! @param  layoutCount [in] 入力レイアウトの数
        //! @param  depthMode    [in] デプスステンシルステートの設定
        //! @return パイプラインステートのポインタ
        //--------------------------------------------------------------
        [[nodiscard]]
        std::shared_ptr<PipelineState> Create(const Tsukino::Asset::ShaderAsset& vs,
                                              const Tsukino::Asset::ShaderAsset& ps,
                                              const D3D11_INPUT_ELEMENT_DESC*    layout,
                                              UINT                               layoutCount,
                                              DepthMode                          depthMode);

    private:
        // DirectXのデバイス
        ID3D11Device* m_device = nullptr;

        // キャッシュ用コンテナ (Key: <VSハンドル, PSハンドル>, Value: PipelineState)
        std::unordered_map<PipelineKey, std::shared_ptr<PipelineState>, PipelineHash> m_cache;
    };

}    // namespace Tsukino::Renderer
