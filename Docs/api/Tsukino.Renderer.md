# Tsukino.Renderer の公開 API

**このファイルは自動生成です。直接編集しないでください。**

型の在処だけ知りたいときは `../api-index.md` を見る。

## 主要な型

ゲーム側から実際に触る型。メンバを全て展開している。

### Tsukino::Renderer::Renderer

`Tsukino.Renderer/include/Tsukino/Renderer/Renderer.hpp`

**公開メンバ**

| メンバ | 説明 |
|---|---|
| `int32_t kEffectSquareMaxCount` | Effekseer が1回にまとめて描ける四角の数 |

**公開関数**

| シグネチャ | 説明 |
|---|---|
| `Renderer()` | コンストラクタ |
| `~Renderer()` | デストラクタ |
| `bool Initialize(const NativeWindow &window, uint32_t width, uint32_t height, const RendererShaderSet &shaders, const GraphicsDeviceOptions &deviceOptions={})` | レンダラーを初期化します。 |
| `void Render(IPostWorldPass *postWorldPass=nullptr)` | 1フレームを描きます。 |
| `void Resize(uint32_t width, uint32_t height)` | 描画領域のリサイズ |
| `void SetNativeWindow(void *window)` | 描画先のウィンドウを付け直します。 |
| `void SetClearColor(float r, float g, float b, float a)` | 背景色を設定します。 |
| `DrawCommandQueue & GetDrawQueue() noexcept` | このフレームの描画コマンドキューを取得する |
| `void SetShadowCullingEnabled(bool enabled)` | シャドウパスでカスケードの範囲外を間引くかを設定する関数 |
| `void SetShadowSettings(const ShadowSettings &settings)` | カスケードシャドウの調整値（範囲・分割・解像度）を設定する関数 |
| `const ShadowSettings & GetShadowSettings() const` | 今のカスケードシャドウの調整値（丸めた後）を取得する関数 |
| `const FrameStats & GetFrameStats() const` | 直前のフレームの描画統計を取得する関数 |
| `void SetVSyncEnabled(bool enabled)` | 垂直同期の有無を設定する関数 |
| `bool IsVSyncEnabled() const` | 垂直同期が有効かを取得する関数 |
| `void SetQuality(const RenderQualitySettings &settings)` | 描画品質を設定する関数 |
| `const RenderQualitySettings & GetQuality() const noexcept` | 今の描画品質を取得する関数 |
| `const GraphicsAdapterInfo & GetAdapterInfo() const noexcept` | 作成したデバイスのアダプタ情報を取得する関数 |
| `DebugDraw & GetDebugDraw() noexcept` | デバッグ用の線と三角形の描画を取得する |
| `RenderResources & GetResources() noexcept` | 描画で共有する資源を取得する |
| `FrameConstants & GetFrameConstants() noexcept` | フレーム単位のシーン定数（u_scene）を取得する |
| `::EffekseerRenderer::Renderer * GetEffectRenderer()` |  |
| `LightingPass & GetLighting() noexcept` | ディファードライティングパスを取得する |
| `SkyPass & GetSky() noexcept` | スカイ（大気散乱）パスを取得する |
| `IBLBaker & GetIBL() noexcept` | スカイ由来の環境光（IBL）のベイクを取得する |
| `AmbientParticlePass & GetAmbientParticles() noexcept` | 環境パーティクル（火の粉・灰）パスを取得する |
| `FogPass & GetFog() noexcept` | フォグパスを取得する |
| `SphereColorGradePass & GetSphereColorGrade() noexcept` | 球形の色調補正パスを取得する |
| `MotionBlurPass & GetMotionBlur() noexcept` | モーションブラーパスを取得する |
| `BloomPass & GetBloom() noexcept` | ブルームパスを取得する |

## 全公開型の索引

メンバ名のみ。詳細が要るときはヘッダを開く。

- **Tsukino::Renderer::AmbientParticlePass** — `Tsukino.Renderer/include/Tsukino/Renderer/AmbientParticlePass.hpp`
  - AmbientParticlePass(), ~AmbientParticlePass(), AmbientParticlePass(), operator=(), GetGpu(), GetGpu(), Initialize(), SetParameters(), SetEnabled(), Execute(), EndFrame()
- **Tsukino::Renderer::AmbientParticlePass::Gpu** — `Tsukino.Renderer/internal/Tsukino/Renderer/Bgfx/ScenePassesGpu.hpp`
  - program, particle, Gpu(), Gpu(), operator=(), ~Gpu()
- **Tsukino::Renderer::BloomParameters** — `Tsukino.Renderer/include/Tsukino/Renderer/BloomPass.hpp`
  - threshold, softKnee, intensity, radius, tintR, tintG, tintB
- **Tsukino::Renderer::BloomPass** — `Tsukino.Renderer/include/Tsukino/Renderer/BloomPass.hpp`
  - kMaxMipCount, BloomPass(), ~BloomPass(), BloomPass(), operator=(), GetGpu(), GetGpu(), Initialize(), SetParameters(), SetEnabled(), IsEnabled(), SetMipLimit(), Execute(), EndFrame()
