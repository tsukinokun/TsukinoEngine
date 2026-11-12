//--------------------------------------------------------------
//! @file   Material.hpp
//! @brief  マテリアルクラスの宣言
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
#include <Tsukino/Renderer/GpuHandles.hpp>
#include <Tsukino/Renderer/ShaderSlots.hpp>

#include <Tsukino/GraphicsCommon/State/SamplerType.hpp>

#include <array>
#include <optional>
namespace Tsukino::Renderer {
    struct PipelineState;    // 前方宣言（描画 API ごとの中身。Renderer の内部で定義）

    //--------------------------------------------------------------
    //! @class  Material
    //! @brief  マテリアルクラス
    //! @note   テクスチャは t0〜t4（Albedo/Normal/MetallicRoughness/Emissive/AO）の
    //!         5枚まで保持できる。未設定のスロットは空のまま SetMaterial が
    //!         バインドするが、空のスロットはサンプル結果が0になり法線マップ等が破綻するため、
    //!         デフォルトテクスチャへの差し替えは積む側（ModelSystem）の責務とする。
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
        //! @param  texture [in] テクスチャのハンドル
        //--------------------------------------------------------------
        void SetTexture(TextureHandle texture) { SetTexture(SRVSlot::Albedo, texture); }

        //--------------------------------------------------------------
        //! @brief  指定スロットにテクスチャを設定する関数
        //! @param  slot    [in] マテリアルテクスチャスロット (t0〜t4)
        //! @param  texture [in] テクスチャのハンドル
        //--------------------------------------------------------------
        void SetTexture(SRVSlot slot, TextureHandle texture) {
            size_t index = static_cast<size_t>(slot);
            if(index < TextureSlotCount) {
                textures[index] = texture;
            }
        }

        //--------------------------------------------------------------
        //! @brief  サンプラーを設定する関数
        //! @param  type [in] サンプラーの種類
        //--------------------------------------------------------------
        void SetSampler(Tsukino::GraphicsCommon::SamplerType type) { sampler = type; }

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
        //! @return テクスチャのハンドル（未設定なら無効なハンドル）
        //--------------------------------------------------------------
        [[nodiscard]]
        TextureHandle GetTexture() const {
            return textures[static_cast<size_t>(SRVSlot::Albedo)];
        }

        //--------------------------------------------------------------
        //! @brief  全マテリアルテクスチャ（t0〜t4）を取得する関数
        //! @return テクスチャのハンドルの配列（未設定は無効なハンドル）
        //--------------------------------------------------------------
        [[nodiscard]]
        const std::array<TextureHandle, TextureSlotCount>& GetTextures() const {
            return textures;
        }

        //--------------------------------------------------------------
        //! @brief  サンプラーを取得する関数
        //! @return サンプラーの種類（未設定なら空。描画 API の既定のサンプラーになる）
        //--------------------------------------------------------------
        [[nodiscard]]
        std::optional<Tsukino::GraphicsCommon::SamplerType> GetSampler() const {
            return sampler;
        }

    private:
        PipelineState*                                      pipeline = nullptr;    // パイプラインステートへのポインタ
        std::array<TextureHandle, TextureSlotCount>         textures{};            // t0〜t4のテクスチャ（未設定は無効なハンドル）
        std::optional<Tsukino::GraphicsCommon::SamplerType> sampler;               // サンプラーの種類（未設定は空）
    };
}    // namespace Tsukino::Renderer
