**日本語** | [English](README.md)

<p align="center">
  <img src="image/Logo.png" width="480" alt="TsukinoEngine ロゴ">
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

EnTTベースのECSアーキテクチャを採用した、C++20 / DirectX 11製の自作ゲームエンジンです。

## 概要

TsukinoEngineは、Windows向けの個人開発ゲームエンジンです。DirectX 11レンダラーと、[EnTT](https://github.com/skypjack/entt)によるECS(Entity Component System)を軸に、コア・描画・物理・オーディオ・アセットパイプラインといった各エンジンモジュールを独立した静的ライブラリとして構成し、[Premake5](https://premake.github.io/)でビルドします。

## 特徴

- **ECSアーキテクチャ** — [EnTT](https://github.com/skypjack/entt)を用いたデータ指向設計により、キャッシュ効率の高いエンティティ管理を実現。
- **DirectX 11レンダラー** — DirectX 11 APIを直接利用した独自の描画パイプライン(`Tsukino.Renderer`)。
- **物理演算** — [JoltPhysics](https://github.com/jrouwe/JoltPhysics)を統合し、剛体シミュレーションに対応。
- **視覚エフェクト** — [Effekseer](https://github.com/tsukinokun/Effekseer)によるパーティクル・エフェクト再生。
- **オーディオ** — DirectXTKをベースとした専用オーディオモジュール(`Tsukino.Audio`)。
- **アセットパイプライン** — [Assimp](https://github.com/assimp/assimp)によるモデル読み込みと、[DirectXTex](https://github.com/microsoft/DirectXTex)によるテクスチャ処理。
- **シリアライズ** — [cereal](https://github.com/USCiLab/cereal)を用いたシーン・アセットデータのシリアライズ。
- **モジュール構成** — `Tsukino.Core` / `Tsukino.GraphicsCommon` / `Tsukino.Engine` / `Tsukino.Renderer` / `Tsukino.Physics` / `Tsukino.Audio` / `Tsukino.BuiltIn` / `Tsukino.EngineIntegration` の各サブシステムを個別のPremakeプロジェクトとして分離し、`Tsukino.Sandbox`が実行可能ファイルとしてそれらを統合します。

## セットアップ

### 必要要件

- Windows
- Visual Studio 2022（「C++によるデスクトップ開発」ワークロード）
- Git

### 手順

このリポジトリはサブモジュールの階層が深く、Windowsが既定で課すパス長制限（260文字）を
超えるため、そのままではクローンが途中で失敗します。先に長いパスを有効にしてください。

```bash
git config --global core.longpaths true
```

サブモジュール（hlslpp, entt, DirectXTex, cereal, JoltPhysics, Effekseer）を含めてクローンします。

```bash
git clone --recurse-submodules https://github.com/tsukinokun/TsukinoEngine.git
```

サブモジュールなしで既にクローン済みの場合は、以下のコマンドで取得してください。

```bash
git submodule update --init --recursive
```

`open.bat`を実行し、Visual Studioのソリューションを生成・起動します。

```bash
open.bat
```

内部で`vendor\premake5.exe vs2022`が実行され、`.build/TsukinoEngine.sln`が生成された後、自動的に開かれます。NuGetパッケージ（`AssimpCpp`, `directxtk_desktop_win10`）は、初回ビルド時にVisual Studioが自動的に復元します。

ソリューションを開いたら、**Tsukino.Sandbox**をスタートアッププロジェクトに設定して実行してください。

## APIの安定性

バージョン番号はセマンティックバージョニングに従いますが、1つだけ先に断っておく点があります。

**1.x の間は安定** — `Tsukino.Core`（Registry / EventBus / Window / InputSystem / Path /
数学）、`Tsukino.BuiltIn` の組み込みコンポーネント定義、Prefab JSON のスキーマ、
`EngineAPI` と `GameSceneBase`。ゲームを書くときに触るのはこの範囲で、
ここを壊すのはメジャーバージョンの仕事だと考えています。

**1.x の間も変わりうる** — `Tsukino.Renderer`。現在の `Renderer` はデバイス・シャドウ・
スカイ・水面・トーンマップ・テクスチャキャッシュ・スプライト・デバッグ描画を
1つのクラスが抱えており、これをパスごとに分割する予定です。
`Renderer` を直接触っているコードは追従が必要になります。

## ライセンス

本プロジェクトは[MIT License](LICENSE)のもとで公開されています。

同梱している第三者コンポーネントとそのライセンス条文は
[THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)に掲載しています。
