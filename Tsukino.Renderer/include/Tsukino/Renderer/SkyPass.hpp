//----------------------------------------------------------------------------
//! @file   SkyPass.hpp
//! @brief  スカイ（大気散乱）パスの宣言
//! @detail 画面全体を覆う三角形へ大気散乱のスカイを描きます。GBuffer パスの前に、
//!         深度を書かずに最背面として描きます。IBL ベイクのキャプチャも同じシェーダーを使います。
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/Core/WindowsLean.hpp>

#include <Tsukino/Renderer/ConstantBuffer.hpp>

#include <wrl/client.h>
#include <d3d11.h>

namespace Tsukino::Asset {
    class ShaderAsset;    // 前方宣言
}

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {
    class GraphicsContext;    // 前方宣言
    class RenderResources;    // 前方宣言
    class FrameConstants;     // 前方宣言

    //------------------------------------------------------------------------
    //! スカイパスのクラスです。
    //! @note  Renderer が1つだけ所有し、Renderer::GetSky() で借りる。
    //!        パイプラインは SkyAtmosphereSystem が SetPipeline() で確立し、
    //!        確立されるまではスカイも IBL ベイクも走らない
    //------------------------------------------------------------------------
    class SkyPass {
    public:

        //! スカイの定数バッファ（b4）を作成します。
        //! @param  [in] graphicsContext デバイスとデバイスコンテキストの取得元
        //! @param  [in] resources       共通ステートの取得元
        //! @param  [in] frameConstants  b0 の取得元
        //! @return true: 作成成功, false: 作成失敗
        [[nodiscard]]
        bool Initialize(GraphicsContext& graphicsContext, RenderResources& resources, FrameConstants& frameConstants);

        //! 大気散乱のパラメータを設定します。
        //! @param  [in] sky 大気散乱の定数バッファデータ
        void SetParameters(const CBufferSky& sky) noexcept {
            m_data = sky;
        }

        //! スカイのシェーダーを作成して、パスを有効にします。
        //! @param  [in] vs 頂点シェーダーアセット
        //! @param  [in] ps ピクセルシェーダーアセット
        void SetPipeline(const Tsukino::Asset::ShaderAsset* vs, const Tsukino::Asset::ShaderAsset* ps);

        //! パイプラインが確立済みかを取得します。
        //! @return true: スカイを描ける, false: まだ SetPipeline() されていない
        [[nodiscard]]
        bool IsReady() const noexcept {
            return m_hasPipeline;
        }

        //! スカイを描きます（パラメータを b4 へ転送してから描きます）。
        void Execute();

        //! 頂点シェーダーを取得します（IBL ベイクのキャプチャで同じシェーダーを使うため）。
        //! @return スカイ用の頂点シェーダー
        [[nodiscard]]
        ID3D11VertexShader* GetVertexShader() const noexcept {
            return m_vs.Get();
        }

        //! ピクセルシェーダーを取得します（同上）。
        //! @return スカイ用のピクセルシェーダー
        [[nodiscard]]
        ID3D11PixelShader* GetPixelShader() const noexcept {
            return m_ps.Get();
        }

        //! スカイの定数バッファ（b4）のアドレスを取得します（同上）。
        //! @return 定数バッファのアドレス
        //! @note   中身は直前の Execute() で今フレームのパラメータに更新されている
        [[nodiscard]]
        ID3D11Buffer* const* GetBufferAddress() const noexcept {
            return m_buffer.GetAddressOf();
        }

    private:
        GraphicsContext* m_graphicsContext = nullptr;    // デバイスとデバイスコンテキストの取得元（借りている）
        RenderResources* m_resources       = nullptr;    // 共通ステートの取得元（借りている）
        FrameConstants*  m_frameConstants  = nullptr;    // b0 の取得元（借りている）

        Microsoft::WRL::ComPtr<ID3D11VertexShader> m_vs;        // スカイ用頂点シェーダー
        Microsoft::WRL::ComPtr<ID3D11PixelShader>  m_ps;        // スカイ用ピクセルシェーダー
        Microsoft::WRL::ComPtr<ID3D11Buffer>       m_buffer;    // スカイ定数バッファ (b4)
        CBufferSky                                 m_data{};    // スカイパラメータ
        bool                                       m_hasPipeline = false;    // SetPipeline() でシェーダーを作れたか
    };
}    // namespace Tsukino::Renderer
