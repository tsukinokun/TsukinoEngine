//----------------------------------------------------------------------------
//! @file   EfkRenderer.cpp
//! @brief  Effekseer のレンダラーの bgfx 版の実装
//----------------------------------------------------------------------------
#include "EfkRenderer.hpp"

#include <Tsukino/Renderer/Bgfx/GraphicsContext.hpp>
#include <Tsukino/Renderer/Bgfx/ShaderPrograms.hpp>

#include <Tsukino/Core/Log.hpp>

#include <EffekseerRendererCommon/EffekseerRenderer.CommonUtils.h>
#include <EffekseerRendererCommon/EffekseerRenderer.Renderer_Impl.h>
#include <EffekseerRendererCommon/EffekseerRenderer.RibbonRendererBase.h>
#include <EffekseerRendererCommon/EffekseerRenderer.RingRendererBase.h>
#include <EffekseerRendererCommon/EffekseerRenderer.SpriteRendererBase.h>
#include <EffekseerRendererCommon/EffekseerRenderer.TrackRendererBase.h>
#include <EffekseerRendererCommon/ModelLoader.h>
#include <EffekseerRendererCommon/TextureLoader.h>

#include <cstring>
#include <string>

// 名前空間 : Tsukino::Renderer::EffekseerBgfx
namespace Tsukino::Renderer::EffekseerBgfx {

    namespace {
        //! 頂点の定数（StandardRendererVertexBuffer）の vec4 の数（Shaders/bgfx/Effekseer/EfkSpriteVS.sh と揃える）
        constexpr u16 kSpriteVSVec4Count = 11;
        //! ピクセルの定数（PixelConstantBuffer）の vec4 の数（EfkPS.sh と揃える）
        constexpr u16 kPSVec4Count = 18;
        //! 歪みのピクセルの定数（PixelConstantBufferDistortion）の vec4 の数（EfkDistortionPS.sh と揃える）
        constexpr u16 kDistortionPSVec4Count = 8;

        static_assert(sizeof(EffekseerRenderer::StandardRendererVertexBuffer) == kSpriteVSVec4Count * 16,
                      "StandardRendererVertexBuffer must match u_efkSpriteVS in EfkSpriteVS.sh.");
        static_assert(sizeof(EffekseerRenderer::PixelConstantBuffer) == kPSVec4Count * 16, "PixelConstantBuffer must match u_efkPS in EfkPS.sh.");
        static_assert(sizeof(EffekseerRenderer::PixelConstantBufferDistortion) == kDistortionPSVec4Count * 16,
                      "PixelConstantBufferDistortion must match u_efkDistPS in EfkDistortionPS.sh.");

        //------------------------------------------------------------------------
        //! 独自マテリアル（.efkmat）を読まない読み込み部品です。読もうとしたら1回だけ警告を出します。
        //! @note  独自マテリアルは Effekseer がシェーダーを実行時に組み立てる仕組みで、
        //!        ビルド時にシェーダーを変換する bgfx 版では扱わない
        //------------------------------------------------------------------------
        class UnsupportedMaterialLoader final : public Effekseer::MaterialLoader {
        public:
            Effekseer::MaterialRef Load(const char16_t* /*path*/) override {
                Warn();
                return nullptr;
            }

            Effekseer::MaterialRef Load(const void* /*data*/, int32_t /*size*/, Effekseer::MaterialFileType /*fileType*/) override {
                Warn();
                return nullptr;
            }

        private:
            //! 1回だけ警告を出します。
            void Warn() {
                if(m_warned)
                    return;
                m_warned = true;
                Tsukino::Core::Log::Warn("Effekseer (bgfx): Custom materials (.efkmat) are not supported; nodes that use them are not drawn.");
            }

            bool m_warned = false;    // 警告を出したか
        };
    }    // namespace

