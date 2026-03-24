//--------------------------------------------------------------
//! @file   PipelineFactory.hpp
//! @brief  パイプラインファクトリークラスの宣言
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
#include <Tsukino/Renderer/DX11/PipelineState.hpp>

#include <Tsukino/Core/typedef.hpp>

#include <memory>
#include <unordered_map>

namespace Tsukino::Asset {
    class ShaderAsset;    // 前方宣言
}

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {
    //--------------------------------------------------------------
    //! @struct PipelineHash
    //! @brief パイプラインステートのハッシュ関数
    //--------------------------------------------------------------
    struct PipelineHash {
        //--------------------------------------------------------------
        //! @brief ハッシュ関数
        //--------------------------------------------------------------
        std::size_t operator()(const std::pair<u64, u64>& p) const { return std::hash<u64>()(p.first) ^ (std::hash<u64>()(p.second) << 1); }
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
        //! @return パイプラインステートのポインタ
        //--------------------------------------------------------------
        [[nodiscard]]
        std::shared_ptr<PipelineState> Create(const Tsukino::Asset::ShaderAsset& vs,
                                              const Tsukino::Asset::ShaderAsset& ps,
                                              const D3D11_INPUT_ELEMENT_DESC*    layout,
                                              UINT                               layoutCount);

    private:
        // DirectXのデバイス
        ID3D11Device* m_device = nullptr;    

        // キャッシュ用コンテナ (Key: <VSハンドル, PSハンドル>, Value: PipelineState)
        std::unordered_map<std::pair<u64, u64>, std::shared_ptr<PipelineState>, PipelineHash> m_cache;
    };

}    // namespace Tsukino::Renderer
