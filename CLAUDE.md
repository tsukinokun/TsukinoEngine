# TsukinoEngine — AI エージェント向けガイド

自作の C++20 ゲームエンジン。ゲーム側リポジトリに submodule として取り込み、
ルートの `premake5.lua` から `include "External/TsukinoEngine"` して使う。

## ゲームを作るときは、まずレシピ

物を置く・物理・衝突・音・参照・自作シェーダー・生成と破棄といった、やりたいことごとの
実際にビルドされる短いお手本と、実装を読まないと分からない挙動（システムの順番、当たり判定の大きさ、
衝突イベントの届き方、破棄のタイミングなど）を `Tsukino.Sandbox/Recipes/README.md` にまとめてある。
**ヘッダや実装（.cpp）を読む前に、まずこれを読み、必要なレシピの .cpp だけを開く。**

- 立方体・平面・球は組み込みモデル（`EngineContext::builtinAssets->models`）がある。モデルのファイルを作らない。
- 自作シェーダーは `CustomShaderComponent` を ModelComponent に付けるだけで描ける。描画コマンドを自分で積まない。

## 最重要ルール

### 探索してはいけない場所

`External/` 配下（Effekseer / JoltPhysics / entt / cereal / hlslpp / SDL / bgfx / bx / bimg / miniaudio など）は
**読まない・grep しない**。約4,600ファイル・180MBあり、コンテキストを食い潰す。
`.ignore` で機械的にも塞いであるが、意図として覚えておくこと。

`Tools/HeaderCheck/generated/` も読まない。premake が公開ヘッダ1本につき1つ
生成する、include 1行だけの検証用ファイル（C-14）で、読む価値のある中身は無い。
`.ignore` で機械的にも塞いである。

`bin/` `bin-int/` `.build/`（ビルド生成物）と `html/` `xml/`（Doxygen生成物。正は
`Docs/` 配下）も同様に `.ignore` で塞いである。

エンジンの API を知りたいときは、ヘッダを片端から読むのではなく次を見る:

| ファイル | 内容 |
|---|---|
| `Tsukino.Sandbox/Recipes/README.md` | やりたいことごとのお手本と挙動の説明。**まずここ** |
| `Docs/api-index.md` | 全公開型 → モジュール → ヘッダパスの1行索引（レシピに無い型を探すとき） |
| `Docs/api/<Module>.md` | モジュール単位の公開API。必要な1本だけ読む |
| `Docs/components.md` | 全コンポーネントと Prefab JSON のフィールド一覧 |
| `Docs/agent-manifest.json` | 上記の機械可読版（外部ツール用の契約ファイル。**エージェントは読まない**） |

`Docs/` は自動生成物。**手で編集しない。** エンジンにコンポーネントや公開 API を
追加・変更したら `generate-docs.bat` を実行して更新する
（doxygen → `vendor\premake5.exe gen-manifest` → `gen-api-digest` を順に走らせる）。

`components.md` に載るのは**エンジン組み込みのコンポーネントだけ**で、
ゲーム側で定義したコンポーネントは含まれない。

## コーディング規約

**`CODING_GUIDELINES.md` が唯一の正。** 命名・include 順・クラス設計・エラーハンドリング・
アセットとパス・Git 運用はすべてそちらを読む（ここには写さない）。

コメントの書式は `tsukino-doc-comment` スキル（`.claude/skills/`）に切り出してある。
ヘッダ / ソースにコメントを書くときはそれに従う。

補足として、ここでしか書いていない注意点:

- 応答は日本語
- シェーダーは bgfx の `.sc`。名前は `<名前>.vs.sc`（頂点）/ `<名前>.ps.sc`（ピクセル）、
  共通部分は `.sh`、入出力は同じフォルダの `varying.def.sc`。変換は `tsukino_shaders()` がビルド時に行う

## モジュール構成

依存は上から下へ一方向。

| モジュール | 役割 |
|---|---|
| `Tsukino.Core` | ECS基盤（Registry / Entity / EntityRef / EventBus / ISystem）、Window、InputSystem、Log、Path、数学 |
| `Tsukino.GraphicsCommon` | API 非依存の描画データ（MeshData / ModelData / MaterialData） |
| `Tsukino.Engine` | アセット基盤（AssetManager / Handle / Ref / 各種 Loader）、Scene、SystemManager、PrefabFactory |
| `Tsukino.Renderer` | 描画（bgfx）。ゲームからは主に `Renderer.hpp` のみ |
| `Tsukino.Physics` | 物理。Jolt Physics のラッパ（PhysicsWorld / BodyHandle / IPhysicsDebugDraw）と SpringBone。Jolt の型はこのモジュールの中に閉じている |
| `Tsukino.BuiltIn` | 組み込みコンポーネント（データのみ）、BuiltInAssets、TransformUtility |
| `Tsukino.EngineIntegration` | アプリ向け層。EngineIntegration / EngineContext / EngineAPI / GameSceneBase と組み込みシステム群 |
| `Tsukino.Audio` | AudioManager |
| `Tsukino.Sandbox` | エンジン単体ビルド時のみのサンプル。ゲーム側から include されたときは自動的にスキップされる |
| `Tsukino.HeaderCheck` | 公開ヘッダを1本ずつ PCH 無しでコンパイルする検証専用。単体ビルド時のみ。中身は自動生成 |

注意: 組み込み**システム**は `Tsukino.EngineIntegration/.../ECS/System/` に置かれているが、
名前空間は `Tsukino::BuiltIn::ECS`（パスと名前空間が一致していない）。

## ビルド

```
build.bat            Debug をビルド
build.bat Release
```

全 OS 共通のビルドは CMake（ルートの `CMakeLists.txt`。手順は `Platforms/README.md`）。
Windows では premake の `build.bat` も使える（ゲームの ChronoSort は今 premake でビルドしている）。

**MSBuild を直接叩かない。** 静音フラグを忘れると数千行出る。`build.bat` は成功時 0 行、
失敗時はエラー行だけを返し、NuGet の復元も必要なときだけ自動で走る。

ソースファイルを新規追加したときだけ premake の再生成が要る（`open.bat`）。
ドキュメント / マニフェスト生成は `generate-docs.bat`。

描画は bgfx（`Tsukino.Renderer/src/Bgfx`）。シェーダーは bgfx の `.sc` で書き、
`tsukino_shaders()`（`Tools/premake/tsukino.lua`）がビルド時に変換する。Renderer の中だけで使うヘッダ（描画 API の型を含む）は
`Tsukino.Renderer/internal/` に置き、公開ヘッダ（`include/`）からは include しない。

## ログ

`Tsukino::Core::Log::{Info, Warn, Error}` は既定でデバッガの出力にのみ出す
（Windows は `OutputDebugStringA`、それ以外は `SDL_Log`。＝デバッガの外からは何も見えない）。

`Log::SetLogFile("Logs/Tsukino.log")` を呼ぶとファイルにも追記される。
実行して結果を確認したいときは必ずこれを有効にすること。
「Prefab file not found」「Unknown component type written in Prefab」といった
致命的な警告は、これが無いと完全に不可視になる。

一時的な調査ログも `Logs/` 配下に出すこと。デバッグ用の `ofstream` をカレント
ディレクトリ直下に開くと、リポジトリを汚し `git diff` を膨らませる。
