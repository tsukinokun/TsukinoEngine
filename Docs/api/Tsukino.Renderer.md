# Tsukino.Renderer の公開 API

**このファイルは自動生成です。直接編集しないでください。**

型の在処だけ知りたいときは `../api-index.md` を見る。

## 主要な型

ゲーム側から実際に触る型。メンバを全て展開している。

### Tsukino::Renderer::Renderer

`Tsukino.Renderer/include/Tsukino/Renderer/Renderer.hpp`

**公開関数**

| シグネチャ | 説明 |
|---|---|
| `Renderer()=default` | コンストラクタ |
| `~Renderer()=default` | デストラクタ |
| `bool Initialize(HWND hwnd, uint32_t width, uint32_t height, const RendererShaderSet &shaders)` | レンダラーの初期化 |
| `void Render(class Tsukino::BuiltIn::ECS::EffectSystem *effectSystem=nullptr)` | 描画処理 |
| `void Resize(uint32_t width, uint32_t height)` | 描画領域のリサイズ |
| `void SetClearColor(float r, float g, float b, float a)` | クリアカラー設定 |
| `void PushDrawCommand(const DrawCommand &cmd)` | 描画コマンドの追加 |
| `const FrameStats & GetFrameStats() const` | 直前のフレームの描画統計を取得する関数 |
| `void SetVSyncEnabled(bool enabled)` | 垂直同期の有無を設定する関数 |
| `bool IsVSyncEnabled() const` | 垂直同期が有効かを取得する関数 |
| `void DrawDebugLine(const Tsukino::GraphicsCommon::DebugVertex &v1, const Tsukino::GraphicsCommon::DebugVertex &v2)` | デバッグラインの追加 |
| `void DrawDebugTriangle(const Tsukino::GraphicsCommon::DebugVertex &v1, const Tsukino::GraphicsCommon::DebugVertex &v2, const Tsukino::GraphicsCommon::DebugVertex &v3)` | デバッグ三角形の追加 |
| `void FlushDebugDraw()` | デバッグ描画の実行 |
| `PipelineFactory * GetPipelineFactory()` |  |
| `ID3D11Device * GetDevice() const` |  |
| `ID3D11DeviceContext * GetContext() const` |  |
| `MeshBuffer * GetPrimitiveMesh(Tsukino::GraphicsCommon::PrimitiveType type)` |  |
| `ID3D11SamplerState * GetSampler(Tsukino::GraphicsCommon::SamplerType type) const` |  |
| `ID3D11ShaderResourceView * GetTextureSRV(const Tsukino::Asset::TextureAsset &textureAsset)` | テクスチャ（SRV）の取得（なければ生成してキャッシュ） |
| `void UpdateSceneBuffer(const CBufferScene &sceneData)` | シーン定数バッファの更新 |
| `std::unique_ptr< DirectX::SpriteFont > CreateSpriteFont(const u8 *data, size_t size)` | SpriteFontの作成 |
| `void SetWorldCameraMatrix(const CBufferScene &data)` | ワールドカメラ行列のセット |
| `void SetOverlayCameraMatrix(const CBufferScene &data)` | オーバーレイカメラ行列のセット |
| `std::unique_ptr< DirectX::SpriteBatch > CreateSpriteBatch()` | SpriteBatchの作成 |
| `DirectX::CommonStates * GetCommonStatesTK() const` | 共通ステートの取得 |
| `void SetDirectionalLight(const hlslpp::float3 &direction, const hlslpp::float3 &color, float intensity)` | ディレクショナルライトの設定 |
| `void SetShadowPipeline(std::shared_ptr< PipelineState > staticPipeline, std::shared_ptr< PipelineState > skeletalPipeline)` | シャドウパイプラインのセット |
| `ID3D11ShaderResourceView * GetWhiteTextureSRV()` | 白テクスチャのSRVを取得 |
| `ID3D11ShaderResourceView * GetFlatNormalTextureSRV()` | フラット法線テクスチャのSRVを取得 |
| `void SetSkyParameters(const CBufferSky &sky)` | 大気散乱パラメータのセット |
| `void SetSkyPipeline(const Tsukino::Asset::ShaderAsset *vs, const Tsukino::Asset::ShaderAsset *ps)` | スカイパイプラインのセット |
| `void UpdateWaterTime(float deltaTime)` | 水面の時間経過を更新（波のアニメーションなどに使用） |
| `void SetWaterParameters(const CBufferWater &water)` | 水面パラメータのセット |
| `void SetWaterPipeline(const Tsukino::Asset::ShaderAsset *vs, const Tsukino::Asset::ShaderAsset *ps)` | 水面パイプラインのセット |
| `void SetLights(const GPULight *lights, u32 count)` | 点光源・スポットライト配列のセット（ディファードLightingパス用） |
| `bool SetMotionBlurPipeline(const Tsukino::Asset::ShaderAsset *ps)` | モーションブラーパイプラインのセット |
| `void SetMotionBlurParameters(const CBufferMotionBlur &params)` | モーションブラーパラメータのセット |
| `void SetMotionBlurEnabled(bool enabled) noexcept` | モーションブラーの有効・無効を切り替える |
| `void SetFogParameters(const CBufferFog &params)` | フォグパラメータのセット |
| `void SetFogEnabled(bool enabled) noexcept` | フォグの有効・無効を切り替える |

