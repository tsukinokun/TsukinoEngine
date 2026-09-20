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
| `Renderer()` | コンストラクタ |
| `~Renderer()` | デストラクタ |
| `bool Initialize(HWND hwnd, uint32_t width, uint32_t height, const RendererShaderSet &shaders)` | レンダラーの初期化 |
| `void Render(IPostWorldPass *postWorldPass=nullptr)` | 描画処理 |
| `void Resize(uint32_t width, uint32_t height)` | 描画領域のリサイズ |
| `void SetClearColor(float r, float g, float b, float a)` | クリアカラー設定 |
| `DrawCommandQueue & GetDrawQueue() noexcept` | このフレームの描画コマンドキューを取得する |
| `const FrameStats & GetFrameStats() const` | 直前のフレームの描画統計を取得する関数 |
| `void SetVSyncEnabled(bool enabled)` | 垂直同期の有無を設定する関数 |
| `bool IsVSyncEnabled() const` | 垂直同期が有効かを取得する関数 |
| `DebugDraw & GetDebugDraw() noexcept` | デバッグ用の線と三角形の描画を取得する |
| `RenderResources & GetResources() noexcept` | 描画で共有する資源を取得する |
| `FrameConstants & GetFrameConstants() noexcept` | フレーム単位のシーン定数（b0）を取得する |
| `ID3D11Device * GetDevice() const` |  |
| `ID3D11DeviceContext * GetContext() const` |  |
| `LightingPass & GetLighting() noexcept` | ディファードライティングパスを取得する |
| `SkyPass & GetSky() noexcept` | スカイ（大気散乱）パスを取得する |
| `IBLBaker & GetIBL() noexcept` | スカイ由来の環境光（IBL）のベイクを取得する |
| `AmbientParticlePass & GetAmbientParticles() noexcept` | 環境パーティクル（火の粉・灰）パスを取得する |
| `FogPass & GetFog() noexcept` | フォグパスを取得する |
| `MotionBlurPass & GetMotionBlur() noexcept` | モーションブラーパスを取得する |

## 全公開型の索引

メンバ名のみ。詳細が要るときはヘッダを開く。

- **Tsukino::Renderer::AmbientParticlePass** — `Tsukino.Renderer/include/Tsukino/Renderer/AmbientParticlePass.hpp`
  - Initialize(), SetParameters(), SetEnabled(), Execute(), EndFrame()
- **Tsukino::Renderer::CBufferAmbientParticle** — `Tsukino.Renderer/include/Tsukino/Renderer/ConstantBuffer.hpp`
  - volumeParams, fadeParams, sizeParams, driftParams, swayParams, colorParams
- **Tsukino::Renderer::CBufferFog** — `Tsukino.Renderer/include/Tsukino/Renderer/ConstantBuffer.hpp`
  - color, distanceParams, heightParams, sunColor, noiseParams, windParams
- **Tsukino::Renderer::CBufferIBL** — `Tsukino.Renderer/include/Tsukino/Renderer/ConstantBuffer.hpp`
  - specularMipCount, iblIntensity, pad
- **Tsukino::Renderer::CBufferIBLBake** — `Tsukino.Renderer/include/Tsukino/Renderer/ConstantBuffer.hpp`
  - roughness, sampleCount, pad
- **Tsukino::Renderer::CBufferLights** — `Tsukino.Renderer/include/Tsukino/Renderer/ConstantBuffer.hpp`
  - lightCount, pad, lights
- **Tsukino::Renderer::CBufferMaterial** — `Tsukino.Renderer/include/Tsukino/Renderer/ConstantBuffer.hpp`
  - baseColor, emissive, metallic, roughness, specular, alphaCutoff, rimColor, rimParams
- **Tsukino::Renderer::CBufferMotionBlur** — `Tsukino.Renderer/include/Tsukino/Renderer/ConstantBuffer.hpp`
  - strength, maxBlurRadius, shutterScale, sampleCount
- **Tsukino::Renderer::CBufferScene** — `Tsukino.Renderer/include/Tsukino/Renderer/ConstantBuffer.hpp`
  - view, projection, viewProj, invViewProj, lightViewProj, lightDir, lightColor, cameraPos, prevViewProj, timeParams, screenParams, shadowParams
- **Tsukino::Renderer::CBufferSkinning** — `Tsukino.Renderer/include/Tsukino/Renderer/ConstantBuffer.hpp`
  - bones
- **Tsukino::Renderer::CBufferSkinningPrev** — `Tsukino.Renderer/include/Tsukino/Renderer/ConstantBuffer.hpp`
  - bones
- **Tsukino::Renderer::CBufferSky** — `Tsukino.Renderer/include/Tsukino/Renderer/ConstantBuffer.hpp`
  - rayleighScattering, mieScattering, mieAnisotropy, sunIntensity, atmosphereHeight, planetRadius, sunDiskSize, padding0, groundColor, sunDirection
- **Tsukino::Renderer::CBufferTransform** — `Tsukino.Renderer/include/Tsukino/Renderer/ConstantBuffer.hpp`
  - world, prevWorld, motionFlags
