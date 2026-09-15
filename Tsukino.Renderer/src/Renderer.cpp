//------------------------------------------------------------
//! @file   Renderer.cpp
//! @brief  レンダラークラスの実装
//! @author 山﨑愛
//------------------------------------------------------------
#include <Tsukino/Engine/Asset/Texture/TextureAsset.hpp>

#include <Tsukino/Renderer/Renderer.hpp>
#include <Tsukino/Renderer/ShaderLoader.hpp>
#include <Tsukino/Renderer/ConstantBuffer.hpp>
#include <Tsukino/Renderer/ShaderSlots.hpp>

#include <Tsukino/Renderer/IPostWorldPass.hpp>

#include "DrawCommandExecutor.hpp"
#include "FullscreenPass.hpp"
#include "TonemapPass.hpp"
#include "ShadowPass.hpp"

#include <Tsukino/Engine/Asset/Shader/ShaderAsset.hpp>

#include <Tsukino/GraphicsCommon/Mesh/MeshPrimitives.hpp>

#include <Tsukino/Core/Log.hpp>

#include <cassert>
#include <d3dcompiler.h>
#include <algorithm>
#include <cstring>

#pragma comment(lib, "d3dcompiler.lib")

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {
    //------------------------------------------------------------
    //! @brief コンストラクタ
    //! @note  内部専用の部品を前方宣言だけで持っているため、
    //!        その完全型が見える Renderer.cpp で定義する
    //------------------------------------------------------------
    Renderer::Renderer() = default;

    //------------------------------------------------------------
    //! @brief デストラクタ
    //------------------------------------------------------------
    Renderer::~Renderer() = default;

    //------------------------------------------------------------
    //! @brief レンダラーの初期化
    //------------------------------------------------------------
    bool Renderer::Initialize(HWND hwnd, uint32_t width, uint32_t height, const RendererShaderSet& shaders) {
        // グラフィックスコンテキストの初期化
        if(!m_graphicsContext.Initialize(hwnd, width, height)) {
            return false;
        }

        Tsukino::Core::Log::Info("sizeof(hlslpp::float4x4) = " + std::to_string(sizeof(hlslpp::float4x4)));
        Tsukino::Core::Log::Info("sizeof(CBufferSkinning) = " + std::to_string(sizeof(CBufferSkinning)));

        ID3D11Device*        device  = m_graphicsContext.GetDevice();     // DirectXのDevice
        ID3D11DeviceContext* context = m_graphicsContext.GetContext();    // DirectXのDeviceContext

        //------------------------------------------------------------
        // 描画で共有する資源（PipelineFactory・共通ステート・サンプラー・
        // プリミティブメッシュ・既定テクスチャ）の作成。
        // 以降のパイプライン生成が PipelineFactory を使うため最初に行う
        //------------------------------------------------------------
        if(!m_resources.Initialize(device, context))
            return false;

        //------------------------------------------------------------
        // フレーム単位のシーン定数（b0）の作成
        //------------------------------------------------------------
        if(!m_frameConstants.Initialize(m_graphicsContext, ShadowPass::kMapSize))
            return false;

        //------------------------------------------------------------
        // 描画コマンドの実行器（Transform・マテリアル・ボーン行列の定数バッファ）の作成
        //------------------------------------------------------------
        m_commandExecutor = std::make_unique<DrawCommandExecutor>();
        if(!m_commandExecutor->Initialize(m_graphicsContext, m_resources, m_frameConstants))
            return false;

        //------------------------------------------------------------
        // シャドウマップパス（影用パイプラインとシャドウマップ）の作成
        //------------------------------------------------------------
        m_shadowPass = std::make_unique<ShadowPass>();
        if(!m_shadowPass->Initialize(m_graphicsContext, m_resources, m_frameConstants, *m_commandExecutor, shaders.shadowStaticVS,
                                     shaders.shadowSkeletalVS, shaders.shadowPS)) {
            Tsukino::Core::Log::Error("Failed to create shadow pipelines.");
            return false;
        }

        //------------------------------------------------------------
        // 定数バッファの作成
        //------------------------------------------------------------
        if(!CreateConstantBuffer())
            return false;    // 定数バッファの作成に失敗した場合は false を返す

        //------------------------------------------------------------
        // スカイの定数バッファ（b4）と IBL の定数バッファ（b10 / b11）の作成。
        // IBL を使わないときもライティングが b10 を読むので、ここは失敗を許さない
        //------------------------------------------------------------
        if(!m_skyPass.Initialize(m_graphicsContext, m_resources, m_frameConstants))
            return false;

        if(!m_iblBaker.CreateConstantBuffers(device))
            return false;

        //------------------------------------------------------------
        // デバッグ描画（線・三角形）の作成
        //------------------------------------------------------------
        if(!m_debugDraw.Initialize(m_graphicsContext, m_resources, m_frameConstants, shaders.debugVS, shaders.debugPS))
            return false;

        //------------------------------------------------------------
        // フルスクリーン三角形（ライティング・フォグ・モーションブラー・トーンマップ・
        // IBLベイクの共用VS）とトーンマップパスの作成。
        // 失敗しても初期化は続行し、それぞれのパスが何もしなくなるだけにする
        //------------------------------------------------------------
        m_fullscreenPass = std::make_unique<FullscreenPass>();
        if(!m_fullscreenPass->Initialize(device, shaders.tonemapVS)) {
            Tsukino::Core::Log::Error("Renderer: Fullscreen passes are disabled because their vertex shader could not be created.");
        }

        m_tonemapPass = std::make_unique<TonemapPass>();
        if(!m_tonemapPass->Initialize(m_graphicsContext, m_resources, *m_fullscreenPass, shaders.tonemapPS)) {
            Tsukino::Core::Log::Error("Renderer: Tonemapping is disabled because its pixel shader could not be created.");
        }

        //------------------------------------------------------------
        // ディファードLightingパスの作成
        // GBufferパスのPS(gbufferPS)はModelSystem側でPipelineFactory経由の
        // 通常のDrawCommandとして扱うため、ここでは不要。
        //------------------------------------------------------------
        if(!m_lightingPass.Initialize(m_graphicsContext, m_resources, m_frameConstants, *m_fullscreenPass, *m_shadowPass, m_iblBaker,
                                      shaders.lightingPS))
            return false;

        //------------------------------------------------------------
        // モーションブラーパイプラインの作成
        // 演出用の任意機能なので、失敗しても描画自体は続行する
        // （m_hasMotionBlur が false のままになり、パスがスキップされる）。
        //------------------------------------------------------------
        if(!SetMotionBlurPipeline(shaders.motionBlurPS)) {
            Tsukino::Core::Log::Error("Renderer: Motion blur is disabled because its pixel shader could not be created.");
        }

        if(!SetFogPipeline(shaders.fogPS)) {
            Tsukino::Core::Log::Error("Renderer: Fog is disabled because its pixel shader could not be created.");
        }

        if(!SetAmbientParticlePipeline(shaders.ambientParticleVS, shaders.ambientParticlePS)) {
            Tsukino::Core::Log::Error("Renderer: Ambient particles are disabled because their shaders could not be created.");
        }

        //------------------------------------------------------------
        // IBL（スカイ由来の環境光）用リソースの作成
        // 演出用の任意機能なので、失敗しても描画自体は続行する
        // （ベイクが走らず、アンビエントが
        //   常に0扱いになるだけで済む）。
        //------------------------------------------------------------
        if(!m_iblBaker.Initialize(m_graphicsContext, m_resources, m_frameConstants, *m_fullscreenPass, m_skyPass, shaders.iblIrradiancePS,
                                  shaders.iblSpecularPrefilterPS, shaders.iblBRDFLUTPS)) {
            Tsukino::Core::Log::Error("Renderer: IBL is disabled because its resources could not be created.");
        }

        return true;
    }

    //------------------------------------------------------------
    //! @brief 定数バッファの作成
    //------------------------------------------------------------
    bool Renderer::CreateConstantBuffer() {
        //デバイスを取得
        ID3D11Device* device = m_graphicsContext.GetDevice();

        D3D11_BUFFER_DESC desc = {};
        desc.Usage             = D3D11_USAGE_DEFAULT;
        desc.BindFlags         = D3D11_BIND_CONSTANT_BUFFER;

        //------------------------------------------------------------
        // m_motionBlurBuffer (b8) の作成
        //------------------------------------------------------------
        desc.ByteWidth = sizeof(Tsukino::Renderer::CBufferMotionBlur);
        HRESULT hr     = device->CreateBuffer(&desc, nullptr, m_motionBlurBuffer.GetAddressOf());
        if(FAILED(hr)) {
            Tsukino::Core::Log::Error("Failed to create motion blur constant buffer.");
            return false;
        }

        //------------------------------------------------------------
        // m_fogBuffer (b9) の作成
        //------------------------------------------------------------
        desc.ByteWidth = sizeof(Tsukino::Renderer::CBufferFog);
        hr             = device->CreateBuffer(&desc, nullptr, m_fogBuffer.GetAddressOf());
        if(FAILED(hr)) {
            Tsukino::Core::Log::Error("Failed to create fog constant buffer.");
            return false;
        }

        //------------------------------------------------------------
        // m_ambientParticleBuffer (b9) の作成
        //------------------------------------------------------------
        desc.ByteWidth = sizeof(Tsukino::Renderer::CBufferAmbientParticle);
        hr             = device->CreateBuffer(&desc, nullptr, m_ambientParticleBuffer.GetAddressOf());
        if(FAILED(hr)) {
            Tsukino::Core::Log::Error("Failed to create ambient particle constant buffer.");
            return false;
        }

        // 成功
        return true;
    }

    //------------------------------------------------------------
    //! @brief 描画処理
    //------------------------------------------------------------
    void Renderer::Render(IPostWorldPass* postWorldPass) {
        m_graphicsContext.BeginFrame(m_clearColor[0], m_clearColor[1], m_clearColor[2], m_clearColor[3]);

        const auto& commands = m_drawQueue.GetCommands();

        //------------------------------------------------------------
        // 今フレームの描画統計をリセットする（負荷調査用）
        // 実際の加算は各Execute*Commandが DrawIndexed の直前で行うため、
        // 早期returnで描かれなかったコマンドは数に入らない
        //------------------------------------------------------------
        m_frameStats              = FrameStats{};
        m_frameStats.commandCount = static_cast<u32>(commands.size());

        //------------------------------------------------------------
        // Shadow パス（ディファードGBufferの対象＝不透明3Dモデルのみ影を落とす）
        //------------------------------------------------------------
        if(m_shadowPass->Execute(commands, m_frameStats)) {
            // RTとビューポートをBeginFrame時の状態に戻す
            m_graphicsContext.BeginFrame(m_clearColor[0], m_clearColor[1], m_clearColor[2], m_clearColor[3]);
        }

        //------------------------------------------------------------
        // Sky パス（GBufferパスの前、深度書き込みなし）
        //------------------------------------------------------------
        m_frameConstants.UploadWorld();
        m_skyPass.Execute();

        //------------------------------------------------------------
        // IBLベイク（スカイのキャプチャ→irradiance畳み込み→スペキュラプレフィルタ）
        //
        // スカイのパイプラインが確立する（=SkyAtmosphereSystemが初めてスカイパイプラインを
        // 確立した）最初のフレームで一度だけ走る。CombatAndroidには現状
        // day-night系のシステムが無く太陽方向はシーン起動時の1回きりなので、
        // これで十分。将来太陽が動く演出が入ったら、そのシステムが
        // IBLBaker::RequestRecapture()を呼べば次フレームでここが再び走る
        // （毎フレーム呼ぶと6+6+36=48回のフルスクリーン三角形描画が
        // 毎フレーム発生するため、呼び出し側でのスロットリングが前提）。
        //
        // ここに置く理由：直前のスカイパスでスカイのパラメータ（b4）がこのフレームの
        // 太陽方向で確定済みであり、直後のm_frameConstants.UploadWorld()
        // （GBufferパスの直前）が、ここで一時的に書き換えたCBufferScene(b0)を
        // 本来のカメラ値へ確実に戻してくれる
        //------------------------------------------------------------
        m_iblBaker.BakeIfNeeded();

        //------------------------------------------------------------
        // GBuffer パス（不透明3Dモデル。ライティングは計算せずG-Bufferへ書き込むだけ）
        //------------------------------------------------------------
        m_frameConstants.UploadWorld();
        m_graphicsContext.BeginGBufferPass();
        for(const auto& cmd : commands) {
            if(cmd.pass != RenderPass::GBuffer)
                continue;
            m_commandExecutor->Execute(cmd, m_motionBlurEnabled, m_frameStats);
        }

        //------------------------------------------------------------
        // Lighting パス（G-Bufferと深度から全ライトを1回でHDRへ加算する）
        //------------------------------------------------------------
        m_lightingPass.Execute();

        //------------------------------------------------------------
        // HDRバッファへ復帰（Lightingの結果を保持したままDSVも再度有効化）
        // 以降のWorld/TransparentはG-Bufferパスで書いた深度に対して
        // 正しく前後関係が出る（デバッグ線や半透明が不透明オブジェクトの後ろに隠れる）
        //------------------------------------------------------------
        m_graphicsContext.BindHDRRenderTarget();

        //------------------------------------------------------------
        // World パス（デバッグ線などcustomDraw経由のフォワード不透明）
        //------------------------------------------------------------
        {
            ID3D11DeviceContext* context = m_graphicsContext.GetContext();
            // シャドウマップをt8・s8にバインド（フォワードでライティングするシェーダー向け）
            m_shadowPass->BindForSampling(context);

            // IBL (b10, t17〜t19, s10)：Model.ps.hlslのEvaluateIBL向け。
            // World/TransparentDepth/Transparentの3パスを通して張りっぱなしにする
            m_iblBaker.Bind();
        }

        m_frameConstants.UploadWorld();
        for(const auto& cmd : commands) {
            if(cmd.pass != RenderPass::World)
                continue;
            m_commandExecutor->Execute(cmd, m_motionBlurEnabled, m_frameStats);
        }

        //------------------------------------------------------------
        // TransparentDepth パス（Transparentの直前。色を書かず深度だけ埋める）
        //
        // 半透明モデル（スキンメッシュの自己重なりが多い）が同じピクセルへ
        // 何度もブレンドされて過度に濃く見える問題を避けるため、先に一番手前の
        // 深度だけを確定させておく。Transparent側はEqualReadOnlyでこの深度と
        // 一致する画素だけを1回シェーディングする
        //------------------------------------------------------------
        m_frameConstants.UploadWorld();
        for(const auto& cmd : commands) {
            if(cmd.pass != RenderPass::TransparentDepth)
                continue;
            m_commandExecutor->Execute(cmd, m_motionBlurEnabled, m_frameStats);
        }

        //------------------------------------------------------------
        // Transparent パス（不透明の後、水面の前）
        //
        // ブレンドと深度の設定は各コマンドの PipelineState が持っているため、
        // ここでは実行順を分けるだけでよい。
        // 半透明同士の前後関係を正しく出すには奥から手前への
        // ソートが必要だが、それはコマンドキュー側の課題として未対応。
        //------------------------------------------------------------
        m_frameConstants.UploadWorld();
        for(const auto& cmd : commands) {
            if(cmd.pass != RenderPass::Transparent)
                continue;
            m_commandExecutor->Execute(cmd, m_motionBlurEnabled, m_frameStats);
        }

        //------------------------------------------------------------
        // シャドウマップ・IBLのバインドを解除（DSVとSRVの同時バインド防止）
        //------------------------------------------------------------
        {
            ID3D11DeviceContext*      context       = m_graphicsContext.GetContext();
            ID3D11ShaderResourceView* nullSRV       = nullptr;
            constexpr UINT            shadowSRVSlot = static_cast<UINT>(SRVSlot::ShadowMap);
            context->PSSetShaderResources(shadowSRVSlot, 1, &nullSRV);
            m_iblBaker.Unbind();
        }

        //------------------------------------------------------------
        // 環境パーティクルパス（HDRバッファへ加算合成）
        //
        // 深度テストありなので世界の物体にきちんと隠れ、深度は書かないので
        // 粒子どうしは順序を気にせず重なる。フォグの前に置くことで、
        // 「3Dの絵すべてにフォグが乗る」という下のパスの前提を壊さずに済む
        // （粒子だけがフォグの掛かっていない浮いた点に見えるのを防ぐ）。
        //------------------------------------------------------------
        ExecuteAmbientParticlePass();

        //------------------------------------------------------------
        // フォグパス（HDRバッファへ直接over合成）
        //
        // 不透明・空・半透明・水面がすべて描き終わったここで掛けることで、
        // 3Dの絵すべてにフォグが乗る。UIとエフェクトはトーンマップ後に
        // バックバッファへ直接描かれるため影響を受けない。
        //------------------------------------------------------------
        ExecuteFogPass();

        //------------------------------------------------------------
        // モーションブラーパス（HDR → ポストプロセス用中間バッファ）
        //
        // 3Dの描画がすべて終わり、UI/エフェクトが乗る前のここで掛ける。
        // 無効なら何もせず false を返すので、その場合はHDRをそのまま
        // トーンマップへ渡す。
        //------------------------------------------------------------
        const bool motionBlurred = ExecuteMotionBlurPass();

        //------------------------------------------------------------
        // Tonemapパス（HDR → LDR変換してバックバッファへ）
        //------------------------------------------------------------
        m_tonemapPass->Execute(motionBlurred ? m_graphicsContext.GetPostProcessSRV() : m_graphicsContext.GetHDRSRV());

        //------------------------------------------------------------
        // エフェクト描画パス（Overlayより前）
        //
        // エフェクトは3D空間の演出なので、UIより奥に描く。Overlayの後に置くと
        // 画面を覆うモーダル（スキル選択の暗転板など）の上にヒットエフェクトだけが
        // 残ってしまう
        //------------------------------------------------------------
        if(postWorldPass) {
            ID3D11DeviceContext* context = m_graphicsContext.GetContext();
            context->OMSetBlendState(m_resources.GetCommonStatesTK()->AlphaBlend(), nullptr, 0xFFFFFFFF);
            context->OMSetDepthStencilState(m_resources.GetCommonStatesTK()->DepthNone(), 0);
            postWorldPass->RenderPostWorld(context, m_frameConstants.GetWorldSceneData().view, m_frameConstants.GetWorldSceneData().projection);
            context->OMSetBlendState(m_resources.GetCommonStatesTK()->Opaque(), nullptr, 0xFFFFFFFF);
            context->OMSetDepthStencilState(m_resources.GetCommonStatesTK()->DepthDefault(), 0);
        }

        //------------------------------------------------------------
        // Overlay パス
        //
        // UIはスプライトも文字も同じRenderPass::Overlayへ積まれる。ここで
        // DrawCommand::sortOrderの昇順へ並べ直さないと「積んだ順＝Systemの登録順」が
        // 優先され、スプライトと文字の前後をsortOrderで指定できない
        // （ダメージ数値がスキル選択カードの上に出る類の不具合になる）。
        //
        // 同じsortOrder同士は積んだ順を保たなければならない。保たないと
        // EnTTのプール順の揺れがそのまま描画順のちらつきとして出る。
        // 比較関数に添字を混ぜているのは、そうすればキーが同値のとき添字の
        // 昇順＝積んだ順に落ちるためで、一時バッファを取るstd::stable_sortを
        // 使わずに安定ソートと同じ結果が得られる
        //------------------------------------------------------------
        m_frameConstants.UploadOverlay();

        m_overlayOrder.clear();
        for(u32 index = 0; index < static_cast<u32>(commands.size()); ++index) {
            if(commands[index].pass == RenderPass::Overlay)
                m_overlayOrder.push_back(index);
        }

        std::sort(m_overlayOrder.begin(), m_overlayOrder.end(), [&commands](u32 lhs, u32 rhs) {
            if(commands[lhs].sortOrder != commands[rhs].sortOrder)
                return commands[lhs].sortOrder < commands[rhs].sortOrder;
            return lhs < rhs;
        });

        for(u32 index : m_overlayOrder) {
            m_commandExecutor->Execute(commands[index], m_motionBlurEnabled, m_frameStats);
        }

        m_drawQueue.Clear();

        //------------------------------------------------------------
        // 次フレームの速度計算用に、今フレームのViewProjectionを退避する
        //
        // CameraSystemはdirty時しか行列を再計算しないため、
        // 「送られてきたタイミング」ではなく「フレームの末尾」で
        // 退避するのが確実。
        //------------------------------------------------------------
        m_frameConstants.EndFrame();

        //------------------------------------------------------------
        // モーションブラーの有効フラグはフレーム単位で消す
        //
        // 毎フレームMotionBlurSystemが再度trueにする前提にしておくと、
        // MotionBlurSystemを持たないシーンへ切り替えたときにフラグが
        // 立ちっぱなしで残る問題が起きない（無駄なフルスクリーンパスの防止）。
        //------------------------------------------------------------
        m_motionBlurEnabled = false;

        //------------------------------------------------------------
        // フォグの有効フラグもフレーム単位で消す（理由はモーションブラーと同じ）
        //------------------------------------------------------------
        m_fogEnabled = false;

        //------------------------------------------------------------
        // 環境パーティクルの有効フラグもフレーム単位で消す（理由は上と同じ）
        //------------------------------------------------------------
        m_ambientParticleEnabled = false;

        m_graphicsContext.EndFrame();
    }

    //------------------------------------------------------------
    //! @brief 描画領域のリサイズ
    //------------------------------------------------------------
    void Renderer::Resize(uint32_t width, uint32_t height) {
        //------------------------------------------------------------
        // GraphicsContext::ClearState() でパイプラインの状態が全て落ちるため、
        // 積み残しの描画コマンドは破棄しておく。
        // コマンドが指す Material / CBufferMaterial はキューが所有しているので
        // Clear() で一緒に破棄される。次フレームの Update で改めて積み直される。
        //------------------------------------------------------------
        m_drawQueue.Clear();
        m_debugDraw.Clear();

        if(!m_graphicsContext.Resize(width, height)) {
            Tsukino::Core::Log::Error("Renderer::Resize - failed to resize the swap chain. Rendering continues at the previous size.");
        }
    }

    //------------------------------------------------------------
    //! @brief クリアカラー設定
    //------------------------------------------------------------
    void Renderer::SetClearColor(float r, float g, float b, float a) {
        m_clearColor = {r, g, b, a};
    }

    //------------------------------------------------------------
    //! @brief モーションブラーパイプラインのセット
    //! @note  VSはトーンマッピングと共用（フルスクリーン三角形）なのでPSだけ作る
    //------------------------------------------------------------
    bool Renderer::SetMotionBlurPipeline(const Tsukino::Asset::ShaderAsset* ps) {
        if(!ps) {
            Tsukino::Core::Log::Error("Renderer::SetMotionBlurPipeline - shader is null.");
            return false;
        }

        ID3D11Device* device = m_graphicsContext.GetDevice();

        HRESULT hr = device->CreatePixelShader(ps->binary.data(), ps->binary.size(), nullptr, m_motionBlurPS.GetAddressOf());
        if(FAILED(hr)) {
            Tsukino::Core::Log::Error("Failed to create motion blur pixel shader.");
            return false;
        }

        m_hasMotionBlur = true;
        return true;
    }

    //------------------------------------------------------------
    //! @brief モーションブラーパラメータのセット
    //------------------------------------------------------------
    void Renderer::SetMotionBlurParameters(const CBufferMotionBlur& params) {
        m_motionBlurData = params;
    }

    //------------------------------------------------------------
    //! @brief フォグパイプラインのセット
    //------------------------------------------------------------
    bool Renderer::SetFogPipeline(const Tsukino::Asset::ShaderAsset* ps) {
        if(!ps) {
            Tsukino::Core::Log::Error("Renderer::SetFogPipeline - shader is null.");
            return false;
        }

        ID3D11Device* device = m_graphicsContext.GetDevice();

        HRESULT hr = device->CreatePixelShader(ps->binary.data(), ps->binary.size(), nullptr, m_fogPS.GetAddressOf());
        if(FAILED(hr)) {
            Tsukino::Core::Log::Error("Failed to create fog pixel shader.");
            return false;
        }

        m_hasFog = true;
        return true;
    }

    //------------------------------------------------------------
    //! @brief フォグパラメータのセット
    //------------------------------------------------------------
    void Renderer::SetFogParameters(const CBufferFog& params) {
        m_fogData = params;
    }

    //------------------------------------------------------------
    //! 環境パーティクルのパイプラインをセットします。
    //------------------------------------------------------------
    bool Renderer::SetAmbientParticlePipeline(const Tsukino::Asset::ShaderAsset* vs, const Tsukino::Asset::ShaderAsset* ps) {
        if(!vs || !ps) {
            Tsukino::Core::Log::Error("Renderer::SetAmbientParticlePipeline - shader is null.");
            return false;
        }

        ID3D11Device* device = m_graphicsContext.GetDevice();

        HRESULT hr = device->CreateVertexShader(vs->binary.data(), vs->binary.size(), nullptr, m_ambientParticleVS.GetAddressOf());
        if(FAILED(hr)) {
            Tsukino::Core::Log::Error("Failed to create ambient particle vertex shader.");
            return false;
        }

        hr = device->CreatePixelShader(ps->binary.data(), ps->binary.size(), nullptr, m_ambientParticlePS.GetAddressOf());
        if(FAILED(hr)) {
            Tsukino::Core::Log::Error("Failed to create ambient particle pixel shader.");
            return false;
        }

        m_hasAmbientParticle = true;
        return true;
    }

    //------------------------------------------------------------
    //! 環境パーティクルのパラメータをセットします。
    //------------------------------------------------------------
    void Renderer::SetAmbientParticleParameters(const CBufferAmbientParticle& params, u32 particleCount) {
        m_ambientParticleData  = params;
        m_ambientParticleCount = (particleCount < kMaxAmbientParticles) ? particleCount : kMaxAmbientParticles;
    }

    //------------------------------------------------------------
    //! @brief フォグパスの実行
    //! @note  深度(t13)だけを読み、HDRバッファへプリマルチプライのover合成で
    //!        書き込む。HDRをSRVとして読まないためRTVに張ったままでよく、
    //!        ポストプロセス用中間バッファを消費しない（モーションブラーと
    //!        取り合いにならない）。
    //------------------------------------------------------------
    void Renderer::ExecuteFogPass() {
        if(!m_fogEnabled || !m_hasFog || !m_fullscreenPass->IsValid() || !m_fogPS)
            return;

        ID3D11DeviceContext* context = m_graphicsContext.GetContext();

        //----------------------------------------------------------
        // HDRのみをRTVにバインド（深度をSRVとして読むためDSVは外す）
        //----------------------------------------------------------
        m_graphicsContext.BindHDRTargetOnly();

        //----------------------------------------------------------
        // 深度(t13)とポイントサンプラー(s9)をバインド
        //----------------------------------------------------------
        constexpr UINT            depthSRVSlot = static_cast<UINT>(SRVSlot::GBufferDepth);
        ID3D11ShaderResourceView* depthSRV     = m_graphicsContext.GetDepthSRV();
        context->PSSetShaderResources(depthSRVSlot, 1, &depthSRV);

        ID3D11SamplerState* pointClamp         = m_resources.GetSampler(Tsukino::GraphicsCommon::SamplerType::PointClamp);
        constexpr UINT      gbufferSamplerSlot = static_cast<UINT>(SamplerSlot::GBuffer);
        context->PSSetSamplers(gbufferSamplerSlot, 1, &pointClamp);

        //----------------------------------------------------------
        // Scene (b0) をバインド（invViewProj・cameraPos・lightDirを使う）
        //----------------------------------------------------------
        constexpr UINT sceneCBSlot = static_cast<UINT>(CBSlot::Scene);
        context->PSSetConstantBuffers(sceneCBSlot, 1, m_frameConstants.GetSceneBufferAddress());

        //----------------------------------------------------------
        // パラメータ (b9) を更新してバインド
        //----------------------------------------------------------
        context->UpdateSubresource(m_fogBuffer.Get(), 0, nullptr, &m_fogData, 0, 0);
        constexpr UINT fogCBSlot = static_cast<UINT>(CBSlot::Fog);
        context->PSSetConstantBuffers(fogCBSlot, 1, m_fogBuffer.GetAddressOf());

        //----------------------------------------------------------
        // シェーダーをセット（VSはTonemapと共用のフルスクリーン三角形用）
        //----------------------------------------------------------
        context->VSSetShader(m_fullscreenPass->GetVertexShader(), nullptr, 0);
        context->PSSetShader(m_fogPS.Get(), nullptr, 0);
        context->IASetInputLayout(nullptr);

        //----------------------------------------------------------
        // 深度なし・プリマルチプライアルファでover合成
        // （DirectXTKのAlphaBlendは ONE / INV_SRC_ALPHA なので、
        //   PSがfloat4(color * f, f)を返せばそのまま正しいoverになる）
        //----------------------------------------------------------
        context->OMSetDepthStencilState(m_resources.GetCommonStatesTK()->DepthNone(), 0);
        context->OMSetBlendState(m_resources.GetCommonStatesTK()->AlphaBlend(), nullptr, 0xFFFFFFFF);
        context->RSSetState(m_resources.GetCommonStatesTK()->CullNone());

        //----------------------------------------------------------
        // フルスクリーントライアングル描画
        //----------------------------------------------------------
        FullscreenPass::Draw(context);

        //----------------------------------------------------------
        // 後片付け：深度を次フレームDSVとして再バインドするため、
        // SRVのバインドを必ず解除する（LightingPass::Executeと同じ理由）。
        // ブレンドも不透明へ戻しておく
        //----------------------------------------------------------
        ID3D11ShaderResourceView* nullSRV = nullptr;
        context->PSSetShaderResources(depthSRVSlot, 1, &nullSRV);

        context->OMSetBlendState(m_resources.GetCommonStatesTK()->Opaque(), nullptr, 0xFFFFFFFF);
    }

    //------------------------------------------------------------
    //! 環境パーティクルパスを実行します。
    //------------------------------------------------------------
    void Renderer::ExecuteAmbientParticlePass() {
        if(!m_ambientParticleEnabled || !m_hasAmbientParticle || m_ambientParticleCount == 0)
            return;

        ID3D11DeviceContext* context = m_graphicsContext.GetContext();

        //----------------------------------------------------------
        // レンダーターゲットは切り替えない。
        // Render()のBindHDRRenderTarget()で貼ったHDR＋DSVがここまで生きており
        // （World/TransparentDepth/Transparentはターゲットを触らない）、
        // 深度をSRVとして読むこともないので同時バインドの問題も起きない
        //----------------------------------------------------------

        //----------------------------------------------------------
        // Scene (b0) をバインド（view・viewProj・cameraPosを頂点シェーダーで使う）
        //----------------------------------------------------------
        constexpr UINT sceneCBSlot = static_cast<UINT>(CBSlot::Scene);
        context->VSSetConstantBuffers(sceneCBSlot, 1, m_frameConstants.GetSceneBufferAddress());

        //----------------------------------------------------------
        // パラメータ (b10) を更新してバインド（頂点シェーダー専用）
        //----------------------------------------------------------
        context->UpdateSubresource(m_ambientParticleBuffer.Get(), 0, nullptr, &m_ambientParticleData, 0, 0);
        constexpr UINT particleCBSlot = static_cast<UINT>(CBSlot::AmbientParticle);
        context->VSSetConstantBuffers(particleCBSlot, 1, m_ambientParticleBuffer.GetAddressOf());

        //----------------------------------------------------------
        // シェーダーをセット（頂点バッファが無いので入力レイアウトも不要）
        //----------------------------------------------------------
        context->VSSetShader(m_ambientParticleVS.Get(), nullptr, 0);
        context->PSSetShader(m_ambientParticlePS.Get(), nullptr, 0);
        context->IASetInputLayout(nullptr);

        //----------------------------------------------------------
        // 深度テストあり・深度書き込みなし・加算合成
        //
        // DepthReadReverseZはGREATER_EQUALなのでこのエンジンのリバースZと
        // 一致する（DepthReadはLESS_EQUALなので使ってはいけない）。
        // 深度を書かないことで粒子どうしの前後関係を気にせずに済み、
        // 加算合成なので描画順のソートも不要になる
        //----------------------------------------------------------
        context->OMSetDepthStencilState(m_resources.GetCommonStatesTK()->DepthReadReverseZ(), 0);
        context->OMSetBlendState(m_resources.GetCommonStatesTK()->Additive(), nullptr, 0xFFFFFFFF);
        context->RSSetState(m_resources.GetCommonStatesTK()->CullNone());

        //----------------------------------------------------------
        // 頂点バッファなしで「1粒 = 三角形2枚」を一括描画
        //----------------------------------------------------------
        context->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
        context->IASetIndexBuffer(nullptr, DXGI_FORMAT_UNKNOWN, 0);
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        context->Draw(m_ambientParticleCount * 6, 0);

        //----------------------------------------------------------
        // ステートを戻す（Sky / Fogと同じ流儀）
        //----------------------------------------------------------
        context->OMSetDepthStencilState(m_resources.GetCommonStatesTK()->DepthDefault(), 0);
        context->OMSetBlendState(m_resources.GetCommonStatesTK()->Opaque(), nullptr, 0xFFFFFFFF);
    }

    //------------------------------------------------------------
    //! @brief モーションブラーパスの実行
    //! @note  HDRバッファ(t0)と速度バッファ(t15)を読み、ポストプロセス用
    //!        中間バッファへ書き出す。UIやエフェクトはトーンマップ後に
    //!        バックバッファへ直接描かれるため、ブラーの影響を受けない。
    //------------------------------------------------------------
    bool Renderer::ExecuteMotionBlurPass() {
        if(!m_motionBlurEnabled || !m_hasMotionBlur || !m_fullscreenPass->IsValid() || !m_motionBlurPS)
            return false;

        ID3D11DeviceContext* context = m_graphicsContext.GetContext();

        //----------------------------------------------------------
        // ポストプロセス用中間バッファへ切り替え
        // （HDRをSRVとして読むため、HDRをRTVに残したままにはできない）
        //----------------------------------------------------------
        m_graphicsContext.BindPostProcessTarget();

        //----------------------------------------------------------
        // シーンカラー(t0/s0)と速度バッファ(t15/s9)をバインド
        //----------------------------------------------------------
        ID3D11ShaderResourceView* hdrSRV = m_graphicsContext.GetHDRSRV();
        context->PSSetShaderResources(0, 1, &hdrSRV);

        ID3D11SamplerState* linearClamp = m_resources.GetSampler(Tsukino::GraphicsCommon::SamplerType::LinearClamp);
        context->PSSetSamplers(0, 1, &linearClamp);

        constexpr UINT            velocitySRVSlot = static_cast<UINT>(SRVSlot::GBufferVelocity);
        ID3D11ShaderResourceView* velocitySRV     = m_graphicsContext.GetGBufferSRV(5);
        context->PSSetShaderResources(velocitySRVSlot, 1, &velocitySRV);

        ID3D11SamplerState* pointClamp = m_resources.GetSampler(Tsukino::GraphicsCommon::SamplerType::PointClamp);
        constexpr UINT      gbufferSamplerSlot = static_cast<UINT>(SamplerSlot::GBuffer);
        context->PSSetSamplers(gbufferSamplerSlot, 1, &pointClamp);

        //----------------------------------------------------------
        // パラメータ (b8) を更新してバインド
        //----------------------------------------------------------
        context->UpdateSubresource(m_motionBlurBuffer.Get(), 0, nullptr, &m_motionBlurData, 0, 0);
        constexpr UINT motionBlurCBSlot = static_cast<UINT>(CBSlot::MotionBlur);
        context->PSSetConstantBuffers(motionBlurCBSlot, 1, m_motionBlurBuffer.GetAddressOf());

        //----------------------------------------------------------
        // シェーダーをセット（VSはTonemapと共用のフルスクリーン三角形用）
        //----------------------------------------------------------
        context->VSSetShader(m_fullscreenPass->GetVertexShader(), nullptr, 0);
        context->PSSetShader(m_motionBlurPS.Get(), nullptr, 0);
        context->IASetInputLayout(nullptr);

        //----------------------------------------------------------
        // 深度なし・ブレンドなし
        //----------------------------------------------------------
        context->OMSetDepthStencilState(m_resources.GetCommonStatesTK()->DepthNone(), 0);
        context->OMSetBlendState(m_resources.GetCommonStatesTK()->Opaque(), nullptr, 0xFFFFFFFF);
        context->RSSetState(m_resources.GetCommonStatesTK()->CullNone());

        //----------------------------------------------------------
        // フルスクリーントライアングル描画
        //----------------------------------------------------------
        FullscreenPass::Draw(context);

        //----------------------------------------------------------
        // 後片付け：次フレームでHDR/G-BufferをRTVとして再バインドするため、
        // SRVのバインドを必ず解除する（LightingPass::Executeと同じ理由）
        //----------------------------------------------------------
        ID3D11ShaderResourceView* nullSRV = nullptr;
        context->PSSetShaderResources(0, 1, &nullSRV);
        context->PSSetShaderResources(velocitySRVSlot, 1, &nullSRV);

        return true;
    }

}    // namespace Tsukino::Renderer