- **Tsukino::Renderer::BloomPass::Gpu** — `Tsukino.Renderer/internal/Tsukino/Renderer/Bgfx/BloomPassGpu.hpp`
  - prefilterProgram, downsampleProgram, upsampleProgram, params, sourceSampler, mips, Gpu(), Gpu(), operator=(), ~Gpu(), DestroyMips()
- **Tsukino::Renderer::BloomPass::Gpu::MipTarget** — `Tsukino.Renderer/internal/Tsukino/Renderer/Bgfx/BloomPassGpu.hpp`
  - texture, frameBuffer, width, height
- **Tsukino::Renderer::BoundingSphere** — `Tsukino.Renderer/internal/Tsukino/Renderer/ShadowCascades.hpp`
  - center, radius
- **Tsukino::Renderer::CBufferAmbientParticle** — `Tsukino.Renderer/include/Tsukino/Renderer/ConstantBuffer.hpp`
  - volumeParams, fadeParams, sizeParams, driftParams, swayParams, colorParams
- **Tsukino::Renderer::CBufferBloom** — `Tsukino.Renderer/include/Tsukino/Renderer/ConstantBuffer.hpp`
  - texelSizeX, texelSizeY, threshold, softKnee, tintR, tintG, tintB, intensity, radius, padding0, padding1, padding2
- **Tsukino::Renderer::CBufferFog** — `Tsukino.Renderer/include/Tsukino/Renderer/ConstantBuffer.hpp`
  - color, distanceParams, heightParams, sunColor, noiseParams, windParams, originParams
- **Tsukino::Renderer::CBufferIBL** — `Tsukino.Renderer/include/Tsukino/Renderer/ConstantBuffer.hpp`
  - specularMipCount, iblIntensity, pad
- **Tsukino::Renderer::CBufferIBLBake** — `Tsukino.Renderer/include/Tsukino/Renderer/ConstantBuffer.hpp`
  - roughness, sampleCount, pad
- **Tsukino::Renderer::CBufferLights** — `Tsukino.Renderer/include/Tsukino/Renderer/ConstantBuffer.hpp`
  - lightCount, pad, lights
- **Tsukino::Renderer::CBufferMaterial** — `Tsukino.Renderer/include/Tsukino/Renderer/ConstantBuffer.hpp`
  - baseColor, emissive, metallic, roughness, specular, alphaCutoff, rimColor, rimParams, spriteFill, shading, toonShadeColor, spriteUv, spriteFog
- **Tsukino::Renderer::CBufferMotionBlur** — `Tsukino.Renderer/include/Tsukino/Renderer/ConstantBuffer.hpp`
  - strength, maxBlurRadius, shutterScale, sampleCount
- **Tsukino::Renderer::CBufferScene** — `Tsukino.Renderer/include/Tsukino/Renderer/ConstantBuffer.hpp`
  - view, projection, viewProj, invViewProj, cascadeViewProj, lightDir, lightColor, cameraPos, prevViewProj, timeParams, screenParams, shadowParams, cascadeTexelWorld, cascadeSplitFar, occlusionCutout0, occlusionCutout1, qualityParams
- **Tsukino::Renderer::CBufferSkinning** — `Tsukino.Renderer/include/Tsukino/Renderer/ConstantBuffer.hpp`
  - bones
- **Tsukino::Renderer::CBufferSkinningPrev** — `Tsukino.Renderer/include/Tsukino/Renderer/ConstantBuffer.hpp`
  - bones
- **Tsukino::Renderer::CBufferSky** — `Tsukino.Renderer/include/Tsukino/Renderer/ConstantBuffer.hpp`
  - rayleighScattering, mieScattering, mieAnisotropy, sunIntensity, atmosphereHeight, planetRadius, sunDiskSize, padding0, groundColor, sunDirection
- **Tsukino::Renderer::CBufferSphereColorGrade** — `Tsukino.Renderer/include/Tsukino/Renderer/ConstantBuffer.hpp`
  - sphere, tint, fogColor, edgeColor, edgeParams
- **Tsukino::Renderer::CBufferTransform** — `Tsukino.Renderer/include/Tsukino/Renderer/ConstantBuffer.hpp`
  - world, prevWorld, motionFlags
- **Tsukino::Renderer::ClipRect** — `Tsukino.Renderer/include/Tsukino/Renderer/DrawCommand.hpp`
  - left, top, right, bottom
- **Tsukino::Renderer::DebugDraw** — `Tsukino.Renderer/include/Tsukino/Renderer/DebugDraw.hpp`
  - DrawLine(), DrawTriangle(), Submit(), Clear()
