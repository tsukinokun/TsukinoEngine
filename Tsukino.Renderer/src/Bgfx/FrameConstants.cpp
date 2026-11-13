//----------------------------------------------------------------------------
//! @file   FrameConstants.cpp
//! @brief  bgfx 版の FrameConstants の実装
//! @detail シーン定数の組み立て（時間・解像度・前フレームの ViewProjection などの差し込み）は
//!         DX11 版と同じです。違うのは送り方で、Upload では値を覚えるだけにし、
//!         描画を Submit する直前に FrameConstants::Gpu::Apply がユニフォームで送ります。
//----------------------------------------------------------------------------
#include <Tsukino/Renderer/FrameConstants.hpp>
#include <Tsukino/Renderer/Bgfx/FrameConstantsGpu.hpp>
#include <Tsukino/Renderer/Bgfx/GraphicsContext.hpp>

#include <Tsukino/Core/Log.hpp>

#include <cmath>

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {

    //------------------------------------------------------------------------
    //! 資源を持たない状態で作ります。
    //------------------------------------------------------------------------
    FrameConstants::FrameConstants()
        : m_gpu(std::make_unique<Gpu>()) {}

    //------------------------------------------------------------------------
    //! 資源を破棄します。
    //------------------------------------------------------------------------
    FrameConstants::~FrameConstants() = default;

    //------------------------------------------------------------------------
    //! シーン定数のユニフォームを作成します。
    //------------------------------------------------------------------------
    bool FrameConstants::Initialize(const GraphicsContext& graphicsContext) {
        m_graphicsContext = &graphicsContext;
        if(!m_gpu->scene.Create("u_scene")) {
            Tsukino::Core::Log::Error("FrameConstants: Failed to create the scene uniform.");
            return false;
        }
        return true;
    }

    //------------------------------------------------------------------------
    //! 3D のカメラを設定します（ライトの情報は上書きしない。DX11 版と同じ）。
    //------------------------------------------------------------------------
    void FrameConstants::SetWorldCamera(const CBufferScene& data) {
        m_worldSceneData.view        = data.view;
        m_worldSceneData.projection  = data.projection;
        m_worldSceneData.viewProj    = data.viewProj;
        m_worldSceneData.invViewProj = data.invViewProj;
        m_worldSceneData.cameraPos   = data.cameraPos;
    }

    //------------------------------------------------------------------------
    //! 2D（UI）のカメラを設定します。
    //------------------------------------------------------------------------
    void FrameConstants::SetOverlayCamera(const CBufferScene& data) {
        m_overlaySceneData = data;
    }

    //------------------------------------------------------------------------
    //! ワールドのシーン定数のカメラだけを差し替えて覚えます。
    //------------------------------------------------------------------------
    void FrameConstants::UploadWorldWithCamera(const CBufferScene& camera) {
        CBufferScene sceneData = m_worldSceneData;
        sceneData.view         = camera.view;
        sceneData.projection   = camera.projection;
        sceneData.viewProj     = camera.viewProj;
        sceneData.invViewProj  = camera.invViewProj;
        sceneData.cameraPos    = camera.cameraPos;
        Upload(sceneData, true);
    }

    //------------------------------------------------------------------------
    //! 経過時間を進めます（DX11 版と同じく、一定時間で折り返す）。
    //------------------------------------------------------------------------
    void FrameConstants::AdvanceTime(float deltaTime) {
        m_frameDeltaTime = deltaTime;
        m_elapsedTime += deltaTime;

        constexpr float kTimeWrap = 6.28318531f * 1000.0f;    // 2πの整数倍で折り返せば sin/cos に段差が出ない
        if(m_elapsedTime > kTimeWrap)
            m_elapsedTime -= kTimeWrap;
    }

    //------------------------------------------------------------------------
    //! 影のカスケードをワールドのシーン定数へ書き込みます。
    //------------------------------------------------------------------------
    void FrameConstants::SetShadowCascades(const std::array<Tsukino::Core::Math::matrix, kShadowCascadeCount>& cascadeViewProj,
                                           const hlslpp::float4& cascadeTexelWorld, const hlslpp::float4& cascadeSplitFar) {
        for(unsigned int i = 0; i < kShadowCascadeCount; ++i)
            m_worldSceneData.cascadeViewProj[i] = cascadeViewProj[i];

        m_worldSceneData.cascadeTexelWorld = cascadeTexelWorld;
        m_worldSceneData.cascadeSplitFar   = cascadeSplitFar;
    }

    //------------------------------------------------------------------------
    //! シーン定数へフレーム共通の値を差し込んで、次の描画で送る値として覚えます。
    //------------------------------------------------------------------------
    void FrameConstants::Upload(const CBufferScene& sceneData, bool /*targetBackBuffer*/) {
        CBufferScene uploadData = sceneData;

        // 前フレームの ViewProjection（呼び出し側は知らないので、ここで合流させる）
        uploadData.prevViewProj = m_prevWorldViewProj;

        // 経過時間
        uploadData.timeParams = hlslpp::float4(m_elapsedTime, m_frameDeltaTime, std::sin(m_elapsedTime), std::cos(m_elapsedTime));

        //--------------------------------------------------------------------
        // 解像度。bgfx 版はまだ内部解像度を下げた描画先を持たないので、
        // ワールドもオーバーレイもバックバッファの大きさにする
        //--------------------------------------------------------------------
        const float width       = m_graphicsContext ? static_cast<float>(m_graphicsContext->GetWidth()) : 0.0f;
        const float height      = m_graphicsContext ? static_cast<float>(m_graphicsContext->GetHeight()) : 0.0f;
        uploadData.screenParams = hlslpp::float4(width, height, width > 0.0f ? 1.0f / width : 0.0f, height > 0.0f ? 1.0f / height : 0.0f);

        // 描画品質と影の寸法（z はシャドウパスが描いているカスケード番号なので、そのまま運ぶ）
        uploadData.qualityParams = hlslpp::float4(m_shadowSoftFilter, 0.0f, 0.0f, 0.0f);
        const float mapSize      = m_shadowMapSize > 0.0f ? m_shadowMapSize : 1.0f;
        uploadData.shadowParams  = hlslpp::float4(m_shadowMapSize, 1.0f / mapSize, sceneData.shadowParams.z, 1.0f / m_shadowDepthRange);

        m_gpu->current = uploadData;
    }

}    // namespace Tsukino::Renderer
