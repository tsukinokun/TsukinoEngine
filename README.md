[日本語](README.ja.md) | **English**

<p align="center">
  <img src="image/Logo.png" width="480" alt="TsukinoEngine Logo">
</p>

<p align="center">
  <img src="https://img.shields.io/badge/C%2B%2B-20-blue.svg" alt="C++20">
  <img src="https://img.shields.io/badge/renderer-bgfx-107C10.svg" alt="Renderer: bgfx">
  <img src="https://img.shields.io/badge/platform-Windows-0078D6.svg" alt="Platform: Windows">
  <img src="https://img.shields.io/badge/build-Premake5-orange.svg" alt="Build: Premake5">
  <img src="https://img.shields.io/badge/ECS-EnTT-9cf.svg" alt="ECS: EnTT">
  <img src="https://img.shields.io/github/license/tsukinokun/TsukinoEngine" alt="License">
  <a href="https://qiita.com/tsukino_"><img src="https://img.shields.io/badge/Qiita-tsukino__-55C500?logo=qiita&logoColor=white" alt="Qiita"></a>
</p>

<p align="center">
  <a href="https://github.com/tsukinokun/TsukinoEngine/actions/workflows/build.yml"><img src="https://github.com/tsukinokun/TsukinoEngine/actions/workflows/build.yml/badge.svg" alt="Build"></a>
  <img src="https://img.shields.io/github/last-commit/tsukinokun/TsukinoEngine" alt="Last commit">
  <img src="https://img.shields.io/github/repo-size/tsukinokun/TsukinoEngine" alt="Repo size">
  <img src="https://img.shields.io/github/languages/top/tsukinokun/TsukinoEngine" alt="Top language">
</p>

# TsukinoEngine

A C++20 game engine built from scratch on SDL3 and bgfx, driven by an EnTT-based ECS architecture.

## Overview

