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
- **Effects** through [Effekseer](https://github.com/effekseer/Effekseer), driven by
  `EffectComponent`.
- **Audio** through DirectXTK, with wave bank import.
- **Asset pipeline.** Importers convert source assets into a cache, loaders read the
  cache, and `AssetManager` hands out handles derived from the asset path, so the same
  path always resolves to the same handle and is never loaded twice.
- **Animation.** Skeletal animation with blending, plus root motion and spring bone
  integration.
- **Modular build.** Eight Premake projects (`Tsukino.Core`, `GraphicsCommon`, `Engine`,
  `Renderer`, `Physics`, `BuiltIn`, `EngineIntegration`, `Audio`) that build standalone
  or as a submodule of a game repository.
- **Sandbox** with four runnable samples: a jump game, block breaking, a water game and
  a many-light showcase.
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

### Known limitations

- `Renderer` is a single large class covering every pass. Splitting it is planned for a
  1.x release and will change its public API — see **API stability** in the README.
- No material sorting and no frustum culling; every submitted command is drawn.
- No unit tests and no CI.
- `Log` writes only to `OutputDebugStringA` until `Log::SetLogFile()` is called, so
  warnings are invisible when running outside a debugger.

[Unreleased]: https://github.com/tsukinokun/TsukinoEngine/compare/v1.0.0...HEAD
[1.0.0]: https://github.com/tsukinokun/TsukinoEngine/releases/tag/v1.0.0
