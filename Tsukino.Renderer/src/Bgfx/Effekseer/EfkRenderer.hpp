//----------------------------------------------------------------------------
//! @file   EfkRenderer.hpp
//! @brief  Effekseer のレンダラー（EffekseerRenderer::Renderer）の bgfx 版
//! @detail スプライト・リボン・リング・軌跡・モデルを、Effekseer の共通部分（StandardRenderer・
//!         ModelRendererBase）が作る頂点と定数のまま、bgfx の描画として出します。
//!         作りは Effekseer の DX11 版（EffekseerRendererDX11 の RendererImplemented）と同じで、
//!         共通部分のテンプレートが呼ぶ関数（BeginShader・SetTextures・DrawSprites など）をそろえています。
//!         Effekseer の DX11 版と違うところ:
//!         - bgfx のユニフォームと描画ステートは描画ごとなので、描く瞬間（Draw*）にまとめて渡す
//!         - 描く先は Renderer が決める bgfx のビュー（SetView）。ビューの描く先と深度も Renderer が用意する
//!         - シェーダーはビルド時に変換したもの（Tsukino.BuiltIn/Assets/Shaders/bgfx/Effekseer）
//!         - GPU パーティクルと独自マテリアル（.efkmat）は描かない（マテリアルは読み込み時に警告を出す）
//----------------------------------------------------------------------------
#pragma once
#include "EfkGraphicsDevice.hpp"

#include <Tsukino/Core/typedef.hpp>

#include <Effekseer.h>
#include <EffekseerRendererCommon/EffekseerRenderer.ModelRendererBase.h>
#include <EffekseerRendererCommon/EffekseerRenderer.RenderStateBase.h>
#include <EffekseerRendererCommon/EffekseerRenderer.Renderer.h>
#include <EffekseerRendererCommon/EffekseerRenderer.ShaderBase.h>
#include <EffekseerRendererCommon/EffekseerRenderer.StandardRenderer.h>

#include <bgfx/bgfx.h>

#include <array>
#include <vector>

// 名前空間 : Tsukino::Renderer::EffekseerBgfx
namespace Tsukino::Renderer::EffekseerBgfx {

    //------------------------------------------------------------------------
    //! シェーダー（bgfx のプログラム1つと、頂点・ピクセルの定数の入れ物）です。
    //! @note  定数は vec4 の配列のユニフォーム1つずつで送る（エンジンの UniformBlock と同じやり方）。
    //!        共通部分は SetVertexBufferToShader などで入れ物へ書くだけで、送るのは描くとき
    //------------------------------------------------------------------------
    class Shader final : public EffekseerRenderer::ShaderBase {
    public:
        //! 作ります。
        //! @param  [in] vsName      頂点シェーダーの名前（"Effekseer/SpriteUnlit.vs" のように、組み込みのシェーダーのフォルダからの名前）
        //! @param  [in] psName      ピクセルシェーダーの名前
        //! @param  [in] layout      頂点形式
        //! @param  [in] vsUniform   頂点の定数のユニフォームの名前
        //! @param  [in] vsVec4Count 頂点の定数の vec4 の数（シェーダーの配列の長さ）
        //! @param  [in] psUniform   ピクセルの定数のユニフォームの名前
        //! @param  [in] psVec4Count ピクセルの定数の vec4 の数
        Shader(const char* vsName, const char* psName, const Effekseer::Backend::VertexLayoutRef& layout, const char* vsUniform, u16 vsVec4Count,
               const char* psUniform, u16 psVec4Count);

        //! プログラムとユニフォームを破棄します。
        ~Shader() override;

        Shader(const Shader&)            = delete;
        Shader& operator=(const Shader&) = delete;

        //! 使える状態かを返します。
        [[nodiscard]]
        bool IsValid() const noexcept {
            return bgfx::isValid(m_program) && m_layout.getStride() > 0;
        }

        //! 頂点の定数の大きさを確かめます（入れ物はシェーダーの配列の長さで作ってある）。
        void SetVertexConstantBufferSize(int32_t size) override;

        //! ピクセルの定数の大きさを確かめます。
        void SetPixelConstantBufferSize(int32_t size) override;