    //------------------------------------------------------------------------
    //! シェーダーを作ります。
    //------------------------------------------------------------------------
    Shader::Shader(const char* vsName, const char* psName, const Effekseer::Backend::VertexLayoutRef& layout, const char* vsUniform,
                   u16 vsVec4Count, const char* psUniform, u16 psVec4Count)
        : m_vsData(static_cast<size_t>(vsVec4Count) * 4, 0.0f)
        , m_psData(static_cast<size_t>(psVec4Count) * 4, 0.0f) {
        m_program = LoadBuiltInBgfxProgram(vsName, psName);
        if(layout != nullptr)
            m_layout = static_cast<const VertexLayout*>(layout.Get())->GetLayout();
        m_vsUniform = bgfx::createUniform(vsUniform, bgfx::UniformType::Vec4, vsVec4Count);
        m_psUniform = bgfx::createUniform(psUniform, bgfx::UniformType::Vec4, psVec4Count);
    }

    //------------------------------------------------------------------------
    //! プログラムとユニフォームを破棄します。
    //------------------------------------------------------------------------
    Shader::~Shader() {
        if(!GraphicsContext::IsBgfxAlive())
            return;
        if(bgfx::isValid(m_program))
            bgfx::destroy(m_program);
        if(bgfx::isValid(m_vsUniform))
            bgfx::destroy(m_vsUniform);
        if(bgfx::isValid(m_psUniform))
            bgfx::destroy(m_psUniform);
    }

    //------------------------------------------------------------------------
    //! 頂点の定数の大きさを確かめます。
    //------------------------------------------------------------------------
    void Shader::SetVertexConstantBufferSize(int32_t size) {
        if(size > GetVertexConstantBufferSize())
            Tsukino::Core::Log::Error("Effekseer (bgfx): The vertex constants (" + std::to_string(size) + " bytes) do not fit the shader.");
    }

    //------------------------------------------------------------------------
    //! ピクセルの定数の大きさを確かめます。
    //------------------------------------------------------------------------
    void Shader::SetPixelConstantBufferSize(int32_t size) {
        if(size > GetPixelConstantBufferSize())
            Tsukino::Core::Log::Error("Effekseer (bgfx): The pixel constants (" + std::to_string(size) + " bytes) do not fit the shader.");
    }

    //------------------------------------------------------------------------
    //! 次の描画へ定数を渡します。
    //------------------------------------------------------------------------
    void Shader::ApplyUniforms() const {
        bgfx::setUniform(m_vsUniform, m_vsData.data(), static_cast<u16>(m_vsData.size() / 4));
        bgfx::setUniform(m_psUniform, m_psData.data(), static_cast<u16>(m_psData.size() / 4));
    }

    //------------------------------------------------------------------------
    //! 描画ステートを作ります。
    //------------------------------------------------------------------------
    RenderState::RenderState(bool reversedDepth)
        : m_reversedDepth(reversedDepth) {}

    //------------------------------------------------------------------------
    //! 次のステートを今のステートにします。
    //------------------------------------------------------------------------
    void RenderState::Update(bool /*forced*/) {
        active_ = next_;
    }

