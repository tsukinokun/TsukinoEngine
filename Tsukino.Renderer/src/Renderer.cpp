//------------------------------------------------------------
//! @file   Renderer.cpp
//! @brief  レンダラークラスの実装
//! @author 山﨑愛
//------------------------------------------------------------
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
        if(!m_frameConstants.Initialize(m_graphicsContext, ShadowPass::kMapSize, ShadowPass::kDepthRange))
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
        // モーションブラー・フォグ・環境パーティクルの作成。
        // 演出用の任意機能なので、失敗しても描画自体は続行する
        // （そのパスが何もしなくなるだけで済む）
        //------------------------------------------------------------
        if(!m_motionBlurPass.Initialize(m_graphicsContext, m_resources, m_frameConstants, *m_fullscreenPass, shaders.motionBlurPS)) {
            Tsukino::Core::Log::Error("Renderer: Motion blur is disabled because its resources could not be created.");
        }

        if(!m_fogPass.Initialize(m_graphicsContext, m_resources, m_frameConstants, *m_fullscreenPass, shaders.fogPS)) {
            Tsukino::Core::Log::Error("Renderer: Fog is disabled because its resources could not be created.");
        }

        if(!m_ambientParticlePass.Initialize(m_graphicsContext, m_resources, m_frameConstants, shaders.ambientParticleVS, shaders.ambientParticlePS)) {
            Tsukino::Core::Log::Error("Renderer: Ambient particles are disabled because their resources could not be created.");
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
    //! @brief 描画処理
    //------------------------------------------------------------
    void Renderer::Render(IPostWorldPass* postWorldPass) {
        m_graphicsContext.BeginFrame(m_clearColor[0], m_clearColor[1], m_clearColor[2], m_clearColor[3]);

        const auto& commands = m_drawQueue.GetCommands();

        //------------------------------------------------------------
        // 今フレームの描画統計をリセットする（負荷調査用）
        // 実際の加算は DrawCommandExecutor が DrawIndexed の直後に行うため、
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
            m_commandExecutor->Execute(cmd, m_motionBlurPass.IsEnabled(), m_frameStats);
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
            m_commandExecutor->Execute(cmd, m_motionBlurPass.IsEnabled(), m_frameStats);
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
            m_commandExecutor->Execute(cmd, m_motionBlurPass.IsEnabled(), m_frameStats);
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
            m_commandExecutor->Execute(cmd, m_motionBlurPass.IsEnabled(), m_frameStats);
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
        m_ambientParticlePass.Execute();

        //------------------------------------------------------------
        // フォグパス（HDRバッファへ直接over合成）
        //
        // 不透明・空・半透明・水面がすべて描き終わったここで掛けることで、
        // 3Dの絵すべてにフォグが乗る。UIとエフェクトはトーンマップ後に
        // バックバッファへ直接描かれるため影響を受けない。
        //------------------------------------------------------------
        m_fogPass.Execute();

        //------------------------------------------------------------
        // モーションブラーパス（HDR → ポストプロセス用中間バッファ）
        //
        // 3Dの描画がすべて終わり、UI/エフェクトが乗る前のここで掛ける。
        // 無効なら何もせず false を返すので、その場合はHDRをそのまま
        // トーンマップへ渡す。
        //------------------------------------------------------------
        const bool motionBlurred = m_motionBlurPass.Execute();

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
            m_commandExecutor->Execute(commands[index], m_motionBlurPass.IsEnabled(), m_frameStats);
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
        // モーションブラー・フォグ・環境パーティクルの有効フラグはフレーム単位で消す。
        // 毎フレーム担当のSystemが再度trueにする前提にしておくと、そのSystemを
        // 持たないシーンへ切り替えたときにフラグが立ちっぱなしで残らない
        //------------------------------------------------------------
        m_motionBlurPass.EndFrame();
        m_fogPass.EndFrame();
        m_ambientParticlePass.EndFrame();

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
    //! @brief シャドウパスのカリングの有無を設定
    //------------------------------------------------------------
    void Renderer::SetShadowCullingEnabled(bool enabled) {
        if(m_shadowPass)
            m_shadowPass->SetCullingEnabled(enabled);
    }

}    // namespace Tsukino::Renderer