- **Tsukino::Renderer::DrawCommand** — `Tsukino.Renderer/include/Tsukino/Renderer/DrawCommand.hpp`
  - material, mesh, transform, pass, materialData, kind, batchFirst, batchCount, sortOrder, boneMatrices, boneCount, instanceCount, instanceData, castsShadow, ditherFade, occlusionCutout, shadowPipeline, userConstantBuffer, userConstantSlot, prevTransform, prevBoneMatrices, hasPrevFrame, hasClipRect, clipRect, cameraOverride
- **Tsukino::Renderer::DrawCommandExecutor** — `Tsukino.Renderer/src/Bgfx/DrawCommandExecutor.hpp`
  - ~DrawCommandExecutor(), Initialize(), Execute(), ExecuteShadow(), SetShadowPass(), SetFogPass(), SetMotionBlurEnabled()
- **Tsukino::Renderer::DrawCommandQueue** — `Tsukino.Renderer/include/Tsukino/Renderer/DrawCommandQueue.hpp`
  - Push(), PushSprites(), PushLines(), AllocMaterial(), AllocMaterialData(), AllocSceneData(), GetCommands(), GetSprites(), GetLineVertices(), Clear(), Size()
- **Tsukino::Renderer::DynamicFontAtlas** — `Tsukino.Renderer/include/Tsukino/Renderer/Text/DynamicFontAtlas.hpp`
  - DynamicFontAtlas(), ~DynamicFontAtlas(), DynamicFontAtlas(), operator=(), AppendStringSprites(), MeasureString(), GetLineHeight(), GetAscent()
- **Tsukino::Renderer::DynamicFontAtlas::Font** — `Tsukino.Renderer/src/Text/DynamicFontAtlas.cpp`
  - data, info
- **Tsukino::Renderer::DynamicFontAtlas::GlyphImage** — `Tsukino.Renderer/include/Tsukino/Renderer/Text/DynamicFontAtlas.hpp`
  - atlasRect, page, bearingX, bearingY, hasInk
- **Tsukino::Renderer::DynamicFontAtlas::GlyphMetrics** — `Tsukino.Renderer/include/Tsukino/Renderer/Text/DynamicFontAtlas.hpp`
  - glyphIndex, advanceX
- **Tsukino::Renderer::DynamicFontAtlas::Page** — `Tsukino.Renderer/include/Tsukino/Renderer/Text/DynamicFontAtlas.hpp`
  - texture, cursorX, cursorY, shelfHeight
- **Tsukino::Renderer::EffectRenderer** — `Tsukino.Renderer/src/Bgfx/EffectRenderer.hpp`
  - EffectRenderer(), ~EffectRenderer(), EffectRenderer(), operator=(), Initialize(), Get(), Prepare(), Present()
- **Tsukino::Renderer::EffectRenderer::Impl** — `Tsukino.Renderer/src/Bgfx/EffectRenderer.cpp`
  - renderer, background, depth
- **Tsukino::Renderer::EffekseerBgfx::GraphicsDevice** — `Tsukino.Renderer/src/Bgfx/Effekseer/EfkGraphicsDevice.hpp`
  - CreateVertexBuffer(), CreateIndexBuffer(), UpdateVertexBuffer(), UpdateIndexBuffer(), CreateVertexLayout(), CreateTexture(), GetDeviceName(), CreateBorrowedTexture()
- **Tsukino::Renderer::EffekseerBgfx::IndexBuffer** — `Tsukino.Renderer/src/Bgfx/Effekseer/EfkGraphicsDevice.hpp`
  - IndexBuffer(), ~IndexBuffer(), UpdateData(), Bind()
- **Tsukino::Renderer::EffekseerBgfx::ModelRenderer** — `Tsukino.Renderer/src/Bgfx/Effekseer/EfkRenderer.hpp`
  - kModelInstanceCount, Create(), BeginRendering(), Rendering(), EndRendering()
- **Tsukino::Renderer::EffekseerBgfx::RenderState** — `Tsukino.Renderer/src/Bgfx/Effekseer/EfkRenderer.hpp`
  - RenderState(), Update(), GetBgfxState(), GetSamplerFlags()
- **Tsukino::Renderer::EffekseerBgfx::RendererImplemented** — `Tsukino.Renderer/src/Bgfx/Effekseer/EfkRenderer.hpp`
  - RendererImplemented(), ~RendererImplemented(), Initialize(), SetView(), GetBgfxGraphicsDevice(), OnLostDevice(), OnResetDevice(), SetRestorationOfStatesFlag(), BeginRendering(), EndRendering(), GetSquareMaxCount(), CreateSpriteRenderer(), CreateRibbonRenderer(), CreateRingRenderer(), CreateModelRenderer(), CreateTrackRenderer(), CreateTextureLoader(), CreateModelLoader(), CreateMaterialLoader(), ResetRenderState(), GetDistortingCallback(), SetDistortingCallback(), GetGraphicsDevice(), GetRenderState(), GetIndexBuffer(), GetStandardRenderer(), SetVertexBuffer(), SetIndexBuffer(), SetLayout(), DrawSprites(), DrawPolygon(), DrawPolygonInstanced(), BeginShader(), EndShader(), SetVertexBufferToShader(), SetPixelBufferToShader(), SetTextures(), GetRef(), AddRef(), Release()
