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

- Windows
- Visual Studio 2022 (with the "Desktop development with C++" workload)
- Git

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

## API stability

Version numbers follow semantic versioning, with one carve-out worth stating up front.

**Stable within 1.x** — `Tsukino.Core` (Registry, EventBus, Window, InputSystem, Path,
math), the built-in component definitions in `Tsukino.BuiltIn`, the Prefab JSON schema,
and `EngineAPI` / `GameSceneBase`. These are what a game is written against, and
breaking them is what a major version is for.

**Expected to change within 1.x** — `Tsukino.Renderer`. `Renderer` is currently a
single class holding the device, shadows, sky, water, tonemapping, the texture cache,
sprites and debug drawing, and it will be split into separate passes. Anything reaching
into `Renderer` directly should expect to follow that.

## License

This project is licensed under the [MIT License](LICENSE).

Third-party components and their license texts are listed in
[THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).