    //------------------------------------------------------------------------
    //! 今のステートを bgfx の描画ステートにします。
    //! @note  合成は Effekseer の DX11 版（RenderState.cpp。乗算済みアルファを使わないとき）と同じ。
    //!        アルファは描く先のアルファに使わない値なので、Effekseer の DX11 版の決まりをそのまま写す
    //------------------------------------------------------------------------
    u64 RenderState::GetBgfxState(bool lines) const {
        u64 state = BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A;

        //--------------------------------------------------------------------
        // 合成
        //--------------------------------------------------------------------
        switch(active_.AlphaBlend) {
            case Effekseer::AlphaBlendType::Opacity:
                state |= BGFX_STATE_BLEND_FUNC_SEPARATE(BGFX_STATE_BLEND_ONE, BGFX_STATE_BLEND_ZERO, BGFX_STATE_BLEND_ONE, BGFX_STATE_BLEND_ONE)
                         | BGFX_STATE_BLEND_EQUATION_SEPARATE(BGFX_STATE_BLEND_EQUATION_ADD, BGFX_STATE_BLEND_EQUATION_MAX);
                break;
            case Effekseer::AlphaBlendType::Blend:
                state |= BGFX_STATE_BLEND_FUNC_SEPARATE(BGFX_STATE_BLEND_SRC_ALPHA, BGFX_STATE_BLEND_INV_SRC_ALPHA, BGFX_STATE_BLEND_ONE,
                                                        BGFX_STATE_BLEND_ONE)
                         | BGFX_STATE_BLEND_EQUATION_SEPARATE(BGFX_STATE_BLEND_EQUATION_ADD, BGFX_STATE_BLEND_EQUATION_MAX);
                break;
            case Effekseer::AlphaBlendType::Add:
                state |= BGFX_STATE_BLEND_FUNC_SEPARATE(BGFX_STATE_BLEND_SRC_ALPHA, BGFX_STATE_BLEND_ONE, BGFX_STATE_BLEND_ONE, BGFX_STATE_BLEND_ONE)
                         | BGFX_STATE_BLEND_EQUATION_SEPARATE(BGFX_STATE_BLEND_EQUATION_ADD, BGFX_STATE_BLEND_EQUATION_MAX);
                break;
            case Effekseer::AlphaBlendType::Sub:
                state |= BGFX_STATE_BLEND_FUNC_SEPARATE(BGFX_STATE_BLEND_SRC_ALPHA, BGFX_STATE_BLEND_ONE, BGFX_STATE_BLEND_ZERO, BGFX_STATE_BLEND_ONE)
                         | BGFX_STATE_BLEND_EQUATION_SEPARATE(BGFX_STATE_BLEND_EQUATION_REVSUB, BGFX_STATE_BLEND_EQUATION_ADD);
                break;
            case Effekseer::AlphaBlendType::Mul:
                state |= BGFX_STATE_BLEND_FUNC_SEPARATE(BGFX_STATE_BLEND_ZERO, BGFX_STATE_BLEND_SRC_COLOR, BGFX_STATE_BLEND_ZERO, BGFX_STATE_BLEND_ONE)
                         | BGFX_STATE_BLEND_EQUATION_SEPARATE(BGFX_STATE_BLEND_EQUATION_ADD, BGFX_STATE_BLEND_EQUATION_ADD);
                break;
        }

        //--------------------------------------------------------------------
        // 深度。テストしないときは書かない（Direct3D と同じ）
        //--------------------------------------------------------------------
        if(active_.DepthTest) {
            state |= m_reversedDepth ? BGFX_STATE_DEPTH_TEST_GEQUAL : BGFX_STATE_DEPTH_TEST_LEQUAL;
            if(active_.DepthWrite)
                state |= BGFX_STATE_WRITE_Z;
        }

        //--------------------------------------------------------------------
        // カリング。Effekseer の Front は「表を描く」（裏を捨てる）。
        // リバース Z のときは反時計回りを表とする
        //--------------------------------------------------------------------
        const u64 cullBack  = m_reversedDepth ? BGFX_STATE_CULL_CW : BGFX_STATE_CULL_CCW;
        const u64 cullFront = m_reversedDepth ? BGFX_STATE_CULL_CCW : BGFX_STATE_CULL_CW;
        if(active_.CullingType == Effekseer::CullingType::Front)
            state |= cullBack;
        else if(active_.CullingType == Effekseer::CullingType::Back)
            state |= cullFront;

        if(lines)
            state |= BGFX_STATE_PT_LINES;
        return state;
    }

    //------------------------------------------------------------------------
    //! テクスチャの読み方を bgfx のサンプラーの指定にします。
    //! @note  線形のときもミップの間は補間しない（MIN_MAG_LINEAR_MIP_POINT）
    //------------------------------------------------------------------------
    u32 RenderState::GetSamplerFlags(int32_t slot) const {
        u32 flags = BGFX_SAMPLER_MIP_POINT;
        if(active_.TextureFilterTypes[slot] == Effekseer::TextureFilterType::Nearest)
            flags |= BGFX_SAMPLER_MIN_POINT | BGFX_SAMPLER_MAG_POINT;

        switch(active_.TextureWrapTypes[slot]) {
            case Effekseer::TextureWrapType::Clamp:  flags |= BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP; break;
            case Effekseer::TextureWrapType::Mirror: flags |= BGFX_SAMPLER_U_MIRROR | BGFX_SAMPLER_V_MIRROR; break;
            default:                                 break;
        }
        return flags;
    }

    //------------------------------------------------------------------------
    //! レンダラーを作ります。
    //------------------------------------------------------------------------
    RendererImplemented::RendererImplemented(int32_t squareMaxCount)
        : m_squareMaxCount(squareMaxCount) {
        m_samplers.fill(BGFX_INVALID_HANDLE);
        m_currentTextures.fill(BGFX_INVALID_HANDLE);
    }