## 全公開型の索引

メンバ名のみ。詳細が要るときはヘッダを開く。

- **Tsukino::Renderer::CBufferFog** — `Tsukino.Renderer/include/Tsukino/Renderer/ConstantBuffer.hpp`
  - color, distanceParams, heightParams, sunColor, noiseParams, windParams
- **Tsukino::Renderer::CBufferLights** — `Tsukino.Renderer/include/Tsukino/Renderer/ConstantBuffer.hpp`
  - lightCount, pad, lights
- **Tsukino::Renderer::CBufferMaterial** — `Tsukino.Renderer/include/Tsukino/Renderer/ConstantBuffer.hpp`
  - baseColor, emissive, metallic, roughness, specular, rimColor, rimParams
- **Tsukino::Renderer::CBufferMotionBlur** — `Tsukino.Renderer/include/Tsukino/Renderer/ConstantBuffer.hpp`
  - strength, maxBlurRadius, shutterScale, sampleCount
- **Tsukino::Renderer::CBufferScene** — `Tsukino.Renderer/include/Tsukino/Renderer/ConstantBuffer.hpp`
  - view, projection, viewProj, invViewProj, lightViewProj, lightDir, lightColor, cameraPos, prevViewProj
- **Tsukino::Renderer::CBufferSkinning** — `Tsukino.Renderer/include/Tsukino/Renderer/ConstantBuffer.hpp`
  - bones
- **Tsukino::Renderer::CBufferSkinningPrev** — `Tsukino.Renderer/include/Tsukino/Renderer/ConstantBuffer.hpp`
  - bones
- **Tsukino::Renderer::CBufferSky** — `Tsukino.Renderer/include/Tsukino/Renderer/ConstantBuffer.hpp`
  - rayleighScattering, mieScattering, mieAnisotropy, sunIntensity, atmosphereHeight, planetRadius, sunDiskSize, padding0, groundColor, sunDirection
- **Tsukino::Renderer::CBufferTransform** — `Tsukino.Renderer/include/Tsukino/Renderer/ConstantBuffer.hpp`
  - world, prevWorld, motionFlags
- **Tsukino::Renderer::CBufferWater** — `Tsukino.Renderer/include/Tsukino/Renderer/ConstantBuffer.hpp`
  - time, waveSpeed, waveScale, fresnelPower, shallowColor, deepColor
- **Tsukino::Renderer::DX11Texture2D** — `Tsukino.Renderer/include/Tsukino/Renderer/DX11/Texture/DX11Texture2D.hpp`
  - DX11Texture2D(), Bind(), GetWidth(), GetHeight(), GetSRV()
- **Tsukino::Renderer::DrawCommand** — `Tsukino.Renderer/include/Tsukino/Renderer/DrawCommand.hpp`
  - material, mesh, customDraw, transform, pass, materialData, sortOrder, boneMatrices, boneCount, prevTransform, prevBoneMatrices, hasPrevFrame
- **Tsukino::Renderer::DrawCommandQueue** — `Tsukino.Renderer/include/Tsukino/Renderer/DrawCommandQueue.hpp`
  - Push(), GetCommands(), Clear(), Size()
- **Tsukino::Renderer::DynamicFontAtlas** — `Tsukino.Renderer/include/Tsukino/Renderer/Text/DynamicFontAtlas.hpp`
  - DynamicFontAtlas(), DynamicFontAtlas(), operator=(), DrawString(), MeasureString(), GetLineHeight(), GetAscent()
- **Tsukino::Renderer::DynamicFontAtlas::GlyphInfo** — `Tsukino.Renderer/include/Tsukino/Renderer/Text/DynamicFontAtlas.hpp`
  - atlasRect, page, bearingX, bearingY, advanceX, hasInk
- **Tsukino::Renderer::DynamicFontAtlas::Page** — `Tsukino.Renderer/include/Tsukino/Renderer/Text/DynamicFontAtlas.hpp`
  - texture, srv, cursorX, cursorY, shelfHeight