        //! 頂点の定数の入れ物を返します。
        void* GetVertexConstantBuffer() override {
            return m_vsData.data();
        }

        //! ピクセルの定数の入れ物を返します。
        void* GetPixelConstantBuffer() override {
            return m_psData.data();
        }

        //! 何もしません（定数は描くときに ApplyUniforms() で送る）。
        void SetConstantBuffer() override {}

        //! 頂点の定数の入れ物のバイト数を返します。
        [[nodiscard]]
        int32_t GetVertexConstantBufferSize() const noexcept {
            return static_cast<int32_t>(m_vsData.size() * sizeof(float));
        }

        //! ピクセルの定数の入れ物のバイト数を返します。
        [[nodiscard]]
        int32_t GetPixelConstantBufferSize() const noexcept {
            return static_cast<int32_t>(m_psData.size() * sizeof(float));
        }

        //! 次の描画へ定数を渡します。
        void ApplyUniforms() const;

        //! プログラムを返します。
        [[nodiscard]]
        bgfx::ProgramHandle GetProgram() const noexcept {
            return m_program;
        }

        //! 頂点レイアウトを返します。
        [[nodiscard]]
        const bgfx::VertexLayout& GetLayout() const noexcept {
            return m_layout;
        }

    private:
        bgfx::ProgramHandle m_program   = BGFX_INVALID_HANDLE;    // プログラム
        bgfx::VertexLayout  m_layout;                             // 頂点レイアウト
        bgfx::UniformHandle m_vsUniform = BGFX_INVALID_HANDLE;    // 頂点の定数
        bgfx::UniformHandle m_psUniform = BGFX_INVALID_HANDLE;    // ピクセルの定数
        std::vector<float>  m_vsData;                             // 頂点の定数の入れ物（vec4 × 数）
        std::vector<float>  m_psData;                             // ピクセルの定数の入れ物
    };

    //------------------------------------------------------------------------
    //! 描画ステート（合成・深度・カリング・テクスチャの読み方）です。
    //! @note  bgfx のステートは描画ごとなので、Update() は次のステートを今のステートにするだけで、
    //!        描くときに GetBgfxState() と GetSamplerFlags() で bgfx の形にする
    //------------------------------------------------------------------------
    class RenderState final : public EffekseerRenderer::RenderStateBase {
    public:
        //! 作ります。
        //! @param  [in] reversedDepth 深度がリバース Z（手前ほど大きい）か
        explicit RenderState(bool reversedDepth);

        //! 次のステートを今のステートにします。
        void Update(bool forced) override;

        //! 今のステートを bgfx の描画ステートにします。
        //! @param  [in] lines 線で描くか（ワイヤーフレーム）
        //! @return BGFX_STATE_*
        [[nodiscard]]
        u64 GetBgfxState(bool lines) const;

        //! テクスチャの読み方を bgfx のサンプラーの指定にします。
        //! @param  [in] slot テクスチャの番号
        //! @return BGFX_SAMPLER_*
        [[nodiscard]]
        u32 GetSamplerFlags(int32_t slot) const;

    private:
        bool m_reversedDepth = true;    // 深度がリバース Z か
    };

    class RendererImplemented;
    using RendererImplementedRef = Effekseer::RefPtr<RendererImplemented>;

    //------------------------------------------------------------------------
    //! Effekseer のレンダラーの bgfx 版です。
    //------------------------------------------------------------------------
    class RendererImplemented final : public EffekseerRenderer::Renderer, public Effekseer::ReferenceObject {
    public:
        //! 作ります。
        //! @param  [in] squareMaxCount 1回にまとめて描ける四角の数
        explicit RendererImplemented(int32_t squareMaxCount);

        //! シェーダーと共通部分を破棄します。
        ~RendererImplemented() override;

        //! 描画デバイス・シェーダー・共通部分を用意します。
        //! @param  [in] reversedDepth 深度がリバース Z か
        //! @return 用意できたら true
        [[nodiscard]]
        bool Initialize(bool reversedDepth);