- **Tsukino::Renderer::EffekseerBgfx::Shader** — `Tsukino.Renderer/src/Bgfx/Effekseer/EfkRenderer.hpp`
  - Shader(), ~Shader(), Shader(), operator=(), IsValid(), SetVertexConstantBufferSize(), SetPixelConstantBufferSize(), GetVertexConstantBuffer(), GetPixelConstantBuffer(), SetConstantBuffer(), GetVertexConstantBufferSize(), GetPixelConstantBufferSize(), ApplyUniforms(), GetProgram(), GetLayout()
- **Tsukino::Renderer::EffekseerBgfx::Texture** — `Tsukino.Renderer/src/Bgfx/Effekseer/EfkGraphicsDevice.hpp`
  - Texture(), ~Texture(), GetHandle(), SetBorrowedHandle()
- **Tsukino::Renderer::EffekseerBgfx::VertexBuffer** — `Tsukino.Renderer/src/Bgfx/Effekseer/EfkGraphicsDevice.hpp`
  - VertexBuffer(), ~VertexBuffer(), UpdateData(), Bind()
- **Tsukino::Renderer::EffekseerBgfx::VertexLayout** — `Tsukino.Renderer/src/Bgfx/Effekseer/EfkGraphicsDevice.hpp`
  - VertexLayout(), GetLayout(), IsValid()
- **Tsukino::Renderer::FogPass** — `Tsukino.Renderer/include/Tsukino/Renderer/FogPass.hpp`
  - FogPass(), ~FogPass(), FogPass(), operator=(), GetGpu(), GetGpu(), Initialize(), SetParameters(), SetEnabled(), Execute(), BindForwardFog(), UnbindForwardFog(), EndFrame()
- **Tsukino::Renderer::FogPass::Gpu** — `Tsukino.Renderer/internal/Tsukino/Renderer/Bgfx/FogPassGpu.hpp`
  - program, fog, depthSampler, forwardData, forwardActive, Gpu(), Gpu(), operator=(), ~Gpu(), ApplyToDraw()
- **Tsukino::Renderer::FrameConstants** — `Tsukino.Renderer/include/Tsukino/Renderer/FrameConstants.hpp`
  - FrameConstants(), ~FrameConstants(), FrameConstants(), operator=(), GetGpu(), GetGpu(), Initialize(), SetShadowMapParams(), SetWorldCamera(), SetOverlayCamera(), AdvanceTime(), SetDirectionalLight(), SetShadowCascades(), SetOcclusionCutout(), SetShadowCascadeIndex(), GetWorldSceneData(), UploadWorld(), SetShadowSoftFilter(), UploadOverlay(), UploadWorldWithCamera(), EndFrame()
- **Tsukino::Renderer::FrameConstants::Gpu** — `Tsukino.Renderer/internal/Tsukino/Renderer/Bgfx/FrameConstantsGpu.hpp`
  - scene, current, Apply()
- **Tsukino::Renderer::FrustumCorners** — `Tsukino.Renderer/internal/Tsukino/Renderer/ShadowCascades.hpp`
  - nearPlane, farPlane, nearDepth, farDepth
- **Tsukino::Renderer::FullscreenPass** — `Tsukino.Renderer/src/Bgfx/FullscreenPass.hpp`
  - Initialize(), IsValid(), CreateProgram(), Submit()
- **Tsukino::Renderer::GPULight** — `Tsukino.Renderer/include/Tsukino/Renderer/ConstantBuffer.hpp`
  - positionRange, colorIntensity, directionType, spotParams
- **Tsukino::Renderer::GpuFrameTimings** — `Tsukino.Renderer/include/Tsukino/Renderer/GpuTiming.hpp`
  - passes, frameMs, valid
- **Tsukino::Renderer::GpuHandle** — `Tsukino.Renderer/include/Tsukino/Renderer/GpuHandles.hpp`
  - kInvalidIndex, index, IsValid(), operator==()
- **Tsukino::Renderer::GpuPassTiming** — `Tsukino.Renderer/include/Tsukino/Renderer/GpuTiming.hpp`
  - name, ms
- **Tsukino::Renderer::GpuProfiler** — `Tsukino.Renderer/src/Bgfx/GpuProfiler.hpp`
  - Initialize(), Collect()
- **Tsukino::Renderer::GraphicsAdapterInfo** — `Tsukino.Renderer/include/Tsukino/Renderer/RenderQuality.hpp`
  - name, vendorId, deviceId, isSoftware, isUnifiedMemory, dedicatedMemory, sharedMemory
