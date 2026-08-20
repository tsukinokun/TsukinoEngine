//--------------------------------------------------------------
//! @file   Material.hpp
//! @brief  マテリアルクラスの宣言
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
#include <Tsukino/Renderer/DX11/PipelineState.hpp>
#include <Tsukino/Renderer/ShaderSlots.hpp>
#include <array>
namespace Tsukino::Renderer {
    //--------------------------------------------------------------
    //! @class  Material
    //! @brief  マテリアルクラス
    //! @note   テクスチャは t0〜t4（Albedo/Normal/MetallicRoughness/Emissive/AO）の
    //!         5枚まで保持できる。未設定のスロットは nullptr のまま返し、
    //!         GraphicsContext::SetMaterial 側でデフォルトテクスチャに差し替える。
    //--------------------------------------------------------------
    class Material {
    public:
        //! @brief マテリアルテクスチャのスロット数（t0〜t4）
        static constexpr size_t TextureSlotCount = 5;

        //--------------------------------------------------------------
        //! @brief  パイプラインステートを設定する関数
        //! @param  p [in] パイプラインステートへのポインタ
        //--------------------------------------------------------------
        void SetPipeline(PipelineState* p) { pipeline = p; }

        //--------------------------------------------------------------
        //! @brief  テクスチャを設定する関数（アルベド = t0 固定の簡易版）
        //! @param  srv [in] テクスチャへのポインタ
        //--------------------------------------------------------------
        void SetTexture(ID3D11ShaderResourceView* srv) { SetTexture(SRVSlot::Albedo, srv); }

        //--------------------------------------------------------------
        //! @brief  指定スロットにテクスチャを設定する関数
        //! @param  slot [in] マテリアルテクスチャスロット (t0〜t4)
        //! @param  srv  [in] テクスチャへのポインタ
        //--------------------------------------------------------------
        void SetTexture(SRVSlot slot, ID3D11ShaderResourceView* srv) {
            size_t index = static_cast<size_t>(slot);
            if(index < TextureSlotCount) {
                textures[index] = srv;
            }
        }

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
        //! @brief  アルベド(t0)テクスチャを取得する関数
        //! @return テクスチャへのポインタ
        //--------------------------------------------------------------
        [[nodiscard]]
        ID3D11ShaderResourceView* GetTexture() const {
            return textures[static_cast<size_t>(SRVSlot::Albedo)];
        }

        //--------------------------------------------------------------
        //! @brief  全マテリアルテクスチャ（t0〜t4）の配列を取得する関数
        //! @return ID3D11ShaderResourceView* の配列（先頭ポインタ）
        //--------------------------------------------------------------
        [[nodiscard]]
        const ID3D11ShaderResourceView* const* GetTextures() const {
            return textures.data();
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
        PipelineState*                                   pipeline = nullptr;    // パイプラインステートへのポインタ
        std::array<ID3D11ShaderResourceView*, TextureSlotCount> textures{};      // t0〜t4のテクスチャ（未設定はnullptr）
        ID3D11SamplerState*                              sampler  = nullptr;    // サンプラーへのポインタ
    };
}    // namespace Tsukino::Renderer
