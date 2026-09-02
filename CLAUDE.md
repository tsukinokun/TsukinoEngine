# TsukinoEngine — AI エージェント向けガイド

自作の C++20 ゲームエンジン。ゲーム側リポジトリに submodule として取り込み、
ルートの `premake5.lua` から `include "External/TsukinoEngine"` して使う。

## 最重要ルール

### 探索してはいけない場所

`External/` 配下（Effekseer / JoltPhysics / entt / cereal / hlslpp / DirectXTex）は
**読まない・grep しない**。数万行あり、コンテキストを食い潰す。

エンジンの API を知りたいときは、ヘッダを片端から読むのではなく次を見る:

| ファイル | 内容 |
|---|---|
| `Docs/components.md` | 全コンポーネントと Prefab JSON のフィールド一覧 |
| `Docs/api-digest.md` | ゲームから触る公開 API の索引 |
| `Docs/agent-manifest.json` | 上記の機械可読版（外部ツール用の契約ファイル） |

`Docs/` の3つは自動生成物。**手で編集しない。** エンジンにコンポーネントや公開 API を
追加・変更したら `generate-docs.bat` を実行して更新する
（doxygen → `vendor\premake5.exe gen-manifest` → `gen-api-digest` を順に走らせる）。

`components.md` に載るのは**エンジン組み込みのコンポーネントだけ**で、
ゲーム側で定義したコンポーネントは含まれない。

### 文字コードとフォーマット

- UTF-8 / CRLF
- インデントは**4スペース**（`.clang-format` の `UseTab: Never` / `IndentWidth: 4`、`BasedOnStyle: Google`）
- 整形は `.clang-format` が唯一の正。手動整形しない
    - `CODING_GUIDELINES.md` には「インデントはタブを使用する」とあるが、`.clang-format` も実コードもスペース。`.clang-format` を優先する
- コメントは日本語。応答も日本語

## モジュール構成

依存は上から下へ一方向。

| モジュール | 役割 |
|---|---|
| `Tsukino.Core` | ECS基盤（Registry / Entity / EntityRef / EventBus / ISystem）、Window、InputSystem、Log、Path、数学 |
| `Tsukino.GraphicsCommon` | API 非依存の描画データ（MeshData / ModelData / MaterialData） |
| `Tsukino.Engine` | アセット基盤（AssetManager / Handle / Ref / 各種 Loader）、Scene、SystemManager、PrefabFactory |
| `Tsukino.Renderer` | DX11 バックエンド。ゲームからは主に `Renderer.hpp` のみ |
| `Tsukino.BuiltIn` | 組み込みコンポーネント（データのみ）、BuiltInAssets、TransformUtility |
| `Tsukino.EngineIntegration` | アプリ向け層。EngineIntegration / EngineContext / EngineAPI / GameSceneBase と組み込みシステム群 |
| `Tsukino.Audio` | AudioManager |
| `Tsukino.Physics` | 実質空。物理は EngineIntegration の PhysicsSystem + Jolt |
| `Tsukino.Sandbox` | エンジン単体ビルド時のみのサンプル。ゲーム側から include されたときは自動的にスキップされる |

注意: 組み込み**システム**は `Tsukino.EngineIntegration/.../ECS/System/` に置かれているが、
名前空間は `Tsukino::BuiltIn::ECS`（パスと名前空間が一致していない）。

## コーディング規約

### 命名

- クラス / 構造体 / 関数 / メソッド / 定数 / enum 値 / 名前空間: **PascalCase**
    - STL 互換関数や軽量アクセサのみ camelCase 可（`extension()` など）
    - グローバル定数は `k` 接頭辞可（`kMaxLightCount`）
- メンバ変数: `m_` + camelCase（`m_path`）。構造体の公開メンバは `m_` を付けない
- 静的メンバ変数: `s_` + PascalCase（`s_Assets`）
- ローカル変数: camelCase
- ファイル名: PascalCase。クラスを定義するヘッダはクラス名と一致させる。`pch.h` など慣習的なものは小文字可

### 禁止・必須

- `using namespace std;` は書かない。`std::` を毎回書く
- 既定値初期化は `= {};` ではなく `{};`
- 変数宣言は1行に1つ。カンマで複数宣言しない
- 省略形を使わない。ただし10行以内のスコープのローカル変数とループカウンタ（i, j, k）は可。範囲 for 文の変数は省略しない
- インクルードガードは `#pragma once`

### include 順

1. 対応するヘッダ（`Foo.cpp` なら `Foo.hpp`）
2. 同一モジュール内のヘッダ
3. 他モジュールのヘッダ
4. 標準ライブラリ / 外部ライブラリ

ヘッダは `pch.h` に依存せず自己完結させる（`pch.h` は `.cpp` に強制インクルードされる）。
可能な限り前方宣言を使い、ヘッダの依存を減らす。

### クラス設計

- 単一責務。依存は一方向
- コンストラクタで重い処理（ファイル IO、GPU リソース確保）をしない。必要なら `initialize()` を用意
- RAII を徹底する
- **メンバ宣言順は破棄順序の設計である。** メンバは宣言の逆順に破棄されるので、上に書いたものほど長生きする

### シェーダー

- ファイル名は小文字 snake_case、種類別接頭辞（`vs_` `ps_` `gs_` `cs_` `ms_` `as_`）、拡張子 `.fx`、エントリは `main`
- 新規シェーダーはプロジェクトに追加しない（追加するならビルドから除外する設定にする）

### コメント

Doxygen 記法（`//!` / `//!<`）で書く。書式規約は分量が多いため
**`tsukino-doc-comment` スキル**に切り出してある。
エンジンのヘッダ / ソースにコメントを書くときは必ずそのスキルを読むこと。

## ビルド

`msbuild` は PATH に無い。premake は `vs2022` を生成するので **VS2022 の MSBuild をフルパスで指定**する
（`vswhere -latest` はこの環境では VS18 を返すため使わない）。

```
"C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" .build\<Solution>.sln /p:Configuration=Debug /p:Platform=x64 /m /nologo /v:q /clp:ErrorsOnly;NoSummary
```

- `/v:q /clp:ErrorsOnly;NoSummary` を**必ず付ける**。付けないと数千行が出力される。
  付けた場合、ビルド成功時の出力は 0 行、失敗時はエラー行だけになる
- **ソースファイルを新規追加したときだけ** premake の再生成が必要（`open.bat`）
- ドキュメント / マニフェスト生成: `generate-docs.bat`

### NuGet の復元

`.build/` を作り直した直後は復元が必要。premake が吐くのは**旧形式の `packages.config`** なので、
`-t:restore` だけでは復元されず `-p:RestorePackagesConfig=true` が要る
（この環境に `nuget.exe` は無い。Visual Studio で .sln を開けば IDE が自動で復元する）。

```
"C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" .build\<Solution>.sln -t:restore -p:RestorePackagesConfig=true -p:Configuration=Debug -p:Platform=x64 -nologo -v:m
```

これを忘れると `This project references NuGet package(s) that are missing on this computer.` で止まる。

## ログ

`Tsukino::Core::Log::{Info, Warn, Error}` は既定で `OutputDebugStringA` にのみ出力する
（＝デバッガの外からは何も見えない）。

`Log::SetLogFile("Logs/Tsukino.log")` を呼ぶとファイルにも追記される。
実行して結果を確認したいときは必ずこれを有効にすること。
「Prefab file not found」「Unknown component type written in Prefab」といった
致命的な警告は、これが無いと完全に不可視になる。