    //------------------------------------------------------------------------
    //! シェーダーと共通部分を破棄します。
    //------------------------------------------------------------------------
    RendererImplemented::~RendererImplemented() {
        GetImpl()->DeleteProxyTextures(this);
        ES_SAFE_DELETE(m_distortingCallback);
        m_standardRenderer.reset();

        if(GraphicsContext::IsBgfxAlive()) {
            for(bgfx::UniformHandle& sampler : m_samplers) {
                if(bgfx::isValid(sampler))
                    bgfx::destroy(sampler);
            }
        }
    }

    //------------------------------------------------------------------------
    //! 描画デバイス・シェーダー・共通部分を用意します。
    //------------------------------------------------------------------------
    bool RendererImplemented::Initialize(bool reversedDepth) {
        m_graphicsDevice = Effekseer::MakeRefPtr<GraphicsDevice>();

        //--------------------------------------------------------------------
        // スプライトなどの頂点を積む入れ物。1回にまとめて描ける四角の数ぶん
        //--------------------------------------------------------------------
        GetImpl()->InternalVertexBuffer = std::make_shared<EffekseerRenderer::VertexBufferRing>(
            m_graphicsDevice, EffekseerRenderer::GetMaximumVertexSizeInAllTypes() * m_squareMaxCount * 4, 1);
        if(!GetImpl()->InternalVertexBuffer->GetIsValid()) {
            GetImpl()->InternalVertexBuffer = nullptr;
            return false;
        }

        // 四角が 16384 個を超えると 16bit では番号が足りないので、32bit のインデックスにする
        if(!EffekseerRenderer::GenerateIndexDataStride<int32_t>(m_graphicsDevice, m_squareMaxCount, m_indexBuffer, m_wireframeIndexBuffer))
            return false;

        m_renderState = std::make_unique<RenderState>(reversedDepth);

        //--------------------------------------------------------------------
        // シェーダー（スプライト用。モデル用は ModelRenderer が作る）
        //--------------------------------------------------------------------
        using EffekseerRenderer::RendererShaderType;
        const auto layout = [this](RendererShaderType type) { return EffekseerRenderer::GetVertexLayout(m_graphicsDevice, type); };

        auto unlit        = std::make_unique<Shader>("Effekseer/SpriteUnlit.vs", "Effekseer/Unlit.ps", layout(RendererShaderType::Unlit), "u_efkSpriteVS",
                                              kSpriteVSVec4Count, "u_efkPS", kPSVec4Count);
        auto lit          = std::make_unique<Shader>("Effekseer/SpriteLit.vs", "Effekseer/Lit.ps", layout(RendererShaderType::Lit), "u_efkSpriteVS",
                                            kSpriteVSVec4Count, "u_efkPS", kPSVec4Count);
        auto distortion   = std::make_unique<Shader>("Effekseer/SpriteDistortion.vs", "Effekseer/Distortion.ps", layout(RendererShaderType::BackDistortion),
                                                   "u_efkSpriteVS", kSpriteVSVec4Count, "u_efkDistPS", kDistortionPSVec4Count);
        auto adUnlit      = std::make_unique<Shader>("Effekseer/SpriteAdUnlit.vs", "Effekseer/AdUnlit.ps", layout(RendererShaderType::AdvancedUnlit),
                                                "u_efkSpriteVS", kSpriteVSVec4Count, "u_efkPS", kPSVec4Count);
        auto adLit        = std::make_unique<Shader>("Effekseer/SpriteAdLit.vs", "Effekseer/AdLit.ps", layout(RendererShaderType::AdvancedLit),
                                              "u_efkSpriteVS", kSpriteVSVec4Count, "u_efkPS", kPSVec4Count);
        auto adDistortion = std::make_unique<Shader>("Effekseer/SpriteAdDistortion.vs", "Effekseer/AdDistortion.ps",
                                                     layout(RendererShaderType::AdvancedBackDistortion), "u_efkSpriteVS", kSpriteVSVec4Count,
                                                     "u_efkDistPS", kDistortionPSVec4Count);
        for(const Shader* shader : {unlit.get(), lit.get(), distortion.get(), adUnlit.get(), adLit.get(), adDistortion.get()}) {
            if(!shader->IsValid()) {
                Tsukino::Core::Log::Error("Effekseer (bgfx): The sprite shaders could not be loaded; effects are not drawn.");
                return false;
            }
        }

        GetImpl()->ShaderUnlit        = std::move(unlit);
        GetImpl()->ShaderLit          = std::move(lit);
        GetImpl()->ShaderDistortion   = std::move(distortion);
        GetImpl()->ShaderAdUnlit      = std::move(adUnlit);
        GetImpl()->ShaderAdLit        = std::move(adLit);
        GetImpl()->ShaderAdDistortion = std::move(adDistortion);

        //--------------------------------------------------------------------
        // テクスチャのサンプラー（シェーダーは s_efkTex0〜7 をステージ 0〜7 に置く）
        //--------------------------------------------------------------------
        for(size_t i = 0; i < m_samplers.size(); ++i)
            m_samplers[i] = bgfx::createUniform(("s_efkTex" + std::to_string(i)).c_str(), bgfx::UniformType::Sampler);

        m_standardRenderer = std::make_unique<EffekseerRenderer::StandardRenderer<RendererImplemented, Shader>>(this);

        GetImpl()->CreateProxyTextures(this);
        GetImpl()->isSoftParticleEnabled = true;
        GetImpl()->isDepthReversed       = reversedDepth;
        return true;
    }

