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
    //! @note   テクスチャは MaterialTextureSlot の5枠（Albedo/Normal/MetallicRoughness/Emissive/AO）を
    //!         保持できる。未設定のスロットは空のまま SetMaterial が
    //!         バインドするが、空のスロットはサンプル結果が0になり法線マップ等が破綻するため、
    //!         デフォルトテクスチャへの差し替えは積む側（ModelSystem）の責務とする。
    //--------------------------------------------------------------
    class Material {
    public:
        //! @brief マテリアルテクスチャの枠の数
        static constexpr size_t TextureSlotCount = static_cast<size_t>(MaterialTextureSlot::Count);

        //--------------------------------------------------------------
        //! @brief  パイプラインステートを設定する関数
        //! @param  p [in] パイプラインステートへのポインタ
        //--------------------------------------------------------------
        void SetPipeline(PipelineState* p) { m_pipeline = p; }

        //--------------------------------------------------------------
        //! @brief  アルベドのテクスチャを設定する関数（簡易版）
        //! @param  texture [in] テクスチャのハンドル
        //--------------------------------------------------------------
        void SetTexture(TextureHandle texture) { SetTexture(MaterialTextureSlot::Albedo, texture); }

        //--------------------------------------------------------------
        //! @brief  指定スロットにテクスチャを設定する関数
        //! @param  slot    [in] マテリアルテクスチャの枠
        //! @param  texture [in] テクスチャのハンドル
        //--------------------------------------------------------------
        void SetTexture(MaterialTextureSlot slot, TextureHandle texture) {
            const size_t index = static_cast<size_t>(slot);
            if(index < TextureSlotCount) {
                m_textures[index] = texture;
            }
        }

        //--------------------------------------------------------------
        //! @brief  サンプラーを設定する関数
        //! @param  type [in] サンプラーの種類
        //--------------------------------------------------------------
        void SetSampler(Tsukino::GraphicsCommon::SamplerType type) { m_sampler = type; }

        //--------------------------------------------------------------
        //! @brief  パイプラインステートを取得する関数
        //! @return パイプラインステートへのポインタ
        //--------------------------------------------------------------
        [[nodiscard]]
        PipelineState* GetPipeline() const {
            return m_pipeline;
        }

        //--------------------------------------------------------------
        //! @brief  アルベドのテクスチャを取得する関数
        //! @return テクスチャのハンドル（未設定なら無効なハンドル）
        //--------------------------------------------------------------
        [[nodiscard]]
        TextureHandle GetTexture() const {
            return m_textures[static_cast<size_t>(MaterialTextureSlot::Albedo)];
        }

        //--------------------------------------------------------------
        //! @brief  全マテリアルテクスチャを取得する関数（MaterialTextureSlot の順）
        //! @return テクスチャのハンドルの配列（未設定は無効なハンドル）
        //--------------------------------------------------------------
        [[nodiscard]]
        const std::array<TextureHandle, TextureSlotCount>& GetTextures() const {
            return m_textures;
        }

        //--------------------------------------------------------------
        //! @brief  サンプラーを取得する関数
        //! @return サンプラーの種類（未設定なら空。描画 API の既定のサンプラーになる）
        //--------------------------------------------------------------
        [[nodiscard]]
        std::optional<Tsukino::GraphicsCommon::SamplerType> GetSampler() const {
            return m_sampler;
        }

    private:
        PipelineState*                                      m_pipeline = nullptr;    // パイプラインステートへのポインタ
        std::array<TextureHandle, TextureSlotCount>         m_textures{};            // マテリアルテクスチャ（未設定は無効なハンドル）
        std::optional<Tsukino::GraphicsCommon::SamplerType> m_sampler;               // サンプラーの種類（未設定は空）
    };
}    // namespace Tsukino::Renderer