        //! 描く先のビューを設定します（Renderer が毎フレーム、描く前に呼ぶ）。
        //! @param  [in] view ビュー
        void SetView(bgfx::ViewId view) noexcept {
            m_view = view;
        }

        //! bgfx 版の描画デバイスを返します。
        [[nodiscard]]
        GraphicsDevice* GetBgfxGraphicsDevice() const noexcept {
            return m_graphicsDevice.Get();
        }

        //------------------------------------------------------------------------
        // EffekseerRenderer::Renderer
        //------------------------------------------------------------------------
        void OnLostDevice() override {}
        void OnResetDevice() override {}
        void SetRestorationOfStatesFlag(bool /*flag*/) override {}
        bool BeginRendering() override;
        bool EndRendering() override;
        int32_t GetSquareMaxCount() const override {
            return m_squareMaxCount;
        }
        ::Effekseer::SpriteRendererRef CreateSpriteRenderer() override;
        ::Effekseer::RibbonRendererRef CreateRibbonRenderer() override;
        ::Effekseer::RingRendererRef   CreateRingRenderer() override;
        ::Effekseer::ModelRendererRef  CreateModelRenderer() override;
        ::Effekseer::TrackRendererRef  CreateTrackRenderer() override;
        ::Effekseer::TextureLoaderRef  CreateTextureLoader(::Effekseer::FileInterfaceRef fileInterface = nullptr) override;
        ::Effekseer::ModelLoaderRef    CreateModelLoader(::Effekseer::FileInterfaceRef fileInterface = nullptr) override;
        ::Effekseer::MaterialLoaderRef CreateMaterialLoader(::Effekseer::FileInterfaceRef fileInterface = nullptr) override;
        void                           ResetRenderState() override;
        EffekseerRenderer::DistortingCallback* GetDistortingCallback() override {
            return m_distortingCallback;
        }
        void                                    SetDistortingCallback(EffekseerRenderer::DistortingCallback* callback) override;
        Effekseer::Backend::GraphicsDeviceRef GetGraphicsDevice() const override {
            return m_graphicsDevice;
        }

        //------------------------------------------------------------------------
        // 共通部分のテンプレート（StandardRenderer・ModelRendererBase など）が呼ぶ関数
        //------------------------------------------------------------------------
        EffekseerRenderer::RenderStateBase* GetRenderState() {
            return m_renderState.get();
        }
        Effekseer::Backend::IndexBufferRef GetIndexBuffer();
        EffekseerRenderer::StandardRenderer<RendererImplemented, Shader>* GetStandardRenderer() {
            return m_standardRenderer.get();
        }
        void SetVertexBuffer(const Effekseer::Backend::VertexBufferRef& vertexBuffer, int32_t stride);
        void SetIndexBuffer(const Effekseer::Backend::IndexBufferRef& indexBuffer);
        void SetLayout(Shader* shader);
        void DrawSprites(int32_t spriteCount, int32_t vertexOffset);
        void DrawPolygon(int32_t vertexCount, int32_t indexCount);
        void DrawPolygonInstanced(int32_t vertexCount, int32_t indexCount, int32_t instanceCount);
        void BeginShader(Shader* shader);
        void EndShader(Shader* shader);
        void SetVertexBufferToShader(const void* data, int32_t size, int32_t dstOffset);
        void SetPixelBufferToShader(const void* data, int32_t size, int32_t dstOffset);
        void SetTextures(Shader* shader, ::Effekseer::Backend::TextureRef* textures, int32_t count);

        //------------------------------------------------------------------------
        // Effekseer::IReference
        //------------------------------------------------------------------------
        int GetRef() override {
            return ::Effekseer::ReferenceObject::GetRef();
        }
        int AddRef() override {
            return ::Effekseer::ReferenceObject::AddRef();
        }
        int Release() override {
            return ::Effekseer::ReferenceObject::Release();
        }

    private:
        //! 今のシェーダー・バッファ・テクスチャ・ステートで1回描きます。
        //! @param  [in] firstVertex   先頭の頂点
        //! @param  [in] vertexCount   頂点の数
        //! @param  [in] indexCount    インデックスの数
        //! @param  [in] instanceCount インスタンスの数（1 なら普通の描画）
        void Submit(u32 firstVertex, u32 vertexCount, u32 indexCount, u32 instanceCount);