- **Tsukino::Renderer::DX11Texture2D** — `Tsukino.Renderer/include/Tsukino/Renderer/DX11/Texture/DX11Texture2D.hpp`
  - DX11Texture2D(), Bind(), GetWidth(), GetHeight(), GetSRV()
- **Tsukino::Renderer::DX11TextureCube** — `Tsukino.Renderer/include/Tsukino/Renderer/DX11/Texture/DX11TextureCube.hpp`
  - DX11TextureCube(), GetSRV(), GetFaceRTV(), GetBaseSize(), GetMipLevels(), GetMipSize(), IsValid()
- **Tsukino::Renderer::DebugDraw** — `Tsukino.Renderer/include/Tsukino/Renderer/DebugDraw.hpp`
  - Initialize(), DrawLine(), DrawTriangle(), Flush(), Clear()
- **Tsukino::Renderer::DrawCommand** — `Tsukino.Renderer/include/Tsukino/Renderer/DrawCommand.hpp`
  - material, mesh, customDraw, transform, pass, materialData, sortOrder, boneMatrices, boneCount, instanceCount, instanceData, castsShadow, userConstantBuffer, userConstantSlot, prevTransform, prevBoneMatrices, hasPrevFrame
- **Tsukino::Renderer::DrawCommandExecutor** — `Tsukino.Renderer/src/DrawCommandExecutor.hpp`
  - Initialize(), Execute(), ExecuteShadow()
- **Tsukino::Renderer::DrawCommandQueue** — `Tsukino.Renderer/include/Tsukino/Renderer/DrawCommandQueue.hpp`
  - Push(), AllocMaterial(), AllocMaterialData(), GetCommands(), Clear(), Size()
- **Tsukino::Renderer::DynamicFontAtlas** — `Tsukino.Renderer/include/Tsukino/Renderer/Text/DynamicFontAtlas.hpp`
  - DynamicFontAtlas(), DynamicFontAtlas(), operator=(), DrawString(), MeasureString(), GetLineHeight(), GetAscent()
- **Tsukino::Renderer::DynamicFontAtlas::GlyphInfo** — `Tsukino.Renderer/include/Tsukino/Renderer/Text/DynamicFontAtlas.hpp`
  - atlasRect, page, bearingX, bearingY, advanceX, hasInk
- **Tsukino::Renderer::DynamicFontAtlas::Page** — `Tsukino.Renderer/include/Tsukino/Renderer/Text/DynamicFontAtlas.hpp`
  - texture, srv, cursorX, cursorY, shelfHeight
- **Tsukino::Renderer::FogPass** — `Tsukino.Renderer/include/Tsukino/Renderer/FogPass.hpp`
  - Initialize(), SetParameters(), SetEnabled(), Execute(), EndFrame()
- **Tsukino::Renderer::FrameConstants** — `Tsukino.Renderer/include/Tsukino/Renderer/FrameConstants.hpp`
  - Initialize(), SetWorldCamera(), SetOverlayCamera(), AdvanceTime(), SetDirectionalLight(), GetWorldSceneData(), UploadWorld(), UploadOverlay(), EndFrame(), GetSceneBuffer(), GetSceneBufferAddress()
- **Tsukino::Renderer::FullscreenPass** — `Tsukino.Renderer/src/FullscreenPass.hpp`
  - Initialize(), IsValid(), GetVertexShader(), BindGeometry(), Draw()
- **Tsukino::Renderer::GPULight** — `Tsukino.Renderer/include/Tsukino/Renderer/ConstantBuffer.hpp`
  - positionRange, colorIntensity, directionType, spotParams
- **Tsukino::Renderer::GraphicsContext** — `Tsukino.Renderer/include/Tsukino/Renderer/DX11/GraphicsContext.hpp`
  - GBufferCount, Initialize(), BeginFrame(), EndFrame(), GetDevice(), GetContext(), SetPipelineState(), SetMaterial(), GetHDRSRV(), BindBackBuffer(), BeginGBufferPass(), GetPostProcessSRV(), BindPostProcessTarget(), BindHDRRenderTarget(), BindHDRTargetOnly(), GetGBufferSRV(), GetDepthSRV(), Resize(), GetWidth(), GetHeight(), SetVSyncEnabled(), IsVSyncEnabled()
- **Tsukino::Renderer::IBLBaker** — `Tsukino.Renderer/include/Tsukino/Renderer/IBLBaker.hpp`
  - RequestRecapture(), CreateConstantBuffers(), Initialize(), BakeIfNeeded(), Bind(), Unbind()
- **Tsukino::Renderer::IPostWorldPass** — `Tsukino.Renderer/include/Tsukino/Renderer/IPostWorldPass.hpp`
  - ~IPostWorldPass(), RenderPostWorld()
- **Tsukino::Renderer::InstanceBuffer** — `Tsukino.Renderer/include/Tsukino/Renderer/DX11/InstanceBuffer.hpp`
  - buffer, srv, stride, capacity, activeCount, IsValid()