- **Tsukino::Renderer::GraphicsContext** — `Tsukino.Renderer/internal/Tsukino/Renderer/Bgfx/GraphicsContext.hpp`
  - kGBufferCount, ~GraphicsContext(), Initialize(), Resize(), SetVSyncEnabled(), SetWindowHandle(), IsVSyncEnabled(), GetAdapterInfo(), GetWidth(), GetHeight(), SetRenderScale(), GetSceneWidth(), GetSceneHeight(), GetGBufferFrameBuffer(), GetHdrFrameBuffer(), GetHdrColorFrameBuffer(), GetHdrTexture(), GetPostProcessTexture(), GetPostProcessFrameBuffer(), GetGBufferTexture(), GetDepthTexture(), EnableEffectTargets(), HasEffectTargets(), GetEffectFrameBuffer(), GetEffectColorTexture(), GetEffectBackgroundTexture(), GetEffectDepthTexture(), IsBgfxAlive()
- **Tsukino::Renderer::GraphicsDeviceOptions** — `Tsukino.Renderer/include/Tsukino/Renderer/RenderQuality.hpp`
  - preference
- **Tsukino::Renderer::IBLBaker** — `Tsukino.Renderer/include/Tsukino/Renderer/IBLBaker.hpp`
  - IBLBaker(), ~IBLBaker(), IBLBaker(), operator=(), GetGpu(), GetGpu(), RequestRecapture(), CreateConstantBuffers(), Initialize(), BakeIfNeeded(), Bind()
- **Tsukino::Renderer::IBLBaker::Gpu** — `Tsukino.Renderer/internal/Tsukino/Renderer/Bgfx/IBLBakerGpu.hpp`
  - kFaceCount, kSpecularMipCount, capture, irradiance, specular, brdfLut, captureFaces, irradianceFaces, specularFaces, brdfLutFrameBuffer, irradianceProgram, specularProgram, brdfLutProgram, iblUniform, irradianceSampler, specularSampler, brdfLutSampler, bakeUniform, captureSampler, specularMipCount, intensity, Gpu(), Gpu(), operator=(), ~Gpu(), CreatePlaceholder(), CreateBakeTargets(), Bind()
- **Tsukino::Renderer::IPostWorldPass** — `Tsukino.Renderer/include/Tsukino/Renderer/IPostWorldPass.hpp`
  - ~IPostWorldPass(), RenderPostWorld()
- **Tsukino::Renderer::LightingPass** — `Tsukino.Renderer/include/Tsukino/Renderer/LightingPass.hpp`
  - LightingPass(), ~LightingPass(), LightingPass(), operator=(), GetGpu(), GetGpu(), Initialize(), SetDirectionalLight(), SetLights(), Execute()
- **Tsukino::Renderer::LightingPass::Gpu** — `Tsukino.Renderer/internal/Tsukino/Renderer/Bgfx/LightingPassGpu.hpp`
  - program, lights, gbufferSamplers, Gpu(), Gpu(), operator=(), ~Gpu()
- **Tsukino::Renderer::Material** — `Tsukino.Renderer/include/Tsukino/Renderer/Material.hpp`
  - TextureSlotCount, SetPipeline(), SetTexture(), SetTexture(), SetSampler(), GetPipeline(), GetTexture(), GetTextures(), GetSampler()
- **Tsukino::Renderer::MeshBuffer** — `Tsukino.Renderer/include/Tsukino/Renderer/MeshBuffer.hpp`
  - gpu, vertexCount, indexCount, stride, boundsCenter, boundsRadius
- **Tsukino::Renderer::MeshBuffer::Gpu** — `Tsukino.Renderer/internal/Tsukino/Renderer/Bgfx/RenderResourcesGpu.hpp`
  - vertexBuffer, indexBuffer, boneBuffer, Gpu(), Gpu(), operator=(), ~Gpu()
- **Tsukino::Renderer::MotionBlurPass** — `Tsukino.Renderer/include/Tsukino/Renderer/MotionBlurPass.hpp`
  - MotionBlurPass(), ~MotionBlurPass(), MotionBlurPass(), operator=(), GetGpu(), GetGpu(), Initialize(), SetParameters(), SetEnabled(), IsEnabled(), SetAllowed(), Execute(), EndFrame()
- **Tsukino::Renderer::MotionBlurPass::Gpu** — `Tsukino.Renderer/internal/Tsukino/Renderer/Bgfx/MotionBlurPassGpu.hpp`
  - program, params, sceneSampler, velocitySampler, Gpu(), Gpu(), operator=(), ~Gpu()
- **Tsukino::Renderer::NativeWindow** — `Tsukino.Renderer/include/Tsukino/Renderer/NativeHandles.hpp`
  - handle
- **Tsukino::Renderer::PipelineFactory** — `Tsukino.Renderer/include/Tsukino/Renderer/PipelineFactory.hpp`
  - PipelineFactory(), Create()
- **Tsukino::Renderer::PipelineHash** — `Tsukino.Renderer/include/Tsukino/Renderer/PipelineFactory.hpp`
  - operator()()
- **Tsukino::Renderer::PipelineState** — `Tsukino.Renderer/internal/Tsukino/Renderer/Bgfx/RenderResourcesGpu.hpp`
  - program, state, format, PipelineState(), PipelineState(), operator=(), ~PipelineState()
