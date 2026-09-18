//----------------------------------------------------------------------------
//! @file   FrameConstants.hpp
//! @brief  フレーム単位のシーン定数（b0）の宣言
//! @detail カメラ行列・ディレクショナルライト・経過時間・前フレームの ViewProjection を持ち、
//!         b0（CBufferScene）へ転送します。ワールド（メインカメラ）とオーバーレイ（UIカメラ）の
//!         2組を保持し、描画パスに応じてどちらかを転送します。
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/Core/WindowsLean.hpp>
#include <Tsukino/Core/typedef.hpp>
#include <Tsukino/Core/Math/Matrix.hpp>

#include <Tsukino/Renderer/ConstantBuffer.hpp>

#include <wrl/client.h>
#include <d3d11.h>

#include <hlsl++.h>

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {
    class GraphicsContext;    // 前方宣言

    //------------------------------------------------------------------------
    //! フレーム単位のシーン定数（b0）を持つクラスです。
    //! @note  Renderer が1つだけ所有し、Renderer::GetFrameConstants() で借りる。
    //!        ゲーム側から触るのは SetWorldCamera / SetOverlayCamera / AdvanceTime の3つで、
    //!        残りは描画パスが使う
    //------------------------------------------------------------------------
    class FrameConstants {
    public:

        //! 定数バッファを作成します。
        //! @param  [in] graphicsContext デバイスと画面サイズの取得元（Renderer がこのクラスより長生きさせる）
        //! @param  [in] shadowMapSize   シャドウマップの一辺（b0 の shadowParams へ配る）
        //! @param  [in] shadowWorldSize シャドウマップ1枚が覆うワールドの幅（1テクセルの幅を求めてshadowParams.zへ配る）
        //! @param  [in] shadowDepthRange シャドウの平行投影の奥行き（ワールド距離。逆数をshadowParams.wへ配る）
        //! @return true: 作成成功, false: 作成失敗
        [[nodiscard]]
        bool Initialize(const GraphicsContext& graphicsContext, u32 shadowMapSize, float shadowWorldSize, float shadowDepthRange);

        //! ワールド（メインカメラ）のカメラ行列を設定します。
        //! @param  [in] data カメラ行列を詰めたシーン定数。view / projection / viewProj / invViewProj / cameraPos だけを使う
        //! @note   ライトの情報は上書きしない
        void SetWorldCamera(const CBufferScene& data);

        //! オーバーレイ（UIカメラ）のシーン定数を設定します。
        //! @param  [in] data シーン定数
        void SetOverlayCamera(const CBufferScene& data);

        //! フレームの経過時間を進めます。
        //! @param  [in] deltaTime 前フレームからの経過秒
        //! @note   ここで進めた時間は b0 の timeParams として全シェーダーへ配られる。
        //!         演出ごとに自前の時間を持たずに済むよう、エンジンが1箇所で数える
        //!         （UnityのTime / UEのView.GameTimeと同じ考え方）。毎フレーム1回だけ呼ぶこと
        void AdvanceTime(float deltaTime);

        //! ディレクショナルライトの情報をワールドのシーン定数へ書き込みます。
        //! @param  [in] lightViewProj ライト空間の ViewProjection 行列
        //! @param  [in] lightDir      ライトの方向（xyz。正規化済み）
        //! @param  [in] lightColor    ライトの色（xyz）と強度（w）
        void SetDirectionalLight(const Tsukino::Core::Math::matrix& lightViewProj, const hlslpp::float4& lightDir, const hlslpp::float4& lightColor);

        //! ワールドのシーン定数を取得します。
        //! @return ワールドのシーン定数
        [[nodiscard]]
        const CBufferScene& GetWorldSceneData() const noexcept {
            return m_worldSceneData;
        }

        //! ワールドのシーン定数を b0 へ転送してバインドします。
        void UploadWorld() {
            Upload(m_worldSceneData);
        }

        //! オーバーレイのシーン定数を b0 へ転送してバインドします。
        void UploadOverlay() {
            Upload(m_overlaySceneData);
        }

        //! フレームの終わりの処理をします（次フレームの速度計算用に ViewProjection を退避します）。
        //! @note   CameraSystem は dirty 時しか行列を再計算しないため、
        //!         「送られてきたタイミング」ではなく「フレームの末尾」で退避するのが確実
        void EndFrame() noexcept {
            m_prevWorldViewProj = m_worldSceneData.viewProj;
        }

        //! b0 の定数バッファを取得します。
        //! @return 定数バッファ
        //! @note   IBL ベイクのように、時間や前フレーム行列を差し込まずに視点だけ差し替えて
        //!         直接書き込むパスが使う
        [[nodiscard]]
        ID3D11Buffer* GetSceneBuffer() const noexcept {
            return m_sceneBuffer.Get();
        }

        //! b0 の定数バッファのアドレスを取得します（*SetConstantBuffers へ渡す用）。
        //! @return 定数バッファのアドレス
        [[nodiscard]]
        ID3D11Buffer* const* GetSceneBufferAddress() const noexcept {
            return m_sceneBuffer.GetAddressOf();
        }

    private:

        //! シーン定数へフレーム共通の値を差し込んで b0 へ転送し、VS と PS にバインドします。
        //! @param  [in] sceneData 転送するシーン定数
        void Upload(const CBufferScene& sceneData);

    private:
        const GraphicsContext*               m_graphicsContext = nullptr;    // デバイスと画面サイズの取得元（借りている）
        Microsoft::WRL::ComPtr<ID3D11Buffer> m_sceneBuffer;                  // b0 の定数バッファ
        float                                m_shadowMapSize = 0.0f;         // シャドウマップの一辺（b0 の shadowParams へ配る）
        float                                m_shadowTexelWorldSize = 0.0f;  // シャドウマップ1テクセルのワールド幅（shadowParams.z）
        float                                m_shadowDepthRange     = 1.0f;  // シャドウの平行投影の奥行き（shadowParams.wはその逆数）

        CBufferScene m_worldSceneData{};      // 3D（メインカメラ）用
        CBufferScene m_overlaySceneData{};    // 2D（UIカメラ）用

        //! 前フレームのワールドの ViewProjection 行列（速度バッファ生成用に b0 の prevViewProj へ配る）
        Tsukino::Core::Math::matrix m_prevWorldViewProj = Tsukino::Core::Math::matrix::identity();

        float m_elapsedTime    = 0.0f;    // 起動からの経過秒（b0 の timeParams.x へ配る）
        float m_frameDeltaTime = 0.0f;    // 前フレームからの経過秒（同 timeParams.y）
    };
}    // namespace Tsukino::Renderer
