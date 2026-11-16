//----------------------------------------------------------------------------
//! @file   IBLBaker.cpp
//! @brief  bgfx 版の、スカイ由来の環境光（IBL）のベイクの実装
//! @detail スカイを6面のキューブマップへキャプチャし、拡散用の irradiance 畳み込みと
//!         鏡面用のスペキュラプレフィルタを行います。bgfx は描く先ごとにビューが1つ要るので、
//!         面（とミップ）ごとにビューを1つずつ使います（Views.hpp の Ibl〜）。
//!         積んだ描画は、そのフレームの bgfx::frame() でライティングより先に実行されます。
//----------------------------------------------------------------------------
#include <Tsukino/Renderer/IBLBaker.hpp>
#include <Tsukino/Renderer/Bgfx/FrameConstantsGpu.hpp>
#include <Tsukino/Renderer/Bgfx/GraphicsContext.hpp>
#include <Tsukino/Renderer/Bgfx/IBLBakerGpu.hpp>
#include <Tsukino/Renderer/Bgfx/SkyPassGpu.hpp>
#include <Tsukino/Renderer/Bgfx/TextureStages.hpp>
#include <Tsukino/Renderer/Bgfx/Views.hpp>

#include "FullscreenPass.hpp"

#include <Tsukino/Core/Log.hpp>
#include <Tsukino/Core/Math/Matrix.hpp>

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {

    namespace {
        //! ベイクの描画ステート（深度テストなし・ブレンドなし）
        constexpr u64 kBakeState = BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A;

        //! キューブを読むサンプラー（線形・端で止める）
        constexpr u32 kLinearClamp = BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP | BGFX_SAMPLER_W_CLAMP;

        //! ビューの描く先と大きさを設定します。
        //! @param  [in] view        ビュー
        //! @param  [in] frameBuffer 描く先
        //! @param  [in] size        描く先の一辺
        void SetupBakeView(bgfx::ViewId view, bgfx::FrameBufferHandle frameBuffer, u16 size) {
            bgfx::setViewFrameBuffer(view, frameBuffer);
            bgfx::setViewRect(view, 0, 0, size, size);
        }

        //! キューブの1面（の1ミップ）だけを描く先にしたフレームバッファを作ります。
        //! @param  [in] cube キューブのテクスチャ
        //! @param  [in] face 面（0〜5）
        //! @param  [in] mip  ミップ
        //! @return フレームバッファ
        bgfx::FrameBufferHandle CreateFaceFrameBuffer(bgfx::TextureHandle cube, u32 face, u32 mip) {
            bgfx::Attachment attachment;
            attachment.init(cube, bgfx::Access::Write, static_cast<u16>(face), 1, static_cast<u16>(mip), BGFX_ATTACHMENT_NONE);
            return bgfx::createFrameBuffer(1, &attachment, false);
        }
    }    // namespace

    //------------------------------------------------------------------------
    //! 資源を持たない状態で作ります。
    //------------------------------------------------------------------------
    IBLBaker::Gpu::Gpu() {
        // 既定の値（0）は有効なハンドルなので、無効な値で埋める
        captureFaces.fill(BGFX_INVALID_HANDLE);
        irradianceFaces.fill(BGFX_INVALID_HANDLE);
        specularFaces.fill(BGFX_INVALID_HANDLE);
    }

    //------------------------------------------------------------------------
    //! テクスチャ・フレームバッファ・プログラム・ユニフォームを破棄します。
    //------------------------------------------------------------------------
    IBLBaker::Gpu::~Gpu() {
        if(!GraphicsContext::IsBgfxAlive())
            return;
        for(bgfx::FrameBufferHandle frameBuffer : captureFaces) {
            if(bgfx::isValid(frameBuffer))
                bgfx::destroy(frameBuffer);
        }
        for(bgfx::FrameBufferHandle frameBuffer : irradianceFaces) {
            if(bgfx::isValid(frameBuffer))
                bgfx::destroy(frameBuffer);
        }
        for(bgfx::FrameBufferHandle frameBuffer : specularFaces) {
            if(bgfx::isValid(frameBuffer))
                bgfx::destroy(frameBuffer);
        }
        if(bgfx::isValid(brdfLutFrameBuffer))
            bgfx::destroy(brdfLutFrameBuffer);
        for(bgfx::TextureHandle texture : {capture, irradiance, specular, brdfLut}) {
            if(bgfx::isValid(texture))
                bgfx::destroy(texture);
        }
        for(bgfx::ProgramHandle program : {irradianceProgram, specularProgram, brdfLutProgram}) {
            if(bgfx::isValid(program))
                bgfx::destroy(program);
        }
        for(bgfx::UniformHandle uniform : {iblUniform, irradianceSampler, specularSampler, brdfLutSampler, bakeUniform, captureSampler}) {
            if(bgfx::isValid(uniform))
                bgfx::destroy(uniform);
        }
    }

    //------------------------------------------------------------------------
    //! 環境光の無いテクスチャと、ユニフォームを作ります。
    //------------------------------------------------------------------------
    bool IBLBaker::Gpu::CreatePlaceholder() {
        // キューブは6面とも黒（環境光なし）。BRDF の表も 0（鏡面の環境光なし）
        constexpr u8 kBlackCube[6 * 4] = {};
        constexpr u8 kBlack[4]         = {};
        irradiance = bgfx::createTextureCube(1, false, 1, bgfx::TextureFormat::RGBA8, BGFX_TEXTURE_NONE, bgfx::copy(kBlackCube, sizeof(kBlackCube)));
        specular   = bgfx::createTextureCube(1, false, 1, bgfx::TextureFormat::RGBA8, BGFX_TEXTURE_NONE, bgfx::copy(kBlackCube, sizeof(kBlackCube)));
        brdfLut    = bgfx::createTexture2D(1, 1, false, 1, bgfx::TextureFormat::RGBA8, BGFX_TEXTURE_NONE, bgfx::copy(kBlack, sizeof(kBlack)));

        iblUniform        = bgfx::createUniform("u_ibl", bgfx::UniformType::Vec4);
        irradianceSampler = bgfx::createUniform("s_iblIrradiance", bgfx::UniformType::Sampler);
        specularSampler   = bgfx::createUniform("s_iblSpecular", bgfx::UniformType::Sampler);
        brdfLutSampler    = bgfx::createUniform("s_iblBrdfLut", bgfx::UniformType::Sampler);
        bakeUniform       = bgfx::createUniform("u_iblBake", bgfx::UniformType::Vec4);
        captureSampler    = bgfx::createUniform("s_iblCaptureSource", bgfx::UniformType::Sampler);
        specularMipCount  = 1.0f;
        intensity         = 1.0f;
        return bgfx::isValid(irradiance) && bgfx::isValid(specular) && bgfx::isValid(brdfLut);
    }

    //------------------------------------------------------------------------
    //! ベイク用のキューブマップ・BRDF の積分表と、面ごとのフレームバッファを作ります。
    //------------------------------------------------------------------------
    bool IBLBaker::Gpu::CreateBakeTargets(u16 captureSize, u16 irradianceSize, u16 specularSize, u16 brdfLutSize) {
        // HDR：太陽の強さは 1.0 を大きく超える
        constexpr auto kCubeFormat = bgfx::TextureFormat::RGBA16F;
        constexpr u64  kFlags      = BGFX_TEXTURE_RT | kLinearClamp;

        //--------------------------------------------------------------------
        // 揃うまでは仮のテクスチャを残しておき、途中で失敗したら作りかけを捨てて仮のまま続ける。
        // スペキュラはミップ = ラフネスで読むので、ミップつきで作る
        // （bgfx は 1×1 まで作る。焼くのは先頭の kSpecularMipCount 枚で、読むのもそこまで）
        //--------------------------------------------------------------------
        const bgfx::TextureHandle newCapture    = bgfx::createTextureCube(captureSize, false, 1, kCubeFormat, kFlags);
        const bgfx::TextureHandle newIrradiance = bgfx::createTextureCube(irradianceSize, false, 1, kCubeFormat, kFlags);
        const bgfx::TextureHandle newSpecular   = bgfx::createTextureCube(specularSize, true, 1, kCubeFormat, kFlags);
        const bgfx::TextureHandle newBrdfLut    = bgfx::createTexture2D(brdfLutSize, brdfLutSize, false, 1, bgfx::TextureFormat::RG16F, kFlags);

        bool created = bgfx::isValid(newCapture) && bgfx::isValid(newIrradiance) && bgfx::isValid(newSpecular) && bgfx::isValid(newBrdfLut);
        if(created) {
            for(u32 face = 0; face < kFaceCount; ++face) {
                captureFaces[face]    = CreateFaceFrameBuffer(newCapture, face, 0);
                irradianceFaces[face] = CreateFaceFrameBuffer(newIrradiance, face, 0);
                created               = created && bgfx::isValid(captureFaces[face]) && bgfx::isValid(irradianceFaces[face]);
            }
            for(u32 mip = 0; mip < kSpecularMipCount; ++mip) {
                for(u32 face = 0; face < kFaceCount; ++face) {
                    const u32 index      = mip * kFaceCount + face;
                    specularFaces[index] = CreateFaceFrameBuffer(newSpecular, face, mip);
                    created              = created && bgfx::isValid(specularFaces[index]);
                }
            }
            brdfLutFrameBuffer = bgfx::createFrameBuffer(1, &newBrdfLut, false);
            created            = created && bgfx::isValid(brdfLutFrameBuffer);
        }

        if(!created) {
            for(auto* frameBuffers : {captureFaces.data(), irradianceFaces.data()}) {
                for(u32 face = 0; face < kFaceCount; ++face) {
                    if(bgfx::isValid(frameBuffers[face]))
                        bgfx::destroy(frameBuffers[face]);
                    frameBuffers[face] = BGFX_INVALID_HANDLE;
                }
            }
            for(bgfx::FrameBufferHandle& frameBuffer : specularFaces) {
                if(bgfx::isValid(frameBuffer))
                    bgfx::destroy(frameBuffer);
                frameBuffer = BGFX_INVALID_HANDLE;
            }
            if(bgfx::isValid(brdfLutFrameBuffer))
                bgfx::destroy(brdfLutFrameBuffer);
            brdfLutFrameBuffer = BGFX_INVALID_HANDLE;
            for(bgfx::TextureHandle texture : {newCapture, newIrradiance, newSpecular, newBrdfLut}) {
                if(bgfx::isValid(texture))
                    bgfx::destroy(texture);
            }
            return false;
        }

        // 仮のテクスチャを手放して、本物へ差し替える
        for(bgfx::TextureHandle texture : {irradiance, specular, brdfLut}) {
            if(bgfx::isValid(texture))
                bgfx::destroy(texture);
        }
        capture    = newCapture;
        irradiance = newIrradiance;
        specular   = newSpecular;
        brdfLut    = newBrdfLut;
        return true;
    }

    //------------------------------------------------------------------------
    //! 次に Submit する描画へ、IBL のテクスチャと定数を張ります。
    //------------------------------------------------------------------------
    void IBLBaker::Gpu::Bind() const {
        const float params[4] = {specularMipCount, intensity, 0.0f, 0.0f};
        bgfx::setUniform(iblUniform, params);
        bgfx::setTexture(BgfxStage::IBLIrradiance, irradianceSampler, irradiance, kLinearClamp);
        bgfx::setTexture(BgfxStage::IBLSpecular, specularSampler, specular, kLinearClamp);
        bgfx::setTexture(BgfxStage::IBLBrdfLut, brdfLutSampler, brdfLut, kLinearClamp);
    }

    //------------------------------------------------------------------------
    //! 資源を持たない状態で作ります。
    //------------------------------------------------------------------------
    IBLBaker::IBLBaker()
        : m_gpu(std::make_unique<Gpu>()) {}

    //------------------------------------------------------------------------
    //! 資源を破棄します。
    //------------------------------------------------------------------------
    IBLBaker::~IBLBaker() = default;

    //------------------------------------------------------------------------
    //! 定数のユニフォームと、環境光の無い仮のテクスチャを作成します。
    //------------------------------------------------------------------------
    bool IBLBaker::CreateConstantBuffers(const GraphicsContext& /*graphicsContext*/) {
        if(!m_gpu->CreatePlaceholder()) {
            Tsukino::Core::Log::Error("Failed to create IBL placeholder textures.");
            return false;
        }
        return true;
    }

    //------------------------------------------------------------------------
    //! キューブマップ・BRDF LUT・ベイク用シェーダーを作成し、BRDF LUT を焼きます。
    //------------------------------------------------------------------------
    bool IBLBaker::Initialize(GraphicsContext&                   graphicsContext,
                              RenderResources&                   resources,
                              FrameConstants&                    frameConstants,
                              const FullscreenPass&              fullscreenPass,
                              const SkyPass&                     skyPass,
                              const Tsukino::Asset::ShaderAsset* irradiancePS,
                              const Tsukino::Asset::ShaderAsset* specularPrefilterPS,
                              const Tsukino::Asset::ShaderAsset* brdfLutPS) {
        static_assert(Gpu::kSpecularMipCount == kSpecularMipCount, "IBLBaker::Gpu::kSpecularMipCount must match IBLBaker::kSpecularMipCount.");

        m_graphicsContext = &graphicsContext;
        m_resources       = &resources;
        m_frameConstants  = &frameConstants;
        m_fullscreenPass  = &fullscreenPass;
        m_skyPass         = &skyPass;

        //------------------------------------------------------------
        // ベイク用のプログラム（頂点シェーダーはフルスクリーン三角形用を共用する）
        //------------------------------------------------------------
        if(!irradiancePS || !specularPrefilterPS || !brdfLutPS) {
            Tsukino::Core::Log::Error("IBLBaker::Initialize - shader is null.");
            return false;
        }

        m_gpu->irradianceProgram = fullscreenPass.CreateProgram(irradiancePS);
        m_gpu->specularProgram   = fullscreenPass.CreateProgram(specularPrefilterPS);
        m_gpu->brdfLutProgram    = fullscreenPass.CreateProgram(brdfLutPS);
        if(!bgfx::isValid(m_gpu->irradianceProgram) || !bgfx::isValid(m_gpu->specularProgram) || !bgfx::isValid(m_gpu->brdfLutProgram)) {
            Tsukino::Core::Log::Error("Renderer: The IBL bake shaders are not bgfx shaders; there is no ambient light.");
            return false;
        }

        //------------------------------------------------------------
        // キューブマップ本体3枚と BRDF LUT（スカイに依存しない2D、r: スケールA, g: バイアスB）
        //------------------------------------------------------------
        if(!m_gpu->CreateBakeTargets(static_cast<u16>(kCaptureSize), static_cast<u16>(kIrradianceSize), static_cast<u16>(kSpecularBaseSize),
                                     static_cast<u16>(kBRDFLUTSize))) {
            Tsukino::Core::Log::Error("Renderer: Failed to create one or more IBL cube maps.");
            return false;
        }

        m_hasBakeShaders = true;

        //------------------------------------------------------------
        // 定数の初期値（プレフィルタ済みスペキュラのミップ数はここで確定する）。
        // キューブを焼き終えるまでは中身が決まっていないので、環境光を足さない
        //------------------------------------------------------------
        m_data.specularMipCount = static_cast<float>(kSpecularMipCount);
        m_data.iblIntensity     = 1.0f;
        m_gpu->specularMipCount = m_data.specularMipCount;
        m_gpu->intensity        = 0.0f;

        //------------------------------------------------------------
        // BRDF LUTはスカイに依存しないため、ここで即座に一度だけベイクする
        // （キャプチャ/irradiance/プレフィルタはスカイパイプライン確立後にRender()側でトリガーされる）
        //------------------------------------------------------------
        ExecuteBRDFLUTPass();

        return true;
    }

    //------------------------------------------------------------------------
    //! まだ焼いていなければベイクします（キャプチャ → irradiance → スペキュラプレフィルタ）。
    //------------------------------------------------------------------------
    bool IBLBaker::BakeIfNeeded() {
        if(!m_skyPass || !m_skyPass->IsReady() || !m_hasBakeShaders || m_baked)
            return false;

        ExecuteCapturePass();
        ExecuteIrradiancePass();
        ExecuteSpecularPrefilterPass();
        m_baked = true;

        // 焼いたキューブは、同じフレームのライティングから読める（ベイクのビューが先に実行される）
        m_gpu->intensity = m_data.iblIntensity;
        return true;
    }

    //------------------------------------------------------------------------
    //! IBL の結果を、次に Submit する描画へ張ります。
    //------------------------------------------------------------------------
    void IBLBaker::Bind() {
        m_gpu->Bind();
    }

    //------------------------------------------------------------------------
    //! 原点から見たキューブの1面ぶんのシーン定数を組み立てます（キューブ面の並び +X,-X,+Y,-Y,+Z,-Z に合わせてあります）。
    //------------------------------------------------------------------------
    CBufferScene IBLBaker::BuildCubeFaceSceneData(u32 face) const {
        static const hlslpp::float3 kFaceTargets[6] = {
            hlslpp::float3(1.0f, 0.0f, 0.0f),
            hlslpp::float3(-1.0f, 0.0f, 0.0f),
            hlslpp::float3(0.0f, 1.0f, 0.0f),
            hlslpp::float3(0.0f, -1.0f, 0.0f),
            hlslpp::float3(0.0f, 0.0f, 1.0f),
            hlslpp::float3(0.0f, 0.0f, -1.0f),
        };
        static const hlslpp::float3 kFaceUps[6] = {
            hlslpp::float3(0.0f, 1.0f, 0.0f),
            hlslpp::float3(0.0f, 1.0f, 0.0f),
            hlslpp::float3(0.0f, 0.0f, -1.0f),
            hlslpp::float3(0.0f, 0.0f, 1.0f),
            hlslpp::float3(0.0f, 1.0f, 0.0f),
            hlslpp::float3(0.0f, 1.0f, 0.0f),
        };

        const u32            idx = (face < 6) ? face : 0;
        const hlslpp::float3 eye = hlslpp::float3(0.0f, 0.0f, 0.0f);

        // lightDir/lightColor/timeParams等はメインカメラのフレームデータをそのまま引き継ぐ
        // （Sky.ps が太陽の色・向きとして lightColor/lightDir を読むため）
        CBufferScene data = m_frameConstants->GetWorldSceneData();

        constexpr float kFovRadians = 1.5707963267948966f;    // 90度：キューブの1面をちょうど覆う画角
        constexpr float kNearZ      = 0.1f;
        constexpr float kFarZ       = 100.0f;

        data.view        = Tsukino::Core::Math::matrix::lookAtLH(eye, eye + kFaceTargets[idx], kFaceUps[idx]);
        // リバースZ対応のperspectiveFovLHはCameraSystemと同じくfarZ, nearZの順で渡す
        data.projection  = Tsukino::Core::Math::matrix::perspectiveFovLH(kFovRadians, 1.0f, kFarZ, kNearZ);
        data.viewProj    = hlslpp::mul(data.view, data.projection);
        data.invViewProj = hlslpp::inverse(data.viewProj);
        data.cameraPos   = hlslpp::float4(eye.x, eye.y, eye.z, 1.0f);

        return data;
    }

    //------------------------------------------------------------------------
    //! スカイを6面のキューブマップへ焼きます（Sky.ps を面ごとに視点だけ差し替えて使います）。
    //------------------------------------------------------------------------
    void IBLBaker::ExecuteCapturePass() {
        const SkyPass::Gpu& sky = m_skyPass->GetGpu();
        if(!bgfx::isValid(m_gpu->capture) || !bgfx::isValid(sky.program))
            return;

        for(u32 face = 0; face < Gpu::kFaceCount; ++face) {
            const bgfx::ViewId view = static_cast<bgfx::ViewId>(BgfxView::IblCapture + face);
            SetupBakeView(view, m_gpu->captureFaces[face], static_cast<u16>(kCaptureSize));

            // 面ごとの視点と、このフレームの大気散乱の定数（SkyPass::Execute が覚えた値）
            m_frameConstants->GetGpu().scene.Set(BuildCubeFaceSceneData(face));
            sky.sky.Set(sky.data);

            m_fullscreenPass->Submit(view, sky.program, kBakeState);
        }
    }

    //------------------------------------------------------------------------
    //! 拡散IBL用の irradiance を畳み込みます（面ごとに1回）。
    //------------------------------------------------------------------------
    void IBLBaker::ExecuteIrradiancePass() {
        if(!bgfx::isValid(m_gpu->irradiance) || !bgfx::isValid(m_gpu->capture) || !bgfx::isValid(m_gpu->irradianceProgram))
            return;

        for(u32 face = 0; face < Gpu::kFaceCount; ++face) {
            const bgfx::ViewId view = static_cast<bgfx::ViewId>(BgfxView::IblIrradiance + face);
            SetupBakeView(view, m_gpu->irradianceFaces[face], static_cast<u16>(kIrradianceSize));

            m_frameConstants->GetGpu().scene.Set(BuildCubeFaceSceneData(face));
            bgfx::setTexture(BgfxStage::IBLCaptureSource, m_gpu->captureSampler, m_gpu->capture, kLinearClamp);

            m_fullscreenPass->Submit(view, m_gpu->irradianceProgram, kBakeState);
        }
    }

    //------------------------------------------------------------------------
    //! 鏡面IBL用のスペキュラをプレフィルタします（面×mipごとに1回）。
    //------------------------------------------------------------------------
    void IBLBaker::ExecuteSpecularPrefilterPass() {
        if(!bgfx::isValid(m_gpu->specular) || !bgfx::isValid(m_gpu->capture) || !bgfx::isValid(m_gpu->specularProgram))
            return;

        // 一発ベイクなのでサンプル数はケチらない。mipが荒い（ラフネスが高い）ほど
        // GGXローブが広がりノイズが出やすいため、mipに応じて増やす
        static const u32 kSampleCountPerMip[6] = {32, 64, 96, 128, 192, 256};

        for(u32 mip = 0; mip < kSpecularMipCount; ++mip) {
            const float roughness   = (kSpecularMipCount > 1) ? (static_cast<float>(mip) / static_cast<float>(kSpecularMipCount - 1)) : 0.0f;
            const float bakeData[4] = {roughness, static_cast<float>(kSampleCountPerMip[(mip < 6) ? mip : 5]), 0.0f, 0.0f};
            const u16   mipSize     = static_cast<u16>(kSpecularBaseSize >> mip);

            for(u32 face = 0; face < Gpu::kFaceCount; ++face) {
                const u32          index = mip * Gpu::kFaceCount + face;
                const bgfx::ViewId view  = static_cast<bgfx::ViewId>(BgfxView::IblSpecular + index);
                SetupBakeView(view, m_gpu->specularFaces[index], mipSize);

                m_frameConstants->GetGpu().scene.Set(BuildCubeFaceSceneData(face));
                bgfx::setUniform(m_gpu->bakeUniform, bakeData);
                bgfx::setTexture(BgfxStage::IBLCaptureSource, m_gpu->captureSampler, m_gpu->capture, kLinearClamp);

                m_fullscreenPass->Submit(view, m_gpu->specularProgram, kBakeState);
            }
        }
    }

    //------------------------------------------------------------------------
    //! BRDF LUT を生成します（スカイに依存しないため起動時に1回だけ呼ぶ）。
    //------------------------------------------------------------------------
    void IBLBaker::ExecuteBRDFLUTPass() {
        if(!bgfx::isValid(m_gpu->brdfLutFrameBuffer) || !bgfx::isValid(m_gpu->brdfLutProgram))
            return;

        SetupBakeView(BgfxView::IblBrdfLut, m_gpu->brdfLutFrameBuffer, static_cast<u16>(kBRDFLUTSize));
        m_fullscreenPass->Submit(BgfxView::IblBrdfLut, m_gpu->brdfLutProgram, kBakeState);
    }

}    // namespace Tsukino::Renderer