    //------------------------------------------------------------------------
    //! 描き始めます。
    //------------------------------------------------------------------------
    bool RendererImplemented::BeginRendering() {
        GetImpl()->CalculateCameraProjectionMatrix();

        m_renderState->GetActiveState().Reset();
        m_renderState->Update(true);

        m_standardRenderer->ResetAndRenderingIfRequired();
        return true;
    }

    //------------------------------------------------------------------------
    //! 描き終わります（まだ描いていない分を描く）。
    //------------------------------------------------------------------------
    bool RendererImplemented::EndRendering() {
        m_standardRenderer->ResetAndRenderingIfRequired();

        m_currentShader = nullptr;
        m_currentVertexBuffer.Reset();
        m_currentIndexBuffer.Reset();
        m_currentTextureCount = 0;
        return true;
    }

    //------------------------------------------------------------------------
    //! スプライト・リボン・リング・軌跡・モデルのレンダラーを作ります。
    //------------------------------------------------------------------------
    ::Effekseer::SpriteRendererRef RendererImplemented::CreateSpriteRenderer() {
        return ::Effekseer::SpriteRendererRef(new ::EffekseerRenderer::SpriteRendererBase<RendererImplemented, false>(this));
    }

    ::Effekseer::RibbonRendererRef RendererImplemented::CreateRibbonRenderer() {
        return ::Effekseer::RibbonRendererRef(new ::EffekseerRenderer::RibbonRendererBase<RendererImplemented, false>(this));
    }

    ::Effekseer::RingRendererRef RendererImplemented::CreateRingRenderer() {
        return ::Effekseer::RingRendererRef(new ::EffekseerRenderer::RingRendererBase<RendererImplemented, false>(this));
    }

    ::Effekseer::ModelRendererRef RendererImplemented::CreateModelRenderer() {
        return ModelRenderer::Create(RendererImplementedRef::FromPinned(this));
    }

    ::Effekseer::TrackRendererRef RendererImplemented::CreateTrackRenderer() {
        return ::Effekseer::TrackRendererRef(new ::EffekseerRenderer::TrackRendererBase<RendererImplemented, false>(this));
    }

    //------------------------------------------------------------------------
    //! 画像・モデル・マテリアルの読み込み部品を作ります。
    //------------------------------------------------------------------------
    ::Effekseer::TextureLoaderRef RendererImplemented::CreateTextureLoader(::Effekseer::FileInterfaceRef fileInterface) {
        return ::EffekseerRenderer::CreateTextureLoader(m_graphicsDevice, fileInterface, ::Effekseer::ColorSpaceType::Gamma);
    }

    ::Effekseer::ModelLoaderRef RendererImplemented::CreateModelLoader(::Effekseer::FileInterfaceRef fileInterface) {
        return ::Effekseer::MakeRefPtr<EffekseerRenderer::ModelLoader>(m_graphicsDevice, fileInterface);
    }

