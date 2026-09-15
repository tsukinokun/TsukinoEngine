//----------------------------------------------------------------------------
//! @file   FrameConstants.cpp
//! @brief  フレーム単位のシーン定数（b0）の実装
//----------------------------------------------------------------------------
#include <Tsukino/Renderer/FrameConstants.hpp>

#include <Tsukino/Renderer/DX11/GraphicsContext.hpp>
#include <Tsukino/Renderer/ShaderSlots.hpp>

#include <Tsukino/Core/Log.hpp>

#include <cmath>

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {
    //------------------------------------------------------------------------
    //! 定数バッファを作成します。
    //------------------------------------------------------------------------
    bool FrameConstants::Initialize(const GraphicsContext& graphicsContext, u32 shadowMapSize) {
        m_graphicsContext = &graphicsContext;
        m_shadowMapSize   = static_cast<float>(shadowMapSize);

        D3D11_BUFFER_DESC desc = {};
        desc.Usage             = D3D11_USAGE_DEFAULT;
        desc.BindFlags         = D3D11_BIND_CONSTANT_BUFFER;
        desc.ByteWidth         = sizeof(CBufferScene);

        if(FAILED(graphicsContext.GetDevice()->CreateBuffer(&desc, nullptr, m_sceneBuffer.GetAddressOf()))) {
            Tsukino::Core::Log::Error("Failed to create scene constant buffer.");
            return false;
        }

        return true;
    }

    //------------------------------------------------------------------------
    //! ワールド（メインカメラ）のカメラ行列を設定します。
    //------------------------------------------------------------------------
    void FrameConstants::SetWorldCamera(const CBufferScene& data) {
        // カメラ行列のみ更新し、ライト情報は上書きしない
        m_worldSceneData.view        = data.view;
        m_worldSceneData.projection  = data.projection;
        m_worldSceneData.viewProj    = data.viewProj;
        m_worldSceneData.invViewProj = data.invViewProj;
        m_worldSceneData.cameraPos   = data.cameraPos;    // PBR視線ベクトル用
    }

    //------------------------------------------------------------------------
    //! オーバーレイ（UIカメラ）のシーン定数を設定します。
    //------------------------------------------------------------------------
    void FrameConstants::SetOverlayCamera(const CBufferScene& data) {
        m_overlaySceneData = data;
    }

    //------------------------------------------------------------------------
    //! フレームの経過時間を進めます。
    //------------------------------------------------------------------------
    void FrameConstants::AdvanceTime(float deltaTime) {
        m_frameDeltaTime = deltaTime;
        m_elapsedTime += deltaTime;

        //--------------------------------------------------------------------
        // floatの精度が落ちて時間の刻みが粗くなるのを防ぐため、
        // 一定時間で折り返す。sin/cosを使う演出が大半なので、
        // 2πの整数倍で折り返せば見た目に不連続は出ない
        //--------------------------------------------------------------------
        constexpr float kTimeWrap = 6.28318531f * 1000.0f;    // 約6283秒（1時間45分）
        if(m_elapsedTime > kTimeWrap)
            m_elapsedTime -= kTimeWrap;
    }

    //------------------------------------------------------------------------
    //! ディレクショナルライトの情報をワールドのシーン定数へ書き込みます。
    //------------------------------------------------------------------------
    void FrameConstants::SetDirectionalLight(const Tsukino::Core::Math::matrix& lightViewProj,
                                             const hlslpp::float4&              lightDir,
                                             const hlslpp::float4&              lightColor) {
        m_worldSceneData.lightViewProj = lightViewProj;
        m_worldSceneData.lightDir      = lightDir;
        m_worldSceneData.lightColor    = lightColor;
    }

    //------------------------------------------------------------------------
    //! シーン定数へフレーム共通の値を差し込んで b0 へ転送し、VS と PS にバインドします。
    //------------------------------------------------------------------------
    void FrameConstants::Upload(const CBufferScene& sceneData) {
        ID3D11DeviceContext* context = m_graphicsContext->GetContext();

        //--------------------------------------------------------------------
        // 前フレームのViewProjectionを差し込む
        //
        // 呼び出し側（CameraSystem）はこの値を知らないので、
        // 自身が退避しておいたものをここで合流させる
        //--------------------------------------------------------------------
        CBufferScene uploadData = sceneData;
        uploadData.prevViewProj = m_prevWorldViewProj;

        //--------------------------------------------------------------------
        // フレーム共通の素材（時間・解像度・シャドウマップ寸法）を差し込む。
        //
        // prevViewProjと同じ理由でここに集約している：呼び出し側
        // （CameraSystem）はこれらを知らないし、知る必要も無い。
        // ここで埋めておけば、どのシェーダーもb0を宣言するだけで
        // 時間や画面サイズを使えるようになり、演出ごとに専用の
        // 定数バッファを1本ずつ確保する必要が無くなる
        //--------------------------------------------------------------------
        uploadData.timeParams = hlslpp::float4(m_elapsedTime, m_frameDeltaTime, std::sin(m_elapsedTime), std::cos(m_elapsedTime));

        const float screenWidth  = static_cast<float>(m_graphicsContext->GetWidth());
        const float screenHeight = static_cast<float>(m_graphicsContext->GetHeight());
        uploadData.screenParams  = hlslpp::float4(screenWidth, screenHeight, screenWidth > 0.0f ? 1.0f / screenWidth : 0.0f,
                                                  screenHeight > 0.0f ? 1.0f / screenHeight : 0.0f);

        uploadData.shadowParams = hlslpp::float4(m_shadowMapSize, 1.0f / m_shadowMapSize, 0.0f, 0.0f);

        // GPU上のバッファの中身を書き換えて、スロット0（b0）にバインドする
        context->UpdateSubresource(m_sceneBuffer.Get(), 0, nullptr, &uploadData, 0, 0);
        context->VSSetConstantBuffers(static_cast<UINT>(CBSlot::Scene), 1, m_sceneBuffer.GetAddressOf());
        context->PSSetConstantBuffers(static_cast<UINT>(CBSlot::Scene), 1, m_sceneBuffer.GetAddressOf());
    }
}    // namespace Tsukino::Renderer