        //! 1回だけ警告を出します。
        //! @param  [in,out] warned  出したかの印
        //! @param  [in]     message 警告
        static void WarnOnce(bool& warned, const char* message);

        Effekseer::RefPtr<GraphicsDevice>                                               m_graphicsDevice;          // 描画デバイス
        int32_t                                                                         m_squareMaxCount = 0;      // まとめて描ける四角の数
        std::unique_ptr<RenderState>                                                    m_renderState;             // 描画ステート
        std::unique_ptr<EffekseerRenderer::StandardRenderer<RendererImplemented, Shader>> m_standardRenderer;     // スプライトなどをまとめて描く共通部分
        Effekseer::Backend::IndexBufferRef                                              m_indexBuffer;             // 四角を並べるインデックス
        Effekseer::Backend::IndexBufferRef                                              m_wireframeIndexBuffer;    // 四角の輪郭を並べるインデックス
        EffekseerRenderer::DistortingCallback*                                          m_distortingCallback = nullptr;    // 歪みの前に呼ぶもの（持ち主）

        bgfx::ViewId                                  m_view = 0;                     // 描く先のビュー
        std::array<bgfx::UniformHandle, Effekseer::TextureSlotMax> m_samplers;        // テクスチャのサンプラー（s_efkTex0〜7）

        // 次の描画に使うもの（共通部分が描く前に設定する）
        Shader*                            m_currentShader = nullptr;    // シェーダー
        Effekseer::Backend::VertexBufferRef m_currentVertexBuffer;      // 頂点バッファ
        int32_t                            m_currentStride = 0;          // 頂点の大きさ
        Effekseer::Backend::IndexBufferRef m_currentIndexBuffer;         // インデックスバッファ
        std::array<bgfx::TextureHandle, Effekseer::TextureSlotMax> m_currentTextures;    // テクスチャ
        int32_t                            m_currentTextureCount = 0;    // テクスチャの数

        bool m_warnedLayout   = false;    // 頂点の大きさが合わない警告を出したか
        bool m_warnedVertices = false;    // 一時的な頂点バッファが足りない警告を出したか
    };

    //------------------------------------------------------------------------
    //! モデルを描くレンダラーです（Effekseer の DX11 版の ModelRenderer と同じ作り）。
    //! @note  1回の描画でモデルを kModelInstanceCount 個まで並べる（インスタンス描画）
    //------------------------------------------------------------------------
    class ModelRenderer final : public EffekseerRenderer::ModelRendererBase {
    public:
        //! まとめて描くモデルの数（Shaders/bgfx/Effekseer/EfkModelVS.sh の EFK_MODEL_INSTANCE_COUNT と揃える）
        static constexpr int32_t kModelInstanceCount = 10;

        //! 作ります。
        //! @param  [in] renderer レンダラー
        //! @return モデルのレンダラー（シェーダーを読めなければ nullptr）
        static Effekseer::RefPtr<ModelRenderer> Create(const RendererImplementedRef& renderer);

        //! 描き始めます。
        void BeginRendering(const NodeParameter& parameter, int32_t count, void* userData) override;

        //! 1つ分を積みます。
        void Rendering(const NodeParameter& parameter, const InstanceParameter& instanceParameter, void* userData) override;

        //! 積んだ分を描きます。
        void EndRendering(const NodeParameter& parameter, void* userData) override;

    private:
        //! 作ります（Create() から呼ぶ）。
        //! @param  [in] renderer レンダラー
        //! @param  [in] shaders  advanced の lit・unlit・歪み、通常の lit・unlit・歪みの順のシェーダー
        ModelRenderer(const RendererImplementedRef& renderer, std::array<std::unique_ptr<Shader>, 6> shaders);

        RendererImplementedRef                 m_renderer;    // レンダラー
        std::array<std::unique_ptr<Shader>, 6> m_shaders;     // シェーダー
    };

}    // namespace Tsukino::Renderer::EffekseerBgfx
