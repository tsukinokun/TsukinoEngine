# Changelog

All notable changes to TsukinoEngine are recorded here.
The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and the
project follows [Semantic Versioning](https://semver.org/spec/v2.0.0.html) — with the
carve-out for `Tsukino.Renderer` described under **API stability** in the README.

## [Unreleased]

### Added

- **Audio buses (`AudioManager::SetBusVolume` / `GetBusVolume`).** `Play` takes an optional bus
  (`AudioBus`, 0–7; the game decides what each bus means, such as music or effects). A sound's
  volume is its own volume × its bus volume, with the master volume on top. Changing a bus volume
  applies at once to the sounds already playing on it, so a settings screen can turn all effects or
  the looping BGM up or down live. `AudioComponent::bus` picks the bus for component-driven sounds.

- **`Tsukino::IO::LoadOptional` / `TryLoadJson` / `TrySaveJson` (JSON files whose fields may be left out).**
  `LoadOptional(archive, name, value)` (`Core/IO/CerealOptional.hpp`) reads one named field and leaves
  `value` untouched when the field is missing, so a `load` written with it accepts older or partial
  files. `TryLoadJson<T>(path, key, out)` (`Core/IO/JsonFile.hpp`) reads into a copy of `out` and only
  assigns it back when the whole read succeeds; a missing or broken file logs a warning and keeps the
  defaults instead of throwing. `TrySaveJson` writes the same layout and creates missing folders. The
  built-in component serializers (Camera, Font, Model, Sprite, Transform, UIRect) now use
  `LoadOptional` instead of their own try/catch copies.

- **`AudioManager::SetVolume` (change the volume of a sound that is already playing).** Applies the
  new 0–1 volume to every instance of that asset currently playing (all overlaps), and does nothing
  when it isn't playing. Lets a settings screen turn the looping BGM up or down live instead of
  restarting it. The master volume still applies on top.

- **`AssetLoadQueue` (load assets on a worker thread).** Enqueue paths, `Start()`, then poll
  `Progress()` / `IsFinished()` from the main thread to drive a loading screen. One worker thread
  calls `AssetManager::Load` for each path (initializing COM for WIC first) and keeps going past
  failures; `.tmat` loads pull their textures in too. `TakeLoaded()` hands the finished handles back
  so the main thread can pre-upload textures with `RenderResources::GetTextureSRV` under a time
  budget. The destructor cancels and joins.

- **`CameraUtility` (screen ↔ world conversion for picking).** `ScreenPointToRay` builds a world-space
  ray through a pixel from `CameraComponent::invViewProjMatrix` (reverse-Z aware, perspective or
  orthographic), `WorldToScreenPoint` projects back to viewport pixels, and `IntersectPlane` /
  `IntersectRect` hit-test a plane or a rectangle placed by a world matrix (returning the hit UV, which
  is handy for tilting a hovered card toward the cursor). Header-only, in
  `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Camera/CameraUtility.hpp`.

- **Gamepad input (XInput) and an action-mapping layer.** `Tsukino::Input::Gamepad` polls one
  XInput slot per frame (radial dead zone, 0–1 triggers, auto-stopping vibration, reconnect checks
  once a second for empty slots). `InputSystem` owns four of them and adds `PollGamepads`,
  `IsButtonDown/Pressed/Released(GamepadButton)` (any pad), `GetLeftStick/GetRightStick/GetStick`,
  `SetGamepadVibration`/`StopGamepadVibration` and `GetLastInputDevice()`, which follows whichever
  device was touched last so games can switch on-screen prompts. `EngineAPI::Update` polls pads before
  the scene update and treats them as neutral while the window is unfocused; `ClearAllKeys` also stops
  vibration. `GamepadButton` includes `LeftTrigger`/`RightTrigger` and hysteresis-based
  `LeftStickUp/Down/Left/Right` for menu navigation. `InputActionMap` / `InputBinding` /
  `InputAxisBinding` bind keys, buttons and the wheel to game-defined action ids, and `InputNames.hpp`
  converts between names, codes and display labels for data-driven bindings. `xinput.lib` is linked
  from `Gamepad.cpp`.

- **`DrawCommand::shadowPipeline` (per-draw shadow pipeline override).** Objects whose
  vertex shader moves vertices (wind-swayed foliage, procedurally placed grass) used to cast
  shadows from the fixed `ShadowMapStatic.vs.hlsl`, so the shadow stayed still, or they had
  to turn shadows off. A command can now supply its own shadow pipeline, built from a matching
  shadow VS plus the built-in `ShadowMap.ps.hlsl`, and `ExecuteShadow` uses it instead. b1 and
  the game's b12 constant buffer are bound during shadow draws as well, so the shadow VS sees
  the same parameters. Cutout and dithered fade keep working through the built-in shadow PS.
  `nullptr` (the default) keeps the built-in pipelines.

- **`DrawCommand::ditherFade` (dithered "screen-door" fade).** Objects that pop in at the
  edge of a draw distance (or switch LOD) can now fade in without going translucent.
  `ditherFade` is the visible fraction (default `1.0` = drawn normally). The executor
  passes it in `CBufferTransform::motionFlags.y` and now also binds b1 to the pixel shader.
  `GBuffer.ps.hlsl` and `ShadowMap.ps.hlsl` discard pixels against a 4×4 Bayer threshold
  (`Dither.hlsli` → `ClipDither`), so the object, its depth and its shadow fade together and
  stay in the deferred path. There is no TAA, so the pattern is visible while fading; put the
  fade band where fog already softens it.

- **Alpha-tested (cutout) shadows.** The shadow pass used to ignore `alphaCutoff`, so
  cutout geometry such as leaf cards cast solid quad shadows. `ExecuteShadow` now binds the
  command's albedo texture (t0), sampler (s0) and material constants (b2) to the pixel
  shader. The shadow VSs pass the UV through, and `ShadowMap.ps.hlsl` clips with the same
  test as `GBuffer.ps.hlsl`. Commands without material constants get a zero material
  (cutoff 0), and opaque materials skip the texture fetch, so ordinary shadows cost the same.

- **`UITween::unscaledTime` and `UnscaledTimeContext`.** Games commonly pass
  `deltaTime = 0` to every system while paused (or shrink it during slow motion), which
  also froze menu tweens and button feedback. A scene can now publish the real elapsed time
  each frame with `registry.SetContext<UnscaledTimeContext>(realDeltaTime)`. `UITweenSystem`
  advances tweens flagged `unscaledTime` and all `UIButtonFeedbackComponent`s with it. Without
  the context, everything advances with the system's `deltaTime` exactly as before.

- **`Tsukino::Core::Math::Spring::Step`** (`Spring.hpp`). Advances a damped harmonic
  oscillator (position, velocity → target) by an exact analytic step, for `float` and vector
  types. Unlike an implicit-Euler spring, the motion is identical at any frame rate and stays
  stable for large `deltaTime`. Use it for camera follow/shake or any value that should
  overshoot and settle.

- **Screen-UI motion: `UIMotionComponent`, `UICanvasGroupComponent`, `UITweenComponent`,
  `UIButtonFeedbackComponent` and `UITweenSystem`.** Until now, UI could only change
  instantly. Every system that animated something (pop-ins, fades, hover feedback) had to
  overwrite `TransformComponent` / colours itself, which fought the systems that already
  write those values every frame.
  - `UIMotionComponent` is a visual-only offset / pivoted scale / rotation.
    `TransformSystem` multiplies it after the local matrix when it builds `worldMatrix`, and
    never touches `position` / `scale` / `localMatrix` / `dirty`. Children, text size (taken
    from the world scale) and pointer hit tests (`InteractionSystem` uses `worldMatrix`)
    follow it automatically. `source` mirrors another entity's motion, for example a label
    that is a separate entity from its button, or the loose parts of a panel.
  - `UICanvasGroupComponent::alpha` multiplies the alpha of the entity and its descendants,
    walking ancestors the same way as `UIVisibilityComponent`. It applies to screen sprites,
    text and its outline, and screen models (including the `anchor`'s ancestors). Nearly
    transparent groups are skipped by the renderers and by the pointer hit test.
  - `UITweenComponent` holds `UITween` entries. Each entry has a property (offset / scale /
    rotation / alpha), from/to, duration, delay, ease, loop (`Once` / `Loop` / `PingPong`),
    `repeatDelay`, `holdAtEnd` and a replaceable `tag`. Play and stop them through
    `UITweenUtility::Play / Stop / IsPlaying`. Tweens on the same property are combined:
    scale and alpha multiply, offset and rotation add.
  - `UIButtonFeedbackComponent` reads `PointerTargetComponent` to grow the button on hover,
    shrink it while pressed and punch it on click. It does nothing while `interactable` is
    false.
  - `UITweenSystem` drives all of the above. Place it after `InteractionSystem` and after
    the systems that start tweens, and before `TransformSystem`.
  - The Sandbox's `SceneSample1` shows button feedback on the sprite and a breathing tween
    on the text.

- **`Tsukino::Core::Math::Easing`** (`Easing.hpp`). It provides the standard easing curves
  (quad / cubic / back / elastic / bounce / sine, plus a decaying `Punch`),
  `Evaluate(EaseType, t)` and a frame-rate independent `Damp`.

- **`tsukino_icon(iconPath)` premake helper for the application icon.** Call it in the
  executable's project after `location`; it generates `TsukinoAppIcon.rc` next to the
  project (`TSUKINO_APP_ICON ICON "<abs path>"`) and adds it to `files`, so the `.ico`
  becomes the exe's icon in Explorer. `Window::Create` now loads that resource for
  `hIcon` / `hIconSm` (sized via `SM_CXICON` / `SM_CXSMICON`), so the title bar,
  taskbar and Alt+Tab match. Projects that do not call it keep the Windows default icon.
  A missing `.ico` only warns at premake time. The `.rc` is rewritten only when its
  content changes, so regenerating does not force a relink. The Sandbox uses it with
  `image/Icon.ico` (the face from `image/Logo.png`).

- **`PhysicsWorld::SetUnitsPerMeter` / `PhysicsSystem::SetUnitsPerMeter`.** Jolt's defaults
  (gravity 9.81, a 2 cm speculative contact distance, 2 cm penetration slop, sleep and
  restitution velocity thresholds) assume 1 unit = 1 m, while the engine's own convention is
  1 unit = 1 cm. In a cm scene gravity was 1/100 of real (samples compensated with
  `gravityFactor`) and contacts were only created once bodies were 1/100 as close, so thin or
  fast bodies sank several units into floors before being pushed out. Calling
  `SetUnitsPerMeter(100)` scales gravity and every length/velocity tolerance in
  `JPH::PhysicsSettings` from Jolt's defaults (not from the current values, so repeated calls
  do not compound). The default stays 1, so existing scenes are unchanged.

- **`PhysicsWorld::MoveKinematic`**, a wrapper over `BodyInterface::MoveKinematic`.

- **Jolt trace and assert messages now reach `Log`.** Jolt's default `Trace` handler wrote
  nothing and stopped on a breakpoint, so a Debug build simply crashed with `0x80000003`
  and no hint of the cause. `Trace` now goes to `Log::Warn` and failed asserts to
  `Log::Error` (asserts still break into the debugger).

- **`FogComponent` can anchor distance fog to a custom world point instead of the
  camera.** New fields `useCustomDistanceOrigin` / `distanceOrigin` (not serialized —
  runtime-only, meant to be written every frame by app-layer code, same convention as
  `density`). With a TPS-style camera that orbits around a followed character, distance
  fog measured from the camera made a fixed-position object drift in and out of the fog
  band as the camera swung around it, even though its distance to the character never
  changed. `Fog.ps.hlsl` now computes the distance-fog term from `originParams.xyz` when
  `originParams.w > 0.5`, falling back to `cameraPos` otherwise; height fog and the noise
  wobble are unaffected and still integrate along the camera's view ray. `CBufferFog`
  gained a 7th `float4` (`originParams`) appended at the end, so existing byte offsets are
  unchanged.

### Changed

- **`DynamicFontAtlas` rasterizes glyphs at stepped sizes.** Text whose scale animates (button hover,
  popping numbers) used to hit a new integer pixel size almost every frame, re-rasterizing every
  glyph with DirectWrite and uploading it to the atlas, which showed up as hitches. Sizes are now
  rounded up to a geometric ladder (`kRasterSizeBase` = 32 px, `kRasterSizeStep` = 2^(1/4)) and
  drawn scaled down by at most ~16%, so an animation touches only a few sizes and reuses the cache.

- **`CBufferMaterial` (b2) now lives in one place.** The struct was hand-written in
  `GBuffer.ps.hlsl`, `Model.ps.hlsl` and `Sprite.ps.hlsl`; all three now
  `#include "Material.hlsli"`, which also holds `EvaluateRimGlow`. The rim-glow formula
  had been copy-pasted into the forward and deferred paths and had already drifted apart.

  `alphaCutoff` becomes a named field instead of riding in `rimParams.z`. That removes
  the ordering trap in `ModelSystem`, where the cutoff had to be written *after* the
  rim-glow block or a highlighted model silently lost its alpha test. `rimColor` and
  `rimParams` keep their meaning and their byte offsets.

- **`Renderer` is split into passes** (QUALITY_REPORT C-4). `Renderer` now owns the
  parts and runs them in order; `Renderer.cpp` went from 2,556 to about 420 lines.
  Rendering output is unchanged: frame stats and captured frames were compared against
  the previous build after every step.

  Settings move from `Renderer` to the class that uses them, reached through an accessor.
  This is a breaking change to the `Tsukino.Renderer` API, as allowed by the carve-out in
  the README.

  | Before | After |
  |---|---|
  | `PushDrawCommand` / `AllocMaterial` / `AllocMaterialData` | `GetDrawQueue().Push` / `AllocMaterial` / `AllocMaterialData` |
  | `GetPipelineFactory` / `GetSampler` / `GetTextureSRV` / `GetWhiteTextureSRV` / `GetFlatNormalTextureSRV` / `GetPrimitiveMesh` / `GetCommonStatesTK` / `CreateSpriteFont` / `CreateSpriteBatch` | `GetResources().` same name |
  | `SetWorldCameraMatrix` / `SetOverlayCameraMatrix` / `AdvanceFrameTime` | `GetFrameConstants().SetWorldCamera` / `SetOverlayCamera` / `AdvanceTime` |
  | `DrawDebugLine` / `DrawDebugTriangle` / `FlushDebugDraw` | `GetDebugDraw().DrawLine` / `DrawTriangle` / `Flush` |
  | `SetDirectionalLight` / `SetLights` | `GetLighting().SetDirectionalLight` / `SetLights` |
  | `SetSkyParameters` / `SetSkyPipeline` | `GetSky().SetParameters` / `SetPipeline` |
  | `RequestIBLRecapture` | `GetIBL().RequestRecapture` |
  | `SetFogParameters` / `SetFogEnabled` | `GetFog().SetParameters` / `SetEnabled` |
  | `SetMotionBlurParameters` / `SetMotionBlurEnabled` | `GetMotionBlur().SetParameters` / `SetEnabled` |
  | `SetAmbientParticleParameters` / `SetAmbientParticleEnabled` | `GetAmbientParticles().SetParameters` / `SetEnabled` |

  `Initialize`, `Render`, `Resize`, `SetClearColor`, `GetDevice`, `GetContext`,
  `GetFrameStats` and the VSync accessors are unchanged.

### Fixed

- **Textures were displayed too bright (sRGB lost in the DDS cache).** `TextureImporter` forced sRGB
  on load but saved the cache with the legacy DDS header, which cannot store sRGB, so every PNG/JPG
  came back as UNORM. Colors were then gamma-encoded twice on the way to the sRGB back buffer
  (a 172 red showed as 215). The cache is now written with the DX10 header so the sRGB format
  survives. Data textures whose file name ends in `_n`, `_normal`, `_nrm`, `_mr`, `_orm`, `_ao`,
  `_metallic`, `_roughness`, `_height`, `_disp` or `_mask` are kept linear. Delete `Cache/` once
  so existing textures are re-imported.

- **Kinematic bodies push instead of teleporting.** `PhysicsSystem` moved every Kinematic
  body with `SetPositionAndRotation` and then also set a velocity derived from last frame's
  position, so during `Step` the body travelled the same distance a second time and was
  snapped back on the next frame. Because a teleport is not a contact, each frame started
  with the Kinematic body already overlapping whatever it was pushing; thin bodies (coins
  in front of a pusher) were resolved downward and ended up underneath or behind it. The
  sync now calls `PhysicsWorld::MoveKinematic`, which sets the velocity that reaches the
  Transform position at the end of the step. A Kinematic body that is moved a long way in
  one frame now sweeps through and pushes what is in between, instead of appearing there.
  The per-entity `m_prevPositions` map is gone.

- **The Jolt world no longer overflows with a few hundred touching bodies.** It was created
  with 1,024 body pairs and 1,024 contact constraints. A bed of ~230 small bodies, each
  touching several neighbours, exceeded that and Jolt stopped on an assert in Debug
  (Release silently dropped contacts). Limits are now 4,096 bodies and 16,384 body pairs /
  contact constraints, and the temp allocator grows from 10 MB to 32 MB to hold them.

- **FBX Phong materials are converted instead of discarded.** `ModelImporter` asked assimp
  only for `metallicFactor` / `roughnessFactor` and ignored the return value, so every FBX —
  which is to say almost every model, since FBX carries Phong materials and has no PBR
  factors — silently baked the same `metallic 0 / roughness 0.5 / specular 0.5`. Every
  character in a project came out with identical surface parameters, and nothing said so.

  The importer now checks the return value and, when no PBR factors are present, derives
  roughness from `ShininessExponent` with the standard Blinn-Phong mapping
  `roughness = sqrt(2 / (shininess + 2))`, clamped to [0.04, 1]. Assets that do carry PBR
  factors are never touched. Each material logs which of the three paths it took — PBR
  values, Phong conversion, or defaults — along with the numbers, so a material that falls
  back is visible instead of merely looking plausible.

  `SpecularFactor` and `SpecularColor` are deliberately not mapped to `specular`. They are
  Phong highlight intensities, not the dielectric F0 this engine's `specular` controls
  (`F0 = 0.08 * specular`); FBX commonly carries 1.0 there, which doubles F0 from 0.04 to
  0.08 and makes skin and cloth shine. `metallic` stays 0 for the same reason — Phong has no
  notion of metal.

  **Cached `.tsm` files do not notice importer changes** — reimport is decided by source
  mtime alone — so `Cache/` has to be deleted once for this to take effect.

- **ACES is no longer applied twice.** `GBuffer.ps.hlsl` and `Model.ps.hlsl` ran the
  albedo through `ACES()` before lighting it, and `Tonemap.ps.hlsl` then tonemapped the
  finished HDR frame. Albedo is a reflectance, not a radiance, so tonemapping it is wrong
  on its own terms, and doing it twice bent every material away from its authored colour.
  The albedo now goes into the BRDF as read, and `Tonemap.ps.hlsl` holds the single
  remaining `ACES()` in the pipeline. The dead copy in `PBR.hlsli` is gone.

  **Material appearance changes.** Removing the curve darkens mid and low albedo values
  and, below about 0.065, *reduces* saturation rather than raising it — so colours
  authored against the old pipeline will need retuning. Anything that never went through
  albedo is untouched, which was verified by measurement: the sky (`Sky.ps.hlsl` has no
  `ACES`) and the UI (drawn to the back buffer after tonemapping) came back bit-identical
  across builds, while ground patches moved as predicted.

  The exposure multiplier in `Tonemap.ps.hlsl` is now the named `kExposure` with its
  provenance written down: 0.6 is an empirical value inherited from the old look, not a
  physical one. `Sky.ps.hlsl` also loses a `skyColor *= 1.0f;` that was labelled
  "exposure correction" and multiplied by one.

- **G-Buffer 0 stores albedo as sRGB.** Albedo textures arrive as `BC3_UNORM_SRGB`, which
  spends its 8 bits where the eye can see them. Decoding that to linear and writing it
  back into a linear `R8G8B8A8_UNORM` target threw the source precision away in the darks.
  The target is now `R8G8B8A8_UNORM_SRGB`, so the hardware encodes on write and decodes on
  read. Views are created with a null desc and inherit the format, the conversion skips
  alpha, and nothing blends into that target, so this is the one format line — the
  lighting pass is unchanged.

- **`CBufferMaterial` (b2) no longer feeds the shader shifted PBR values.**
  `hlslpp::float3` occupies a full 16-byte SIMD register, while HLSL's `float3` is
  12 bytes and lets the next scalar share its 16-byte row. `emissive` was the only
  `float3` in the file followed by scalars, so `metallic`, `roughness` and `specular`
  sat 4 bytes further along in C++ than the shader expected: the shader read the
  material's `metallic` as `roughness`, its `roughness` as `specular`, and picked up
  the unused fourth lane of `emissive` as `metallic`. Both structs were 80 bytes, so
  a size check could never have caught it.

  `Material.hlsli` now declares an explicit `emissivePad` after `emissive`, and
  `ConstantBuffer.hpp` pins the size *and* the offsets of `metallic`, `alphaCutoff`,
  `rimColor` and `rimParams` with `static_assert`. Material appearance changes, because
  authored metallic / roughness / specular values finally reach the shader.

- **`AudioManager` can stop sounds.** Playback used fire-and-forget `WaveBank::Play`, so
  `Stop` only logged a warning, `IsPlaying` always returned `false`, and the `isLoop`
  argument was ignored. Each `Play` now creates a `SoundEffectInstance` that the manager
  keeps until it finishes, which makes all three work:
  - `Stop(asset)` stops every playing instance of that asset immediately.
  - `IsPlaying(asset)` reports whether any instance of it is playing.
  - `Play(asset, true)` loops until stopped. `AudioComponent::loop` therefore loops too.
  - `StopAll` stops the tracked instances instead of suspending and resuming the engine.
- `AudioManager::Play` applied the master volume twice (once to the sound, once to the
  audio engine). It is now applied only by the engine, so sounds are louder than before
  whenever the master volume is below 1.

### Removed

- `Renderer::UpdateSceneBuffer`, `SetShadowPipeline` and `SetMotionBlurPipeline`. They
  had no callers outside the renderer.

## [1.0.0] — 2026-09-05

First tagged release. Development started 2026-02-15.

### Added

- **ECS** built on [EnTT](https://github.com/skypjack/entt). `Registry` wraps entity and
  component access, adds deferred destruction (`QueueDestroy` / `FlushDestroyQueue`) so
  systems can delete during iteration, and exposes EnTT's construct/destroy signals for
  resources whose ownership sits outside the ECS.
- **Prefabs.** Entities are described in JSON and instantiated by `PrefabFactory`.
  `EntityRef` and `AssetRef` fields are resolved after instantiation, so prefabs can
  reference other entities and assets by name and path rather than by runtime handle.
- **Deferred DirectX 11 renderer.** G-Buffer, directional and point/spot lighting,
  shadow maps, sky atmosphere, water, fog, tonemapping and motion blur, plus forward
  passes for transparency (with a depth pre-pass), sprites and text.
- **Physics** through [Jolt](https://github.com/jrouwe/JoltPhysics), wrapped so that Jolt
  types do not appear in any public header. Rigid bodies, a character controller, and
  spring bones for secondary animation.
- **Effects** through Effekseer, driven by `EffectComponent`. The engine tracks a
  [fork](https://github.com/tsukinokun/Effekseer) of
  [Effekseer](https://github.com/effekseer/Effekseer); each release tags the fork so
  that older releases stay clonable.
- **Audio** through DirectXTK, with wave bank import.
- **Asset pipeline.** Importers convert source assets into a cache, loaders read the
  cache, and `AssetManager` hands out handles derived from the asset path, so the same
  path always resolves to the same handle and is never loaded twice.
- **Animation.** Skeletal animation with blending, plus root motion and spring bone
  integration.
- **Modular build.** Eight Premake projects (`Tsukino.Core`, `GraphicsCommon`, `Engine`,
  `Renderer`, `Physics`, `BuiltIn`, `EngineIntegration`, `Audio`) that build standalone
  or as a submodule of a game repository.
- **Sandbox** with five runnable scenes, chosen by a command-line argument
  (`Tsukino.Sandbox.exe [blocks|water|lights|api]`, default a jump game): a jump game,
  block breaking, a water game, a many-light showcase, and a scene that exercises the
  engine API and checks `PrefabFactory` round-trips.
- **Consumer build helper.** `Tools/premake/tsukino.lua` gives a game repository
  `tsukino_workspace_defaults()`, `tsukino_link()` and `tsukino_release_payload()`, so
  the include, link and redistributable settings live in one place instead of being
  copied into each consumer. The Sandbox goes through the same functions.
- **Redistributable payload.** Release builds place `LICENSE`,
  `THIRD_PARTY_NOTICES.md`, the built-in assets and the external tools next to the
  executable.
- **CI.** GitHub Actions builds Debug and Release on `windows-latest` from a clean
  checkout, deliberately without pre-provisioning MSBuild, so that the documented
  first-time setup is what gets tested. A separate `Tsukino.HeaderCheck` project
  compiles every public header on its own, without a precompiled header, so that
  headers stay usable by a consumer that does not share the engine's PCH.
- **Generated API documentation.** `generate-docs.bat` produces an API index, a
  component and Prefab field reference, and per-module digests under `Docs/`.

### Fixed

Hardening carried out before this release. The full record, including what was measured
and what was deliberately deferred, is in [Docs/QUALITY_REPORT.md](Docs/QUALITY_REPORT.md).

- **Destruction order.** `Scene` and `EngineIntegration` destroyed their members in
  dependency order rather than reverse dependency order, so an `EventBus` could outlive
  the systems subscribed to it and be unsubscribed from after it was gone. Both now
  declare members in reverse dependency order, with the reasoning recorded in place.
- **Resource leaks.** Jolt bodies and Effekseer playback handles were never released on
  entity destruction, so `cMaxBodies` was reachable in a long session. Cleanup moved to
  EnTT destroy signals, which fire on every destruction path.
- **Iterator invalidation** when destroying entities during a view iteration.
- **Renderer correctness.** Window resize was unimplemented, the transparent pass was
  never executed, `HRESULT`s from depth resource creation went unchecked, and input stuck
  down after Alt+Tab.
- **Layering.** `Tsukino.Renderer` depended on `Tsukino.EngineIntegration`. The
  dependency now runs through `IPostWorldPass`, an interface the lower layer owns.
- **Asset handles are deterministic.** Handles were random per process, which meant the
  same asset could be loaded twice and no handle could be serialized. They are now
  derived from the normalized asset path.
- **Draw command lifetime.** `DrawCommand` pointed into per-system buffers that were
  cleared at a different point in the frame than the commands themselves. Both now live
  in `DrawCommandQueue` and are discarded together.
- **Unchecked failures.** Eight asset importers ignored the result of directory creation
  and carried on writing into a path that did not exist, surfacing far from the cause as
  a missing cache entry.
- **Public headers no longer leak names into the global namespace.** The `i8`…`u64`
  aliases moved from the global namespace into `Tsukino`, and the file-scope
  `using Microsoft::WRL::ComPtr;` in three renderer headers moved inside
  `Tsukino::Renderer`. Both reached every consumer translation unit and could not have
  been withdrawn after 1.0.
- **Public headers are self-contained.** They previously relied on the engine's own
  forced `pch.h`, which `tsukino_link()` does not give a consumer, so a game without a
  matching precompiled header failed to compile. `Tsukino/Core/WindowsLean.hpp` now
  centralises the guarded `<windows.h>` include, and `Tsukino.HeaderCheck` compiles all
  179 public headers individually with no PCH to keep it that way.
- **`PhysicsSystem` had a different public API in Debug and Release.**
  `SetDebugDrawEnabled` and two members were behind `#ifdef`, so code that compiled in
  Debug failed in Release and `sizeof(PhysicsSystem)` differed between configurations.
- **`DebugFeatures.hpp` defined build flags unconditionally**, so a consumer could not
  turn the frame profiler or the stress test off without editing engine source, and
  defining the same names collided. They are guarded, with `TSUKINO_DISABLE_*` opt-outs.
- **Physics thread pool could request 4,294,967,295 threads.**
  `std::thread::hardware_concurrency() - 1` underflowed when the count was unavailable.
- **Texture paths baked into a model are resolved relative to the model.** FBX files
  keep the absolute path from the machine that authored them, which never resolves
  anywhere else; the importer now falls back to the same filename next to the model.
- **A sample loaded assets that had been deleted.** `DeferredLightSampleScene` still
  referenced a removed demo's models, and another scene referenced `Wall.fbx` where the
  file on disk is `wall.fbx` — which worked only because Windows ignores case.
- **Running a sample no longer dirties the working tree.** The scene that exercises
  `PrefabFactory` writes prefabs into the asset tree; those generated files were
  committed and are now ignored.
- **Sample types are no longer published as engine API.** The digest generator excluded
  `Tsukino.Sandbox` in a comment but not in the filter, so a test-only component defined
  inside a sample's `.cpp` appeared in the public API index.

### Known limitations

- `Renderer` is a single large class covering every pass. Splitting it is planned for a
  1.x release and will change its public API — see **API stability** in the README.
- No material sorting and no frustum culling; every submitted command is drawn.
- No unit tests. CI builds both configurations and compiles every public header
  standalone, but nothing asserts runtime behaviour; the sample scenes are still checked
  by running them and reading the log.
- `Log` writes only to `OutputDebugStringA` until `Log::SetLogFile()` is called, so
  warnings are invisible when running outside a debugger. There is no level filter.
- Debug builds bake the engine's source path in as `TSUKINO_ENGINE_ROOT` so that assets
  resolve without copying. A Debug build therefore only runs on the machine that
  generated the project files; Release builds are self-contained.
- `AssetManager` has no `Unload`. Assets live until the manager is destroyed.

[Unreleased]: https://github.com/tsukinokun/TsukinoEngine/compare/v1.0.0...HEAD
[1.0.0]: https://github.com/tsukinokun/TsukinoEngine/releases/tag/v1.0.0