    ::Effekseer::MaterialLoaderRef RendererImplemented::CreateMaterialLoader(::Effekseer::FileInterfaceRef /*fileInterface*/) {
        return ::Effekseer::MakeRefPtr<UnsupportedMaterialLoader>();
    }

    //------------------------------------------------------------------------
    //! 描画ステートを最初の状態へ戻します。
    //------------------------------------------------------------------------
    void RendererImplemented::ResetRenderState() {
        m_renderState->GetActiveState().Reset();
        m_renderState->Update(true);
    }

    //------------------------------------------------------------------------
    //! 歪みの前に呼ぶものを設定します（前のものは破棄する）。
    //------------------------------------------------------------------------
    void RendererImplemented::SetDistortingCallback(EffekseerRenderer::DistortingCallback* callback) {
        ES_SAFE_DELETE(m_distortingCallback);
        m_distortingCallback = callback;
    }

    //------------------------------------------------------------------------
    //! 四角を並べるインデックスを返します（ワイヤーフレームのときは輪郭）。
    //------------------------------------------------------------------------
    Effekseer::Backend::IndexBufferRef RendererImplemented::GetIndexBuffer() {
        return GetRenderMode() == ::Effekseer::RenderMode::Wireframe ? m_wireframeIndexBuffer : m_indexBuffer;
    }

    //------------------------------------------------------------------------
    //! 次の描画の頂点バッファを設定します。
    //------------------------------------------------------------------------
    void RendererImplemented::SetVertexBuffer(const Effekseer::Backend::VertexBufferRef& vertexBuffer, int32_t stride) {
        m_currentVertexBuffer = vertexBuffer;
        m_currentStride       = stride;
    }

    //------------------------------------------------------------------------
    //! 次の描画のインデックスバッファを設定します。
    //------------------------------------------------------------------------
    void RendererImplemented::SetIndexBuffer(const Effekseer::Backend::IndexBufferRef& indexBuffer) {
        m_currentIndexBuffer = indexBuffer;
    }

    //------------------------------------------------------------------------
    //! 頂点形式を設定します（頂点形式はシェーダーが持っているので、描くときに使う）。
    //------------------------------------------------------------------------
    void RendererImplemented::SetLayout(Shader* /*shader*/) {}

    //------------------------------------------------------------------------
    //! 四角を並べて描きます。
    //------------------------------------------------------------------------
    void RendererImplemented::DrawSprites(int32_t spriteCount, int32_t vertexOffset) {
        GetImpl()->drawcallCount++;
        GetImpl()->drawvertexCount += spriteCount * 4;

        const u32 indexPerSprite = GetRenderMode() == ::Effekseer::RenderMode::Wireframe ? 8 : 6;
        Submit(static_cast<u32>(vertexOffset), static_cast<u32>(spriteCount) * 4, static_cast<u32>(spriteCount) * indexPerSprite, 1);
    }

    //------------------------------------------------------------------------
    //! 頂点とインデックスをそのまま描きます（モデル）。
    //------------------------------------------------------------------------
    void RendererImplemented::DrawPolygon(int32_t vertexCount, int32_t indexCount) {
        GetImpl()->drawcallCount++;
        GetImpl()->drawvertexCount += vertexCount;
        Submit(0, static_cast<u32>(vertexCount), static_cast<u32>(indexCount), 1);
    }

    //------------------------------------------------------------------------
    //! 同じモデルを並べて描きます（インスタンス描画）。
    //------------------------------------------------------------------------
    void RendererImplemented::DrawPolygonInstanced(int32_t vertexCount, int32_t indexCount, int32_t instanceCount) {
        GetImpl()->drawcallCount++;
        GetImpl()->drawvertexCount += vertexCount * instanceCount;
        Submit(0, static_cast<u32>(vertexCount), static_cast<u32>(indexCount), static_cast<u32>(instanceCount));
    }

    //------------------------------------------------------------------------
    //! 使うシェーダーを設定します。
    //------------------------------------------------------------------------
    void RendererImplemented::BeginShader(Shader* shader) {
        m_currentShader = shader;
    }