TsukinoEngine is a personal game engine project targeting Windows. It combines a deferred renderer on [bgfx](https://github.com/bkaradzic/bgfx), an Entity Component System (ECS) built on [EnTT](https://github.com/skypjack/entt), and a set of engine modules (core, rendering, physics, audio, and asset pipeline) organized as independent static-library projects, all wired together with [Premake5](https://premake.github.io/).

## Features

- **ECS Architecture** — Gameplay logic is built on top of [EnTT](https://github.com/skypjack/entt) for cache-friendly, data-oriented entity management.
- **bgfx Renderer** — Custom deferred rendering pipeline (`Tsukino.Renderer`) on bgfx (Direct3D 11 on Windows). Shaders are written in bgfx's shader language (`.sc`) and compiled at build time.
- **Physics** — Integrated with [JoltPhysics](https://github.com/jrouwe/JoltPhysics) for rigid body simulation.
- **Visual Effects** — Particle and effect playback powered by [Effekseer](https://github.com/tsukinokun/Effekseer).
- **Audio** — Dedicated audio module (`Tsukino.Audio`) built on [miniaudio](https://github.com/mackron/miniaudio); `.wav` / `.mp3` / `.flac` are played as they are.
- **Asset Pipeline** — Model import via [Assimp](https://github.com/assimp/assimp) and texture decoding, mipmapping and block compression via [bimg](https://github.com/bkaradzic/bimg); textures are cached as KTX.
- **Serialization** — Scene and asset data serialization powered by [cereal](https://github.com/USCiLab/cereal).
- **Modular Build** — Each subsystem (`Tsukino.Core`, `Tsukino.GraphicsCommon`, `Tsukino.Engine`, `Tsukino.Renderer`, `Tsukino.Physics`, `Tsukino.Audio`, `Tsukino.BuiltIn`, `Tsukino.EngineIntegration`) is a separate Premake project, and `Tsukino.Sandbox` ties them together as a runnable executable.

## Getting Started

### Prerequisites

- Windows 10 or 11 (x64)
- Visual Studio 2022 with the "Desktop development with C++" workload. The engine
  builds as C++20; the vendored Jolt builds as C++17.
- Git
- About 2 GB of disk space and a slow first clone — the vendored submodules, and this
  repository's own sample assets, are large.
- Doxygen, only if you intend to run `generate-docs.bat`.

### Setup

This repository nests its submodules deeply enough to exceed the 260 character
path limit that Windows applies by default, which makes a recursive clone fail
part way through. Enable long paths first:

```bash
git config --global core.longpaths true
```

Clone the repository together with its submodules (hlslpp, entt, cereal, JoltPhysics, Effekseer, SDL, bgfx, bx, bimg, bgfx.cmake, miniaudio):

```bash
git clone --recurse-submodules https://github.com/tsukinokun/TsukinoEngine.git
```

If you already cloned the repository without submodules, fetch them with:

```bash
git submodule update --init --recursive
```

Generate the Visual Studio solution and open it:

```bash
open.bat
```

This runs `vendor\premake5.exe vs2022` to generate `.build/TsukinoEngine.sln` and opens it automatically. The NuGet package `AssimpCpp` is restored automatically by Visual Studio on the first build.

Once the solution is open, set **Tsukino.Sandbox** as the startup project and run it.
It starts a many-light showcase. The other sample scene is selected with a command-line
argument:

```
Tsukino.Sandbox.exe            many-light showcase (default; F1 cycles 0/1/16/64 point lights)
Tsukino.Sandbox.exe api        exercises the engine API and checks PrefabFactory
```

To build from the command line instead, use `build.bat` (`build.bat Release` for a
release build). It prints nothing on success and only the error lines on failure.

### Two things that will otherwise cost you an afternoon

**Call `Log::SetLogFile()` first.** `Tsukino::Core::Log` writes only to
`OutputDebugStringA` until you do, so running outside a debugger hides everything —
including fatal warnings like `Prefab file not found` and
`Unknown component type written in Prefab`, which do not stop execution. Every sample
starts with:

```cpp
Tsukino::Core::Log::SetLogFile("Logs/Tsukino.log");
```

**A Debug build only runs on the machine that generated the project files.** Debug
resolves engine assets through `TSUKINO_ENGINE_ROOT`, an absolute path baked in by
Premake, so that the built-in assets and tools do not have to be copied. Release builds
place everything next to the executable and are self-contained.

### Using the engine from your own game

Add this repository as a submodule and let its Premake helper supply the include, link
and redistributable settings, rather than copying them:

```lua
include "External/TsukinoEngine/Tools/premake/tsukino.lua"

workspace "MyGame"
    startproject "MyGame"
    location ".build"
    tsukino_workspace_defaults()

include "External/TsukinoEngine"

project "MyGame"
    kind "WindowedApp"
    language "C++"
    cppdialect "C++20"
    files { "src/**.cpp" }

    tsukino_link()             -- engine includes, libs and NuGet packages
    tsukino_release_payload()  -- built-in assets, tools and licence texts
```

`Tsukino.Sandbox` is skipped automatically when the engine is included this way, so a
consumer never builds the samples. The engine's public headers are self-contained, so
your project does not need a precompiled header to match the engine's.

## API stability

Version numbers follow semantic versioning, with one carve-out worth stating up front.

**Stable within 1.x** — `Tsukino.Core` (Registry, EventBus, Window, InputSystem, Path,
math), the built-in component definitions in `Tsukino.BuiltIn`, the Prefab JSON schema,
and `EngineAPI` / `GameSceneBase`. These are what a game is written against, and
breaking them is what a major version is for.

**Stable in practice, but not yet promised** — `Tsukino.Engine` (AssetManager,
AssetHandle, AssetRef, Scene, PrefabFactory), `Tsukino.Physics` (PhysicsWorld,
BodyHandle, SpringBone), `Tsukino.Audio`, `Tsukino.GraphicsCommon`, and the built-in
systems in `Tsukino.EngineIntegration`. These are not expected to change, but they have
not had the same scrutiny as the list above, so a 1.x release may still adjust a
signature here. Anything moved out of this list becomes stable.

**Expected to change within 1.x** — `Tsukino.Renderer`. `Renderer` has been split into
separate passes (`LightingPass`, `SkyPass`, `FogPass` and so on, reached through
accessors such as `GetFog()`); see the CHANGELOG. Those pass interfaces are new and may
still be adjusted, so code reaching into the renderer should expect to follow that.

### What the stable list does not cover

Three things are part of the contract whether or not they look like it, and 1.x will
not change them:

- **EnTT is part of the ECS API.** `Entity` is `entt::entity`, `Registry::View()`
  returns an `entt::view`, and `Registry::OnConstruct` / `OnDestroy` return EnTT sinks.
  A game therefore compiles against the EnTT version this engine vendors, and 1.x will
  not bump EnTT across a breaking release.
- **The platform layer is SDL3.** `Window` and `InputSystem` are built on SDL3, and the
  renderer on bgfx. The public headers do not include `<windows.h>`.
- **Failures are reported by logging and returning a default.** `FileSystem::ReadBinary`
  returns an empty buffer for a missing file, an empty file and a partial read alike;
  `AssetManager::Load` returns `AssetHandle::Invalid()` for every failure; a Prefab
  naming an unknown component logs a warning and instantiates without it. Call
  `Log::SetLogFile()` at startup or none of it is visible outside a debugger. Replacing
  this with a real error channel would change signatures everywhere, so it is a 2.0
  question, not a 1.x one.

## Demo
- [explanatory article(Qiita)](https://qiita.com/tsukino_/items/5d21a83fe3c2ad91828c)

## License
This project is licensed under the [MIT License](LICENSE).

Third-party components and their license texts are listed in
[THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).