- **Tsukino::Renderer::GPULight** — `Tsukino.Renderer/include/Tsukino/Renderer/ConstantBuffer.hpp`
  - positionRange, colorIntensity, directionType, spotParams
- **Tsukino::Renderer::GraphicsContext** — `Tsukino.Renderer/include/Tsukino/Renderer/DX11/GraphicsContext.hpp`
  - GBufferCount, Initialize(), BeginFrame(), EndFrame(), GetDevice(), GetContext(), SetPipelineState(), SetMaterial(), GetHDRSRV(), BindBackBuffer(), BeginGBufferPass(), GetPostProcessSRV(), BindPostProcessTarget(), BindHDRRenderTarget(), BindHDRTargetOnly(), GetGBufferSRV(), GetDepthSRV(), Resize(), GetWidth(), GetHeight(), SetVSyncEnabled(), IsVSyncEnabled()
- **Tsukino::Renderer::Material** — `Tsukino.Renderer/include/Tsukino/Renderer/DX11/Material.hpp`
  - TextureSlotCount, SetPipeline(), SetTexture(), SetTexture(), SetSampler(), GetPipeline(), GetTexture(), GetTextures(), GetSampler()
- **Tsukino::Renderer::MeshBuffer** — `Tsukino.Renderer/include/Tsukino/Renderer/DX11/MeshBuffer.hpp`
  - vertexBuffer, indexBuffer, boneWeightBuffer, vertexCount, indexCount, stride
- **Tsukino::Renderer::PipelineFactory** — `Tsukino.Renderer/include/Tsukino/Renderer/DX11/PipelineFactory.hpp`
  - PipelineFactory(), Create()
- **Tsukino::Renderer::PipelineHash** — `Tsukino.Renderer/include/Tsukino/Renderer/DX11/PipelineFactory.hpp`
  - operator()()
- **Tsukino::Renderer::PipelineState** — `Tsukino.Renderer/include/Tsukino/Renderer/DX11/PipelineState.hpp`
  - vs, ps, inputLayout, rasterizer, blend, depth, topology
- **Tsukino::Renderer::Renderer** — `Tsukino.Renderer/include/Tsukino/Renderer/Renderer.hpp`
  - Renderer(), ~Renderer(), Initialize(), Render(), Resize(), SetClearColor(), PushDrawCommand(), GetFrameStats(), SetVSyncEnabled(), IsVSyncEnabled(), DrawDebugLine(), DrawDebugTriangle(), FlushDebugDraw(), GetPipelineFactory(), GetDevice(), GetContext(), GetPrimitiveMesh(), GetSampler(), GetTextureSRV(), UpdateSceneBuffer(), CreateSpriteFont(), SetWorldCameraMatrix(), SetOverlayCameraMatrix(), CreateSpriteBatch(), GetCommonStatesTK(), SetDirectionalLight(), SetShadowPipeline(), GetWhiteTextureSRV(), GetFlatNormalTextureSRV(), SetSkyParameters(), SetSkyPipeline(), UpdateWaterTime(), SetWaterParameters(), SetWaterPipeline(), SetLights(), SetMotionBlurPipeline(), SetMotionBlurParameters(), SetMotionBlurEnabled(), SetFogParameters(), SetFogEnabled()
- **Tsukino::Renderer::Renderer::FrameStats** — `Tsukino.Renderer/include/Tsukino/Renderer/Renderer.hpp`
  - commandCount, shadowDrawCalls, gbufferDrawCalls, worldDrawCalls, transparentDrawCalls, waterDrawCalls, overlayDrawCalls, skinnedDrawCalls, triangleCount, boneBytesUploaded, TotalDrawCalls()
- **Tsukino::Renderer::RendererShaderSet** — `Tsukino.Renderer/include/Tsukino/Renderer/Renderer.hpp`
  - debugVS, debugPS, tonemapVS, tonemapPS, shadowStaticVS, shadowSkeletalVS, shadowPS, lightingPS, motionBlurPS, fogPS
- **Tsukino::Renderer::Shader** — `Tsukino.Renderer/include/Tsukino/Renderer/Shader.hpp`
  - Shader(), ~Shader(), Bind()
- **Tsukino::Renderer::ShaderLoader** — `Tsukino.Renderer/include/Tsukino/Renderer/ShaderLoader.hpp`
  - LoadFromFile()
- **Tsukino::Renderer::SpriteRenderer** — `Tsukino.Renderer/include/Tsukino/Renderer/SpriteRenderer.hpp`
  - SpriteRenderer(), Draw()
