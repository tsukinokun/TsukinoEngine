# Changelog

All notable changes to TsukinoEngine are recorded here.
The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and the
project follows [Semantic Versioning](https://semver.org/spec/v2.0.0.html) — with the
carve-out for `Tsukino.Renderer` described under **API stability** in the README.

## [Unreleased]

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