    //------------------------------------------------------------------------
    //! シェーダーを使い終わります。
    //------------------------------------------------------------------------
    void RendererImplemented::EndShader(Shader* /*shader*/) {
        m_currentShader = nullptr;
    }

    //------------------------------------------------------------------------
    //! 頂点の定数の入れ物へ書きます。
    //------------------------------------------------------------------------
    void RendererImplemented::SetVertexBufferToShader(const void* data, int32_t size, int32_t dstOffset) {
        if(!m_currentShader || dstOffset < 0 || size + dstOffset > m_currentShader->GetVertexConstantBufferSize())
            return;
        std::memcpy(static_cast<u8*>(m_currentShader->GetVertexConstantBuffer()) + dstOffset, data, static_cast<size_t>(size));
    }

    //------------------------------------------------------------------------
    //! ピクセルの定数の入れ物へ書きます。
    //------------------------------------------------------------------------
    void RendererImplemented::SetPixelBufferToShader(const void* data, int32_t size, int32_t dstOffset) {
        if(!m_currentShader || dstOffset < 0 || size + dstOffset > m_currentShader->GetPixelConstantBufferSize())
            return;
        std::memcpy(static_cast<u8*>(m_currentShader->GetPixelConstantBuffer()) + dstOffset, data, static_cast<size_t>(size));
    }

    //------------------------------------------------------------------------
    //! 次の描画のテクスチャを設定します。
    //------------------------------------------------------------------------
    void RendererImplemented::SetTextures(Shader* /*shader*/, ::Effekseer::Backend::TextureRef* textures, int32_t count) {
        m_currentTextureCount = std::min(count, static_cast<int32_t>(m_currentTextures.size()));
        for(int32_t i = 0; i < m_currentTextureCount; ++i) {
            m_currentTextures[i] =
                textures[i] != nullptr ? static_cast<const Texture*>(textures[i].Get())->GetHandle() : bgfx::TextureHandle BGFX_INVALID_HANDLE;
        }
    }

    //------------------------------------------------------------------------
    //! 今のシェーダー・バッファ・テクスチャ・ステートで1回描きます。
    //------------------------------------------------------------------------
    void RendererImplemented::Submit(u32 firstVertex, u32 vertexCount, u32 indexCount, u32 instanceCount) {
        if(!m_currentShader || !m_currentShader->IsValid() || m_currentVertexBuffer == nullptr || m_currentIndexBuffer == nullptr)
            return;

        // 頂点の大きさは、共通部分が積んだ頂点とシェーダーの頂点形式で一致しているはず
        const bgfx::VertexLayout& layout = m_currentShader->GetLayout();
        if(static_cast<int32_t>(layout.getStride()) != m_currentStride) {
            WarnOnce(m_warnedLayout, "Effekseer (bgfx): The vertex size does not match the shader; some effects are not drawn.");
            return;
        }

        if(!static_cast<VertexBuffer*>(m_currentVertexBuffer.Get())->Bind(firstVertex, vertexCount, layout)) {
            WarnOnce(m_warnedVertices, "Effekseer (bgfx): The vertices did not fit the transient vertex buffer; some effects were not drawn.");
            return;
        }
        if(!static_cast<IndexBuffer*>(m_currentIndexBuffer.Get())->Bind(indexCount)) {
            bgfx::discard();
            return;
        }

        if(instanceCount > 1)
            bgfx::setInstanceCount(instanceCount);

        m_currentShader->ApplyUniforms();

        for(int32_t i = 0; i < m_currentTextureCount; ++i) {
            if(bgfx::isValid(m_currentTextures[i]))
                bgfx::setTexture(static_cast<u8>(i), m_samplers[i], m_currentTextures[i], m_renderState->GetSamplerFlags(i));
        }

        bgfx::setState(m_renderState->GetBgfxState(GetRenderMode() == ::Effekseer::RenderMode::Wireframe));
        bgfx::submit(m_view, m_currentShader->GetProgram());
    }

    //------------------------------------------------------------------------
    //! 1回だけ警告を出します。
    //------------------------------------------------------------------------
    void RendererImplemented::WarnOnce(bool& warned, const char* message) {
        if(warned)
            return;
        warned = true;
        Tsukino::Core::Log::Warn(message);
    }

}    // namespace Tsukino::Renderer::EffekseerBgfx
