[日本語](README.ja.md) | **English**

<p align="center">
  <img src="image/Logo.png" width="480" alt="TsukinoEngine Logo">
</p>

<p align="center">
  <img src="https://img.shields.io/badge/C%2B%2B-20-blue.svg" alt="C++20">
  <img src="https://img.shields.io/badge/DirectX-11-107C10.svg" alt="DirectX 11">
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

A C++20 / DirectX 11 game engine built from scratch, driven by an EnTT-based ECS architecture.

## Overview

TsukinoEngine is a personal game engine project targeting Windows. It combines a DirectX 11 renderer, an Entity Component System (ECS) built on [EnTT](https://github.com/skypjack/entt), and a set of engine modules (core, rendering, physics, audio, and asset pipeline) organized as independent static-library projects, all wired together with [Premake5](https://premake.github.io/).

## Features

- **ECS Architecture** — Gameplay logic is built on top of [EnTT](https://github.com/skypjack/entt) for cache-friendly, data-oriented entity management.
- **DirectX 11 Renderer** — Custom rendering pipeline (`Tsukino.Renderer`) built directly on the DirectX 11 API.
- **Physics** — Integrated with [JoltPhysics](https://github.com/jrouwe/JoltPhysics) for rigid body simulation.
- **Visual Effects** — Particle and effect playback powered by [Effekseer](https://github.com/tsukinokun/Effekseer).
- **Audio** — Dedicated audio module (`Tsukino.Audio`) built on DirectXTK.
- **Asset Pipeline** — Model import via [Assimp](https://github.com/assimp/assimp) and texture processing via [DirectXTex](https://github.com/microsoft/DirectXTex).
- **Serialization** — Scene and asset data serialization powered by [cereal](https://github.com/USCiLab/cereal).
- **Modular Build** — Each subsystem (`Tsukino.Core`, `Tsukino.GraphicsCommon`, `Tsukino.Engine`, `Tsukino.Renderer`, `Tsukino.Physics`, `Tsukino.Audio`, `Tsukino.BuiltIn`, `Tsukino.EngineIntegration`) is a separate Premake project, and `Tsukino.Sandbox` ties them together as a runnable executable.

## Getting Started

### Prerequisites

- Windows 10 or 11 (x64)
- Visual Studio 2022 with the "Desktop development with C++" workload. The engine
  builds as C++20; the vendored DirectXTex and Jolt build as C++17.
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

Clone the repository together with its submodules (hlslpp, entt, DirectXTex, cereal, JoltPhysics, Effekseer):

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

This runs `vendor\premake5.exe vs2022` to generate `.build/TsukinoEngine.sln` and opens it automatically. NuGet packages (`AssimpCpp`, `directxtk_desktop_win10`) are restored automatically by Visual Studio on the first build.

Once the solution is open, set **Tsukino.Sandbox** as the startup project and run it.
It starts a small jump game. The other sample scenes are selected with a command-line
argument:

```
Tsukino.Sandbox.exe            a jump game (default)
Tsukino.Sandbox.exe blocks     block breaking
Tsukino.Sandbox.exe water      a water-surface minigame
Tsukino.Sandbox.exe lights     many-light showcase (F1 cycles 0/1/16/64 point lights)
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

**Expected to change within 1.x** — `Tsukino.Renderer`. `Renderer` is currently a
single class holding the device, shadows, sky, water, tonemapping, the texture cache,
sprites and debug drawing, and it will be split into separate passes. Anything reaching
into `Renderer` directly should expect to follow that.

### What the stable list does not cover

Three things are part of the contract whether or not they look like it, and 1.x will
not change them:

- **EnTT is part of the ECS API.** `Entity` is `entt::entity`, `Registry::View()`
  returns an `entt::view`, and `Registry::OnConstruct` / `OnDestroy` return EnTT sinks.
  A game therefore compiles against the EnTT version this engine vendors, and 1.x will
  not bump EnTT across a breaking release.
- **`Window` is Win32.** `Tsukino/Core/Window.hpp` pulls in `<windows.h>` (through
  `Tsukino/Core/WindowsLean.hpp`, which sets `NOMINMAX` and `WIN32_LEAN_AND_MEAN`
  first), and `MessageCallback` takes `UINT` / `WPARAM` / `LPARAM`. This engine is
  Windows and DirectX 11 only; the Window API is not going to become portable in 1.x.
- **Failures are reported by logging and returning a default.** `FileSystem::ReadBinary`
  returns an empty buffer for a missing file, an empty file and a partial read alike;
  `AssetManager::Load` returns `AssetHandle::Invalid()` for every failure; a Prefab
  naming an unknown component logs a warning and instantiates without it. Call
  `Log::SetLogFile()` at startup or none of it is visible outside a debugger. Replacing
  this with a real error channel would change signatures everywhere, so it is a 2.0
  question, not a 1.x one.

## License

This project is licensed under the [MIT License](LICENSE).

Third-party components and their license texts are listed in
[THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).
