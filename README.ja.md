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

- Windows 10 または 11（x64）
- Visual Studio 2022（「C++によるデスクトップ開発」ワークロード）。
  エンジン本体は C++20、同梱の DirectXTex と Jolt は C++17 でビルドされます
- Git
- ディスク空き容量 2GB 程度。同梱サブモジュールとサンプルアセットが大きいため、
  最初のクローンには時間がかかります
- Doxygen（`generate-docs.bat` を実行する場合のみ）

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
起動するとジャンプゲームが始まります。他のサンプルシーンはコマンドライン引数で選びます。

```
Tsukino.Sandbox.exe            ジャンプゲーム（既定）
Tsukino.Sandbox.exe blocks     ブロック崩し
Tsukino.Sandbox.exe water      水面のミニゲーム
Tsukino.Sandbox.exe lights     多光源ショーケース（F1でライト数を 0/1/16/64 に切り替え）
Tsukino.Sandbox.exe api        エンジンAPIの動作確認（PrefabFactoryの検査を含む）
```

コマンドラインからビルドする場合は `build.bat`（Release は `build.bat Release`）を使います。
成功時は何も出力せず、失敗時はエラー行だけを返します。

### 先に知っておくと半日損しない2点

**最初に `Log::SetLogFile()` を呼んでください。** これを呼ぶまで
`Tsukino::Core::Log` は `OutputDebugStringA` にしか出力しません。
つまりデバッガの外で実行すると何も見えず、
`Prefab file not found` や `Unknown component type written in Prefab` といった
**実行は継続してしまう致命的な警告**が完全に不可視になります。
サンプルはいずれも次の1行から始まります。

```cpp
Tsukino::Core::Log::SetLogFile("Logs/Tsukino.log");
```

**Debug ビルドはプロジェクトを生成したマシンでしか動きません。**
Debug ではエンジンのアセットを `TSUKINO_ENGINE_ROOT`（Premake がコンパイル時に
埋め込む絶対パス）から解決し、組み込みアセットやツールのコピーを不要にしています。
Release ビルドは実行ファイルの隣に一式を配置するため単体で動きます。

### 自作ゲームからエンジンを使う

サブモジュールとして取り込み、include・link・配布物の設定は書き写さずに
エンジン側の Premake ヘルパーへ任せます。

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

    tsukino_link()             -- エンジンのinclude・lib・NuGet
    tsukino_release_payload()  -- 組み込みアセット・ツール・ライセンス条文
```

この形で取り込むと `Tsukino.Sandbox` は自動的にスキップされるため、
利用側がサンプルをビルドすることはありません。
エンジンの公開ヘッダは自己完結しているので、
エンジンと揃えたプリコンパイル済みヘッダを用意する必要もありません。

## APIの安定性

バージョン番号はセマンティックバージョニングに従いますが、1つだけ先に断っておく点があります。

**1.x の間は安定** — `Tsukino.Core`（Registry / EventBus / Window / InputSystem / Path /
数学）、`Tsukino.BuiltIn` の組み込みコンポーネント定義、Prefab JSON のスキーマ、
`EngineAPI` と `GameSceneBase`。ゲームを書くときに触るのはこの範囲で、
ここを壊すのはメジャーバージョンの仕事だと考えています。

**実質的には安定しているが、まだ約束はしない** — `Tsukino.Engine`（AssetManager /
AssetHandle / AssetRef / Scene / PrefabFactory）、`Tsukino.Physics`（PhysicsWorld /
BodyHandle / SpringBone）、`Tsukino.Audio`、`Tsukino.GraphicsCommon`、
`Tsukino.EngineIntegration` の組み込みシステム群。変更する予定はありませんが、
上の一覧ほど検討し切れていないため、1.x の間にシグネチャを直す可能性を残しています。
ここから外れたものは安定扱いになります。

**1.x の間も変わりうる** — `Tsukino.Renderer`。現在の `Renderer` はデバイス・シャドウ・
スカイ・水面・トーンマップ・テクスチャキャッシュ・スプライト・デバッグ描画を
1つのクラスが抱えており、これをパスごとに分割する予定です。
`Renderer` を直接触っているコードは追従が必要になります。

### 安定一覧に書いていないが、契約に含まれるもの

見た目には分かりにくいものの、次の3つも契約の一部です。1.x では変えません。

- **EnTT は ECS の公開 API の一部です。** `Entity` は `entt::entity` そのもので、
  `Registry::View()` は `entt::view` を返し、`Registry::OnConstruct` / `OnDestroy` は
  EnTT の sink を返します。したがってゲームはこのエンジンが取り込んでいる
  EnTT のバージョンに対してコンパイルされ、1.x では EnTT を破壊的に上げません。
- **`Window` は Win32 です。** `Tsukino/Core/Window.hpp` は
  `Tsukino/Core/WindowsLean.hpp`（`NOMINMAX` と `WIN32_LEAN_AND_MEAN` を先に定義）
  を通して `<windows.h>` を取り込み、`MessageCallback` は `UINT` / `WPARAM` / `LPARAM`
  を受け取ります。本エンジンは Windows / DirectX 11 専用で、1.x の間に
  Window の API が移植可能になることはありません。
- **失敗はログを出して既定値を返す方式です。** `FileSystem::ReadBinary` は
  ファイルが無い場合・空の場合・途中までしか読めなかった場合のいずれも空を返し、
  `AssetManager::Load` はどの失敗でも `AssetHandle::Invalid()` を返し、
  Prefab に未知のコンポーネントが書かれていれば警告を出してそれを付けずに生成します。
  起動時に `Log::SetLogFile()` を呼ばないと、これらはデバッガの外から一切見えません。
  本物のエラー通知経路へ置き換えると全体のシグネチャが変わるため、
  これは 2.0 で考えることとし、1.x では触りません。

## ライセンス

本プロジェクトは[MIT License](LICENSE)のもとで公開されています。

同梱している第三者コンポーネントとそのライセンス条文は
[THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)に掲載しています。
