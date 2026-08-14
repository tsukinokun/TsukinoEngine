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

## License

This project is licensed under the [MIT License](LICENSE).