- **Tsukino::Renderer::LightingPass** — `Tsukino.Renderer/include/Tsukino/Renderer/LightingPass.hpp`
  - Initialize(), SetDirectionalLight(), SetLights(), Execute()
- **Tsukino::Renderer::Material** — `Tsukino.Renderer/include/Tsukino/Renderer/DX11/Material.hpp`
  - TextureSlotCount, SetPipeline(), SetTexture(), SetTexture(), SetSampler(), GetPipeline(), GetTexture(), GetTextures(), GetSampler()
- **Tsukino::Renderer::MeshBuffer** — `Tsukino.Renderer/include/Tsukino/Renderer/DX11/MeshBuffer.hpp`
  - vertexBuffer, indexBuffer, boneWeightBuffer, vertexCount, indexCount, stride
- **Tsukino::Renderer::MotionBlurPass** — `Tsukino.Renderer/include/Tsukino/Renderer/MotionBlurPass.hpp`
  - Initialize(), SetParameters(), SetEnabled(), IsEnabled(), Execute(), EndFrame()
- **Tsukino::Renderer::PipelineFactory** — `Tsukino.Renderer/include/Tsukino/Renderer/DX11/PipelineFactory.hpp`
  - PipelineFactory(), Create()
- **Tsukino::Renderer::PipelineHash** — `Tsukino.Renderer/include/Tsukino/Renderer/DX11/PipelineFactory.hpp`
  - operator()()
- **Tsukino::Renderer::PipelineState** — `Tsukino.Renderer/include/Tsukino/Renderer/DX11/PipelineState.hpp`
  - vs, ps, inputLayout, rasterizer, blend, depth, topology
- **Tsukino::Renderer::RenderResources** — `Tsukino.Renderer/include/Tsukino/Renderer/RenderResources.hpp`
  - Initialize(), GetPipelineFactory(), GetCommonStatesTK(), GetSampler(), GetPrimitiveMesh(), GetTextureSRV(), GetWhiteTextureSRV(), GetFlatNormalTextureSRV(), CreateSpriteFont(), CreateSpriteBatch()
- **Tsukino::Renderer::Renderer** — `Tsukino.Renderer/include/Tsukino/Renderer/Renderer.hpp`
  - Renderer(), ~Renderer(), Initialize(), Render(), Resize(), SetClearColor(), GetDrawQueue(), GetFrameStats(), SetVSyncEnabled(), IsVSyncEnabled(), GetDebugDraw(), GetResources(), GetFrameConstants(), GetDevice(), GetContext(), GetLighting(), GetSky(), GetIBL(), GetAmbientParticles(), GetFog(), GetMotionBlur()
- **Tsukino::Renderer::Renderer::FrameStats** — `Tsukino.Renderer/include/Tsukino/Renderer/Renderer.hpp`
  - commandCount, shadowDrawCalls, gbufferDrawCalls, worldDrawCalls, transparentDrawCalls, overlayDrawCalls, skinnedDrawCalls, triangleCount, boneBytesUploaded, TotalDrawCalls()
- **Tsukino::Renderer::RendererShaderSet** — `Tsukino.Renderer/include/Tsukino/Renderer/Renderer.hpp`
  - debugVS, debugPS, tonemapVS, tonemapPS, shadowStaticVS, shadowSkeletalVS, shadowPS, lightingPS, motionBlurPS, fogPS, ambientParticleVS, ambientParticlePS, iblIrradiancePS, iblSpecularPrefilterPS, iblBRDFLUTPS
- **Tsukino::Renderer::Shader** — `Tsukino.Renderer/include/Tsukino/Renderer/Shader.hpp`
  - Shader(), ~Shader(), Bind()
- **Tsukino::Renderer::ShaderLoader** — `Tsukino.Renderer/include/Tsukino/Renderer/ShaderLoader.hpp`
  - LoadFromFile()
- **Tsukino::Renderer::ShadowPass** — `Tsukino.Renderer/src/ShadowPass.hpp`
  - kMapSize, kOrthoHalfExtent, kDepthTowardLight, kDepthAwayFromLight, kDepthRange, Initialize(), Execute(), BindForSampling(), ComputeLightViewProj()
- **Tsukino::Renderer::SkyPass** — `Tsukino.Renderer/include/Tsukino/Renderer/SkyPass.hpp`
  - Initialize(), SetParameters(), SetPipeline(), IsReady(), Execute(), GetVertexShader(), GetPixelShader(), GetBufferAddress()
- **Tsukino::Renderer::SpriteRenderer** — `Tsukino.Renderer/include/Tsukino/Renderer/SpriteRenderer.hpp`
  - SpriteRenderer(), Draw()
- **Tsukino::Renderer::TonemapPass** — `Tsukino.Renderer/src/TonemapPass.hpp`
  - Initialize(), Execute()
- **Tsukino::Renderer::UserConstantBuffer** — `Tsukino.Renderer/include/Tsukino/Renderer/DX11/UserConstantBuffer.hpp`
  - buffer, byteSize, IsValid()