- **Tsukino::Renderer::RenderQualityGovernor** — `Tsukino.Renderer/include/Tsukino/Renderer/RenderQualityGovernor.hpp`
  - SetSettings(), Reset(), Tick()
- **Tsukino::Renderer::RenderQualityGovernorSettings** — `Tsukino.Renderer/include/Tsukino/Renderer/RenderQualityGovernor.hpp`
  - targetFrameSeconds, sustainSeconds, cooldownSeconds, smoothingSeconds, ignoreAboveSeconds
- **Tsukino::Renderer::RenderQualitySettings** — `Tsukino.Renderer/include/Tsukino/Renderer/RenderQuality.hpp`
  - renderScale, shadowMapSizeLimit, shadowCascadeCount, shadowSoftFilter, bloomMipCount, motionBlur, drawSky, textureMipSkip, kMinRenderScale
- **Tsukino::Renderer::RenderResources** — `Tsukino.Renderer/include/Tsukino/Renderer/RenderResources.hpp`
  - RenderResources(), ~RenderResources(), RenderResources(), operator=(), Initialize(), GetPipelineFactory(), GetPrimitiveMesh(), CreateMesh(), GetTexture(), UploadTexture(), GetWhiteTexture(), GetFlatNormalTexture(), CreateTexture(), UpdateTexture(), DestroyTexture(), CreateInstanceBuffer(), UpdateInstanceBuffer(), CreateUserConstantBuffer(), UpdateUserConstantBuffer(), DestroyBuffer(), GetGpu(), GetGpu()
- **Tsukino::Renderer::RenderResources::Gpu** — `Tsukino.Renderer/internal/Tsukino/Renderer/Bgfx/RenderResourcesGpu.hpp`
  - kMaxInstanceStride, kMaxUserConstantVec4Count, kUserConstantSlotCount, pipelineFactory, primitiveMeshes, textures, freeTextureSlots, textureCache, buffers, freeBufferSlots, white, flatNormal, userConstantUniforms, Gpu(), Gpu(), operator=(), ~Gpu(), AddTexture(), FindTexture(), AddBuffer(), FindBuffer(), GetVertexLayout(), GetBoneLayout(), GetSamplerFlags()
- **Tsukino::Renderer::RenderResources::Gpu::BufferEntry** — `Tsukino.Renderer/internal/Tsukino/Renderer/Bgfx/RenderResourcesGpu.hpp`
  - kind, stride, capacity, activeCount, data
- **Tsukino::Renderer::RenderResources::Gpu::TextureEntry** — `Tsukino.Renderer/internal/Tsukino/Renderer/Bgfx/RenderResourcesGpu.hpp`
  - texture, width, height, format
- **Tsukino::Renderer::Renderer** — `Tsukino.Renderer/include/Tsukino/Renderer/Renderer.hpp`
  - kEffectSquareMaxCount, Renderer(), ~Renderer(), Initialize(), Render(), Resize(), SetNativeWindow(), SetClearColor(), GetDrawQueue(), SetShadowCullingEnabled(), SetShadowSettings(), GetShadowSettings(), GetFrameStats(), SetVSyncEnabled(), IsVSyncEnabled(), SetQuality(), GetQuality(), GetAdapterInfo(), GetDebugDraw(), GetResources(), GetFrameConstants(), GetEffectRenderer(), GetLighting(), GetSky(), GetIBL(), GetAmbientParticles(), GetFog(), GetSphereColorGrade(), GetMotionBlur(), GetBloom()
- **Tsukino::Renderer::Renderer::FrameStats** — `Tsukino.Renderer/include/Tsukino/Renderer/Renderer.hpp`
  - commandCount, shadowDrawCalls, gbufferDrawCalls, worldDrawCalls, transparentDrawCalls, overlayDrawCalls, skinnedDrawCalls, triangleCount, boneBytesUploaded, gpu, TotalDrawCalls()
- **Tsukino::Renderer::RendererShaderSet** — `Tsukino.Renderer/include/Tsukino/Renderer/Renderer.hpp`
  - debugVS, debugPS, tonemapVS, tonemapPS, shadowStaticVS, shadowSkeletalVS, shadowPS, lightingPS, motionBlurPS, fogPS, sphereColorGradePS, bloomPrefilterPS, bloomDownsamplePS, bloomUpsamplePS, ambientParticleVS, ambientParticlePS, iblIrradiancePS, iblSpecularPrefilterPS, iblBRDFLUTPS
- **Tsukino::Renderer::ShadowCascade** — `Tsukino.Renderer/internal/Tsukino/Renderer/ShadowCascades.hpp`
  - splitNear, splitFar, bounds, texelWorldSize, lightViewProj
- **Tsukino::Renderer::ShadowCascadeSet** — `Tsukino.Renderer/internal/Tsukino/Renderer/ShadowCascades.hpp`
  - cascades, depthTowardLight, depthAwayFromLight
