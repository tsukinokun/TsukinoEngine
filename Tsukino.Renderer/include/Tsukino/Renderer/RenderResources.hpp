//----------------------------------------------------------------------------
//! @file   RenderResources.hpp
//! @brief  描画で共有する資源の宣言
//! @detail どのパスからも、ゲーム側の System からも借りられる資源をまとめて持ちます。
//!         共通ステート・サンプラー・既定テクスチャ・テクスチャキャッシュ・
//!         プリミティブメッシュ・PipelineFactory と、SpriteFont / SpriteBatch の生成を担当します。
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/Core/WindowsLean.hpp>
#include <Tsukino/Core/typedef.hpp>

#include <Tsukino/Renderer/DX11/MeshBuffer.hpp>
#include <Tsukino/Renderer/DX11/PipelineFactory.hpp>
#include <Tsukino/Renderer/DX11/Texture/DX11Texture2D.hpp>

#include <Tsukino/GraphicsCommon/Mesh/PrimitiveType.hpp>
#include <Tsukino/GraphicsCommon/State/SamplerType.hpp>

#include <wrl/client.h>
#include <d3d11.h>
#include <SpriteFont.h>
#include <CommonStates.h>

#include <array>
#include <memory>
#include <optional>
#include <unordered_map>

namespace Tsukino::Asset {
    class TextureAsset;    // 前方宣言
}

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {
    //------------------------------------------------------------------------
    //! 描画で共有する資源を持つクラスです。
    //! @note  Renderer が1つだけ所有し、Renderer::GetResources() で借りる。
    //!        デバイスとデバイスコンテキストは GraphicsContext が所有しており、
    //!        ここでは借りているだけ（Renderer がこのクラスより長生きさせる）
    //------------------------------------------------------------------------
    class RenderResources {
    public:

        //! 資源を作成します。
        //! @param  [in] device  描画デバイス
        //! @param  [in] context デバイスコンテキスト（SpriteBatch の生成に使う）
        //! @return true: 作成成功, false: 作成失敗
        [[nodiscard]]
        bool Initialize(ID3D11Device* device, ID3D11DeviceContext* context);

        //! パイプラインの生成器を取得します。
        //! @return PipelineFactory へのポインタ
        [[nodiscard]]
        PipelineFactory* GetPipelineFactory() {
            return &m_pipelineFactory.value();
        }

        //! DirectXTK の共通ステートを取得します。
        //! @return 共通ステートへのポインタ
        [[nodiscard]]
        DirectX::CommonStates* GetCommonStatesTK() const {
            return m_commonStates.get();
        }

        //! サンプラーを取得します。
        //! @param  [in] type 取得するサンプラーの種類
        //! @return サンプラーステートへのポインタ
        [[nodiscard]]
        ID3D11SamplerState* GetSampler(Tsukino::GraphicsCommon::SamplerType type) const {
            return m_samplers[static_cast<size_t>(type)].Get();
        }

        //! プリミティブメッシュを取得します。
        //! @param  [in] type 取得するプリミティブの種類
        //! @return メッシュバッファへのポインタ
        [[nodiscard]]
        MeshBuffer* GetPrimitiveMesh(Tsukino::GraphicsCommon::PrimitiveType type) {
            return &m_primitiveMeshes[static_cast<size_t>(type)];
        }

        //! テクスチャの SRV を取得します（初回はGPUへ転送してキャッシュします）。
        //! @param  [in] textureAsset 取得元のテクスチャアセット
        //! @return ID3D11ShaderResourceView へのポインタ
        [[nodiscard]]
        ID3D11ShaderResourceView* GetTextureSRV(const Tsukino::Asset::TextureAsset& textureAsset);

        //! 白テクスチャの SRV を取得します。
        //! @return ID3D11ShaderResourceView へのポインタ
        //! @note  マテリアルテクスチャ未設定時の既定値。
        //!        アルベド/MR/エミッシブ/AOはいずれもcbuffer定数との「乗算」で
        //!        合成するため、白(=1.0)を掛ければ定数値がそのまま残る
        [[nodiscard]]
        ID3D11ShaderResourceView* GetWhiteTextureSRV() const {
            return m_whiteSRV.Get();
        }

        //! フラット法線テクスチャの SRV を取得します。
        //! @return ID3D11ShaderResourceView へのポインタ
        //! @note  ノーマルマップ未設定時の既定値。接空間の(0,0,1)を
        //!        エンコードした値(R=0x80,G=0x80,B=0xFF)で、これを適用しても
        //!        頂点法線がそのまま保たれる。白を使うと法線が斜めにずれる
        [[nodiscard]]
        ID3D11ShaderResourceView* GetFlatNormalTextureSRV() const {
            return m_flatNormalSRV.Get();
        }

        //! スプライトフォントを作成します。
        //! @param  [in] data フォントデータのバイナリ
        //! @param  [in] size フォントデータのサイズ
        //! @return 作成した SpriteFont
        [[nodiscard]]
        std::unique_ptr<DirectX::SpriteFont> CreateSpriteFont(const u8* data, size_t size) const;

        //! スプライトバッチを作成します。
        //! @return 作成した SpriteBatch
        [[nodiscard]]
        std::unique_ptr<DirectX::SpriteBatch> CreateSpriteBatch() const;

    private:

        //! サンプラーを作成します。
        //! @return true: 作成成功, false: 作成失敗
        [[nodiscard]]
        bool CreateSamplers();

        //! プリミティブメッシュを作成します。
        void CreatePrimitiveMeshes();

        //! マテリアル用の既定テクスチャ（白・フラット法線）を作成します。
        //! @return true: 作成成功, false: 作成失敗
        [[nodiscard]]
        bool CreateDefaultTextures();

        //! 1x1 のテクスチャを作成します。
        //! @param  [in]  rgba      ピクセル値。R8G8B8A8_UNORM はメモリ上のバイト順が
        //!                         R,G,B,A なので、リトルエンディアンでは 0xAABBGGRR と書く
        //!                         （例: フラット法線 R=0x80,G=0x80,B=0xFF,A=0xFF → 0xFFFF8080）
        //! @param  [out] outTex    作成したテクスチャ
        //! @param  [out] outSRV    作成した SRV
        //! @param  [in]  debugName 失敗時のログに出す名前
        //! @return true: 作成成功, false: 作成失敗
        [[nodiscard]]
        bool Create1x1Texture(u32                                               rgba,
                              Microsoft::WRL::ComPtr<ID3D11Texture2D>&          outTex,
                              Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& outSRV,
                              const char*                                       debugName);

    private:
        ID3D11Device*        m_device  = nullptr;    // 描画デバイス（GraphicsContext から借りている）
        ID3D11DeviceContext* m_context = nullptr;    // デバイスコンテキスト（同上）

        std::optional<PipelineFactory>         m_pipelineFactory;    // デバイスが決まってから構築するため optional
        std::unique_ptr<DirectX::CommonStates> m_commonStates;       // DirectXTK の共通ステート

        std::array<Microsoft::WRL::ComPtr<ID3D11SamplerState>, static_cast<size_t>(Tsukino::GraphicsCommon::SamplerType::Count)> m_samplers;
        std::array<MeshBuffer, static_cast<size_t>(Tsukino::GraphicsCommon::PrimitiveType::Count)>                              m_primitiveMeshes;

        Microsoft::WRL::ComPtr<ID3D11Texture2D>          m_whiteTex;         // 白テクスチャ本体
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_whiteSRV;         // 白テクスチャの SRV
        Microsoft::WRL::ComPtr<ID3D11Texture2D>          m_flatNormalTex;    // フラット法線テクスチャ本体
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_flatNormalSRV;    // フラット法線テクスチャの SRV

        std::unordered_map<u64, std::unique_ptr<DX11Texture2D>> m_textureCache;    // AssetHandle の値をキーにしたテクスチャのキャッシュ
    };
}    // namespace Tsukino::Renderer
