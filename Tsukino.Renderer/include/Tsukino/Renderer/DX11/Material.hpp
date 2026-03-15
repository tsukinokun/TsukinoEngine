//--------------------------------------------------------------
//! @file   Material.hpp
//! @brief  マテリアルクラスの宣言
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
#include <Tsukino/Renderer/DX11/PipelineState.hpp>
namespace Tsukino::Renderer {
    //--------------------------------------------------------------
    //! @class  Material
    //! @brief  マテリアルクラス
    //--------------------------------------------------------------
    class Material {
    public:
        //--------------------------------------------------------------
        //! @brief  パイプラインステートを設定する関数
        //! @param  p [in] パイプラインステートへのポインタ
        //--------------------------------------------------------------
        void SetPipeline(PipelineState* p) { pipeline = p; }

        //--------------------------------------------------------------
        //! @brief  テクスチャを設定する関数
        //! @param  srv [in] テクスチャへのポインタ
        //--------------------------------------------------------------
        void SetTexture(ID3D11ShaderResourceView* srv) { texture = srv; }

        //--------------------------------------------------------------
        //! @brief  サンプラーを設定する関数
        //! @param  s [in] サンプラーへのポインタ
        //--------------------------------------------------------------
        void SetSampler(ID3D11SamplerState* s) { sampler = s; }

        //--------------------------------------------------------------
        //! @brief  パイプラインステートを取得する関数
        //! @return パイプラインステートへのポインタ
        //--------------------------------------------------------------
        [[nodiscard]]
        PipelineState* GetPipeline() const {
            return pipeline;
        }

        //--------------------------------------------------------------
        //! @brief  テクスチャを取得する関数
        //! @return テクスチャへのポインタ
        //--------------------------------------------------------------
        [[nodiscard]]
        ID3D11ShaderResourceView* GetTexture() const {
            return texture;
        }

        //--------------------------------------------------------------
        //! @brief  サンプラーを取得する関数
        //! @return サンプラーへのポインタ
        //--------------------------------------------------------------
        [[nodiscard]]
        ID3D11SamplerState* GetSampler() const {
            return sampler;
        }

    private:
        PipelineState*            pipeline = nullptr;    // パイプラインステートへのポインタ
        ID3D11ShaderResourceView* texture  = nullptr;    // テクスチャへのポインタ
        ID3D11SamplerState*       sampler  = nullptr;    // サンプラーへのポインタ
    };
}    // namespace Tsukino::Renderer