- **Tsukino::Renderer::ShadowPass** — `Tsukino.Renderer/src/Bgfx/ShadowPass.hpp`
  - kCascadeCount, ShadowPass(), ~ShadowPass(), Initialize(), SetSettings(), SetQuality(), GetSettings(), UpdateCascades(), Execute(), BindForSampling(), SetCullingEnabled()
- **Tsukino::Renderer::ShadowSettings** — `Tsukino.Renderer/include/Tsukino/Renderer/ShadowSettings.hpp`
  - distance, splitStart, splitLambda, mapSize, casterMargin, kMinMapSize, kMaxMapSize
- **Tsukino::Renderer::SkyPass** — `Tsukino.Renderer/include/Tsukino/Renderer/SkyPass.hpp`
  - SkyPass(), ~SkyPass(), SkyPass(), operator=(), GetGpu(), GetGpu(), Initialize(), SetParameters(), SetPipeline(), IsReady(), Execute(), SetDrawAllowed()
- **Tsukino::Renderer::SkyPass::Gpu** — `Tsukino.Renderer/internal/Tsukino/Renderer/Bgfx/SkyPassGpu.hpp`
  - program, sky, fullscreenPass, data, Gpu(), Gpu(), operator=(), ~Gpu()
- **Tsukino::Renderer::SphereColorGradePass** — `Tsukino.Renderer/include/Tsukino/Renderer/SphereColorGradePass.hpp`
  - SphereColorGradePass(), ~SphereColorGradePass(), SphereColorGradePass(), operator=(), GetGpu(), GetGpu(), Initialize(), SetParameters(), SetEnabled(), Execute(), EndFrame()
- **Tsukino::Renderer::SphereColorGradePass::Gpu** — `Tsukino.Renderer/internal/Tsukino/Renderer/Bgfx/ScenePassesGpu.hpp`
  - program, grade, depthSampler, Gpu(), Gpu(), operator=(), ~Gpu()
- **Tsukino::Renderer::SpriteQuad** — `Tsukino.Renderer/include/Tsukino/Renderer/DrawCommand.hpp`
  - texture, x, y, source, color, rotation, originX, originY, scaleX, scaleY
- **Tsukino::Renderer::SpriteRenderer** — `Tsukino.Renderer/include/Tsukino/Renderer/SpriteRenderer.hpp`
  - SpriteRenderer(), Draw()
- **Tsukino::Renderer::TexelRect** — `Tsukino.Renderer/include/Tsukino/Renderer/DrawCommand.hpp`
  - left, top, right, bottom
- **Tsukino::Renderer::TonemapPass** — `Tsukino.Renderer/src/Bgfx/TonemapPass.hpp`
  - ~TonemapPass(), Initialize(), Execute()
- **Tsukino::Renderer::UniformBlock** — `Tsukino.Renderer/internal/Tsukino/Renderer/Bgfx/UniformBlock.hpp`
  - kVec4Count, UniformBlock(), UniformBlock(), operator=(), ~UniformBlock(), Create(), Destroy(), Set(), IsValid()

## 列挙型

- **Tsukino::Renderer::BlendMode** — `Tsukino.Renderer/include/Tsukino/Renderer/BlendMode.hpp` — パイプラインのブレンドモード
  - `Opaque` — 不透明（デフォルト）
  - `Alpha` — アルファブレンド（半透明）
  - `Additive` — 加算合成
  - `DepthOnly` — 色を書かず深度のみ埋める（半透明モデルの自己重なり対策用の事前パス）
- **Tsukino::Renderer::CullMode** — `Tsukino.Renderer/include/Tsukino/Renderer/CullMode.hpp` — 面カリングの種類を表す列挙型です。
  - `None`
  - `Back`
  - `Front`
- **Tsukino::Renderer::DepthMode** — `Tsukino.Renderer/include/Tsukino/Renderer/DepthMode.hpp` — 深度モードの列挙型
  - `None`
  - `ReadWrite`
  - `ReadOnly`
  - `EqualReadOnly`
- **Tsukino::Renderer::DrawKind** — `Tsukino.Renderer/include/Tsukino/Renderer/DrawCommand.hpp` — 描画コマンドが何を描くか
  - `Mesh`
  - `Sprites`
  - `Lines`
- **Tsukino::Renderer::GpuPass** — `Tsukino.Renderer/include/Tsukino/Renderer/GpuTiming.hpp`
  - `Shadow` — シャドウマップ
  - `Sky` — スカイ（大気散乱）
  - `GBuffer` — GBuffer（不透明3Dモデル）
  - `Lighting` — ディファードライティング
  - `World` — フォワード不透明と環境パーティクル
  - `Fog` — フォグ
  - `ColorGrade` — 球形の色調補正
  - `Transparent` — 半透明（深度事前パスを含む）
  - `MotionBlur` — モーションブラー
  - `Bloom` — ブルーム
  - `Tonemap` — トーンマップ
  - `Effects` — エフェクト（IPostWorldPass）
  - `Overlay` — UI・フォント・画面上の3Dモデル
  - `Count`
