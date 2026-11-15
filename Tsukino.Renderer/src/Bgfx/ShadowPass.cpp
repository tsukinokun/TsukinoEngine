//----------------------------------------------------------------------------
//! @file   ShadowPass.cpp
//! @brief  bgfx 版のシャドウマップパスの実装
//! @detail カスケード1枚を、配列の深度テクスチャの1層と bgfx のビュー1つに当てて描きます。
//!         設定の丸め・カスケードの求め方・間引きは ShadowCascades.hpp にあります。
//----------------------------------------------------------------------------
#include "ShadowPass.hpp"
#include "DrawCommandExecutor.hpp"

#include <Tsukino/Renderer/Bgfx/GraphicsContext.hpp>
#include <Tsukino/Renderer/Bgfx/RenderResourcesGpu.hpp>
#include <Tsukino/Renderer/Bgfx/TextureStages.hpp>
#include <Tsukino/Renderer/Bgfx/Views.hpp>

#include <Tsukino/Engine/Asset/Shader/ShaderAsset.hpp>

#include <Tsukino/Core/Log.hpp>

#include <algorithm>
#include <string>

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {

    //------------------------------------------------------------------------
    //! 資源を持たない状態で作ります。
    //------------------------------------------------------------------------
    ShadowPass::ShadowPass() {
        m_frameBuffers.fill(BGFX_INVALID_HANDLE);    // 既定の値（0）は有効なハンドルなので、無効な値で埋める
    }

    //------------------------------------------------------------------------
    //! シャドウマップとサンプラーを破棄します。
    //------------------------------------------------------------------------
    ShadowPass::~ShadowPass() {
        if(!GraphicsContext::IsBgfxAlive())
            return;
        for(bgfx::FrameBufferHandle frameBuffer : m_frameBuffers) {
            if(bgfx::isValid(frameBuffer))
                bgfx::destroy(frameBuffer);
        }
        if(bgfx::isValid(m_mapTexture))
            bgfx::destroy(m_mapTexture);
        if(bgfx::isValid(m_sampler))
            bgfx::destroy(m_sampler);
    }

    //------------------------------------------------------------------------
    //! シャドウマップと影用パイプラインを作成します。
    //------------------------------------------------------------------------
    bool ShadowPass::Initialize(RenderResources&                   resources,
                                FrameConstants&                    frameConstants,
                                DrawCommandExecutor&               commandExecutor,
                                const Tsukino::Asset::ShaderAsset* staticVS,
                                const Tsukino::Asset::ShaderAsset* skeletalVS,
                                const Tsukino::Asset::ShaderAsset* ps) {
        m_frameConstants  = &frameConstants;
        m_commandExecutor = &commandExecutor;

        m_sampler = bgfx::createUniform("s_shadowMap", bgfx::UniformType::Sampler);

        //--------------------------------------------------------------------
        // シェーダーへ配る値の仮置き。
        // 奥行きは最初の UpdateCascades() でカスケードの大きさから決め直すので、
        // ここでは0除算にならない値を入れておくだけ
        //--------------------------------------------------------------------
        m_frameConstants->SetShadowMapParams(m_settings.mapSize, m_settings.distance * 2.0f + m_settings.casterMargin);

        // シャドウマップを先に作る。影用パイプラインが作れなくても、照らす側は「影なし」の深度を読める
        if(!CreateShadowMap(m_settings.mapSize))
            return false;

        PipelineFactory* factory = resources.GetPipelineFactory();
        if(!factory)
            return false;

        // 静的メッシュ用シャドウパイプライン
        if(staticVS && ps) {
            m_staticPipeline = factory->Create(*staticVS, *ps, Tsukino::GraphicsCommon::VertexFormat::PositionNormalUV, DepthMode::ReadWrite);
            if(!m_staticPipeline) {
                Tsukino::Core::Log::Error("Renderer: Shadow Static Pipeline generation failed.");
                return false;
            }
        }

        // スキニングメッシュ用シャドウパイプライン
        if(skeletalVS && ps) {
            m_skeletalPipeline = factory->Create(*skeletalVS, *ps, Tsukino::GraphicsCommon::VertexFormat::Skinned, DepthMode::ReadWrite);
            if(!m_skeletalPipeline) {
                Tsukino::Core::Log::Error("Renderer: Shadow Skeletal Pipeline generation failed.");
                return false;
            }
        }

        return true;
    }

    //------------------------------------------------------------------------
    //! 影の調整値を設定します。
    //------------------------------------------------------------------------
    void ShadowPass::SetSettings(const ShadowSettings& settings) {
        m_requestedSettings = settings;
        ApplySettings();
    }

    //------------------------------------------------------------------------
    //! 描画品質による影の上限を設定します。
    //------------------------------------------------------------------------
    void ShadowPass::SetQuality(u32 mapSizeLimit, u32 cascadeCount) {
        m_mapSizeLimit       = mapSizeLimit;
        m_activeCascadeCount = std::clamp(cascadeCount, 1u, kCascadeCount);
        ApplySettings();
    }

    //------------------------------------------------------------------------
    //! シーンの調整値へ描画品質の上限を掛けて適用します。
    //------------------------------------------------------------------------
    void ShadowPass::ApplySettings() {
        ShadowSettings limited = m_requestedSettings;
        if(m_mapSizeLimit > 0)
            limited.mapSize = std::min(limited.mapSize, m_mapSizeLimit);

        const u32 previousMapSize = m_settings.mapSize;
        m_settings                = ShadowCascades::SanitizeSettings(limited);

        // Initialize() の前ならシャドウマップはまだ無く、Initialize() が新しい解像度で作る
        if(!bgfx::isValid(m_mapTexture) || m_settings.mapSize == previousMapSize)
            return;

        // 解像度が変わったのでシャドウマップを作り直す。失敗したら前のものがそのまま残る
        if(!CreateShadowMap(m_settings.mapSize)) {
            Tsukino::Core::Log::Error("Failed to recreate the shadow map at " + std::to_string(m_settings.mapSize)
                                      + " px; keeping the previous " + std::to_string(previousMapSize) + " px shadow map.");
            m_settings.mapSize = previousMapSize;
        }
    }

    //------------------------------------------------------------------------
    //! このフレームのカスケードを求めて、ワールドのシーン定数へ書き込みます。
    //------------------------------------------------------------------------
    void ShadowPass::UpdateCascades(const CBufferScene& worldScene) {
        // ディレクショナルライトが一度も設定されていなければ、向きが無いので投影を作れない
        const hlslpp::float3 lightDir = worldScene.lightDir.xyz;
        if(static_cast<float>(hlslpp::dot(lightDir, lightDir)) <= 0.0f)
            return;

        const ShadowCascadeSet cascadeSet = ShadowCascades::ComputeCascades(worldScene.view, worldScene.invViewProj, hlslpp::normalize(lightDir),
                                                                            m_settings, m_activeCascadeCount);
        m_cascades                        = cascadeSet.cascades;

        // 奥行きはカスケードの大きさから毎フレーム決まる。カメラ行列が未設定で求まらなかったフレームは前の値を残す
        const float depthRange = cascadeSet.depthTowardLight + cascadeSet.depthAwayFromLight;
        if(depthRange > 0.0f)
            m_frameConstants->SetShadowMapParams(m_settings.mapSize, depthRange);

        //--------------------------------------------------------------------
        // シェーダーへ渡す形に並べ替える。
        // テクセル幅と分割距離は float4 1本ずつで運ぶので、カスケードは最大4枚まで
        //--------------------------------------------------------------------
        static_assert(kCascadeCount <= 4, "cascadeTexelWorld and cascadeSplitFar are single float4s; they hold at most 4 cascades.");

        std::array<Tsukino::Core::Math::matrix, kCascadeCount> cascadeViewProj{};
        float                                                  texelWorld[4]{};
        float                                                  splitFar[4]{};

        for(u32 i = 0; i < kCascadeCount; ++i) {
            cascadeViewProj[i] = m_cascades[i].lightViewProj;
            texelWorld[i]      = m_cascades[i].texelWorldSize;
            splitFar[i]        = m_cascades[i].splitFar;
        }

        m_frameConstants->SetShadowCascades(cascadeViewProj, hlslpp::float4(texelWorld[0], texelWorld[1], texelWorld[2], texelWorld[3]),
                                            hlslpp::float4(splitFar[0], splitFar[1], splitFar[2], splitFar[3]));
    }

    //------------------------------------------------------------------------
    //! GBuffer パスの描画コマンドのうち、影を落とすものをシャドウマップへ描きます。
    //------------------------------------------------------------------------
    void ShadowPass::Execute(const std::vector<DrawCommand>& commands, Renderer::FrameStats& stats) {
        if(!bgfx::isValid(m_mapTexture))
            return;

        const u16  mapSize     = static_cast<u16>(m_settings.mapSize);
        const bool hasPipeline = m_staticPipeline || m_skeletalPipeline;

        for(u32 cascade = 0; cascade < kCascadeCount; ++cascade) {
            //----------------------------------------------------------------
            // カスケード1枚 = ビュー1つ。この層だけを消す（リバース Z なので 0 で消す）。
            // 何も描かないビューも touch しておかないと、bgfx は消す処理ごと飛ばす
            //----------------------------------------------------------------
            const bgfx::ViewId view = static_cast<bgfx::ViewId>(BgfxView::Shadow + cascade);
            bgfx::setViewFrameBuffer(view, m_frameBuffers[cascade]);
            bgfx::setViewRect(view, 0, 0, mapSize, mapSize);
            bgfx::setViewClear(view, BGFX_CLEAR_DEPTH, 0, 0.0f, 0);
            bgfx::touch(view);

            //----------------------------------------------------------------
            // 使わないカスケードは消すだけで描かない。
            // シェーダーが最後の有効なカスケードの境目で次の層を読むと
            // 「影なし」が返るので、影を出す奥行きの端で影が自然に消える
            //----------------------------------------------------------------
            if(cascade >= m_activeCascadeCount || !hasPipeline)
                continue;

            //----------------------------------------------------------------
            // 描画中のカスケード番号をシーン定数へ流す。
            // 頂点シェーダーはこれを見て cascadeViewProj のどれを使うかを決めるので、
            // シャドウ用シェーダーはカスケードの枚数を知らなくて済む
            //----------------------------------------------------------------
            m_frameConstants->SetShadowCascadeIndex(cascade);
            m_frameConstants->UploadWorld();

            for(const auto& cmd : commands) {
                if(cmd.pass != RenderPass::GBuffer)
                    continue;

                // 影を落とさない物は外す。頂点シェーダーが独自に頂点を組み立てる物は、
                // 同じ動きの影用パイプライン（DrawCommand::shadowPipeline）を渡せば影を落とせる
                if(!cmd.castsShadow)
                    continue;

                if(m_cullingEnabled && !ShadowCascades::OverlapsCascade(cmd, m_cascades[cascade]))
                    continue;

                m_commandExecutor->ExecuteShadow(cmd, view, m_staticPipeline.get(), m_skeletalPipeline.get(), stats);
            }
        }
    }

    //------------------------------------------------------------------------
    //! 次に Submit する描画へ、シャドウマップを張ります。
    //------------------------------------------------------------------------
    void ShadowPass::BindForSampling() const {
        // サンプラーの設定（比較・線形・端で止める）はテクスチャに付けたものをそのまま使う
        if(bgfx::isValid(m_mapTexture))
            bgfx::setTexture(BgfxStage::ShadowMap, m_sampler, m_mapTexture);
    }

    //------------------------------------------------------------------------
    //! シャドウマップ（配列の深度テクスチャと、層ごとのフレームバッファ）を作成します。
    //------------------------------------------------------------------------
    bool ShadowPass::CreateShadowMap(u32 mapSize) {
        //--------------------------------------------------------------------
        // カスケード1枚 = 配列の1層。シェーダーは配列として読み、第3成分でカスケードを選ぶ。
        // 比較サンプラー（PCF 用。線形補間が効く）はテクスチャに付けておく。リバース Z なので GREATER_EQUAL。
        // 端は止めるだけ（範囲外を決まった色で返すサンプラーは、スマホの描画 API に無いことがある）。
        // シェーダーは範囲外の UV を引かず、はみ出すのは PCF の外周の1テクセルだけなので、見た目は変わらない
        //--------------------------------------------------------------------
        constexpr u64 kFlags = BGFX_TEXTURE_RT | BGFX_SAMPLER_COMPARE_GEQUAL | BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP;
        const u16     size   = static_cast<u16>(mapSize);

        const bgfx::TextureHandle texture = bgfx::createTexture2D(size, size, false, static_cast<u16>(kCascadeCount), bgfx::TextureFormat::D32F, kFlags);
        if(!bgfx::isValid(texture)) {
            Tsukino::Core::Log::Error("Failed to create shadow map texture.");
            return false;
        }

        // フレームバッファは層ごとに1つずつ作る。書き込みは「1枚のカスケード」単位
        std::array<bgfx::FrameBufferHandle, kCascadeCount> frameBuffers{};
        frameBuffers.fill(BGFX_INVALID_HANDLE);
        bool created = true;
        for(u32 i = 0; i < kCascadeCount; ++i) {
            bgfx::Attachment attachment;
            attachment.init(texture, bgfx::Access::Write, static_cast<u16>(i), 1, 0, BGFX_ATTACHMENT_NONE);
            frameBuffers[i] = bgfx::createFrameBuffer(1, &attachment, false);
            if(!bgfx::isValid(frameBuffers[i])) {
                Tsukino::Core::Log::Error("Failed to create shadow map frame buffer for cascade " + std::to_string(i) + ".");
                created = false;
                break;
            }
        }

        if(!created) {
            for(bgfx::FrameBufferHandle frameBuffer : frameBuffers) {
                if(bgfx::isValid(frameBuffer))
                    bgfx::destroy(frameBuffer);
            }
            bgfx::destroy(texture);
            return false;
        }

        // 新しいものが揃ってから、前のものを手放す（解像度を変えて作り直すときにも呼ばれる）
        for(bgfx::FrameBufferHandle frameBuffer : m_frameBuffers) {
            if(bgfx::isValid(frameBuffer))
                bgfx::destroy(frameBuffer);
        }
        if(bgfx::isValid(m_mapTexture))
            bgfx::destroy(m_mapTexture);

        m_mapTexture   = texture;
        m_frameBuffers = frameBuffers;
        return true;
    }
}    // namespace Tsukino::Renderer