- **Tsukino::Renderer::GraphicsDevicePreference** — `Tsukino.Renderer/include/Tsukino/Renderer/RenderQuality.hpp` — グラフィックスデバイスの選び方です。
  - `Auto` — 高性能なハードウェアを優先し、作れなければ WARP（ソフトウェア描画）へ落とす
  - `ForceSoftware` — 最初から WARP で作る（GPU が無い環境の確認用）
- **Tsukino::Renderer::MaterialTextureSlot** — `Tsukino.Renderer/include/Tsukino/Renderer/ShaderSlots.hpp` — マテリアルが持つテクスチャの枠
  - `Albedo = 0` — アルベド
  - `Normal` — 法線マップ
  - `MetallicRoughness` — メタリック・ラフネス（G = roughness、B = metallic）
  - `Emissive` — 発光
  - `AO` — アンビエントオクルージョン
  - `Count` — 枠の数
- **Tsukino::Renderer::RenderPass** — `Tsukino.Renderer/include/Tsukino/Renderer/DrawCommand.hpp` — 描画パス
  - `GBuffer`
  - `World`
  - `TransparentDepth`
  - `Transparent`
  - `Overlay`
- **Tsukino::Renderer::RenderQualityLevel** — `Tsukino.Renderer/include/Tsukino/Renderer/RenderQuality.hpp` — 描画品質の段階です（上が高品質）。
  - `High` — 等倍解像度・全カスケード・全段ブルーム（品質を下げない）
  - `Medium` — 影とブルームを少し抑える
  - `Low` — 内部解像度を下げ、影を簡略化する（内蔵 GPU 向け）
  - `Minimum` — ブルームを切り、影を最小にする（ソフトウェア描画向け）
- **Tsukino::Renderer::RenderResources::Gpu::BufferKind** — `Tsukino.Renderer/internal/Tsukino/Renderer/Bgfx/RenderResourcesGpu.hpp` — バッファの種類
  - `None`
  - `Instance`
  - `UserConstant`
- **Tsukino::Renderer::UserUniformSlot** — `Tsukino.Renderer/include/Tsukino/Renderer/ShaderSlots.hpp` — ゲームが自前のシェーダーへ渡す定数の枠
  - `User0 = 0` — 1本目（u_user0）
  - `User1` — 2本目（u_user1）
  - `Count` — 枠の数

## 型の別名

- `using Tsukino::Renderer::BufferHandle = GpuHandle<BufferHandleTag>` — `Tsukino.Renderer/include/Tsukino/Renderer/GpuHandles.hpp` — バッファ（インスタンス・ゲーム定義の定数）を指すハンドル
- `using Tsukino::Renderer::PipelineKey = std::tuple<u64, u64, Tsukino::GraphicsCommon::VertexFormat, DepthMode, BlendMode, CullMode>` — `Tsukino.Renderer/include/Tsukino/Renderer/PipelineFactory.hpp`
- `using Tsukino::Renderer::TextureHandle = GpuHandle<TextureHandleTag>` — `Tsukino.Renderer/include/Tsukino/Renderer/GpuHandles.hpp` — テクスチャを指すハンドル

## 名前空間の関数

### Tsukino::Renderer — `Tsukino.Renderer/include/Tsukino/Renderer/GpuTiming.hpp`

| シグネチャ | 説明 |
|---|---|
| `const char * GetGpuPassName(GpuPass pass) noexcept` |  |

### Tsukino::Renderer — `Tsukino.Renderer/include/Tsukino/Renderer/RenderQuality.hpp`

| シグネチャ | 説明 |
|---|---|
| `const char * GetRenderQualityName(RenderQualityLevel level) noexcept` |  |
| `RenderQualitySettings GetRenderQualityPreset(RenderQualityLevel level) noexcept` |  |
| `RenderQualityLevel RecommendRenderQuality(const GraphicsAdapterInfo &adapter) noexcept` |  |

## 名前空間の定数・変数

### Tsukino::Renderer — `Tsukino.Renderer/include/Tsukino/Renderer/ConstantBuffer.hpp`

| シグネチャ | 説明 |
|---|---|
| `unsigned int MAX_LIGHTS = 64` | 同時に扱える点光源・スポットライトの上限数 |
| `unsigned int kMaxAmbientParticles = 65536` |  |
| `int kMotionBlurMaxSamples = 16` | モーションブラーのサンプル数上限（MotionBlur.ps.sc と一致させること） |
| `unsigned int kShadowCascadeCount = 4` |  |

### Tsukino::Renderer — `Tsukino.Renderer/include/Tsukino/Renderer/GpuTiming.hpp`

| シグネチャ | 説明 |
|---|---|
| `std::size_t kGpuPassCount = static_cast<std::size_t>(GpuPass::Count)` | 計測するパスの数 |
