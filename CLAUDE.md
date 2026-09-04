# TsukinoEngine — AI エージェント向けガイド

自作の C++20 ゲームエンジン。ゲーム側リポジトリに submodule として取り込み、
ルートの `premake5.lua` から `include "External/TsukinoEngine"` して使う。

## 最重要ルール

### 探索してはいけない場所

`External/` 配下（Effekseer / JoltPhysics / entt / cereal / hlslpp / DirectXTex）は
**読まない・grep しない**。19,000ファイル以上あり、コンテキストを食い潰す。
`.ignore` と `.claude/settings.json` で機械的にも塞いであるが、意図として覚えておくこと。

エンジンの API を知りたいときは、ヘッダを片端から読むのではなく次を見る:

| ファイル | 内容 |
|---|---|
| `Docs/api-index.md` | 全公開型 → モジュール → ヘッダパスの1行索引。**まずここ** |
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
- シェーダーは小文字 snake_case、種類別接頭辞（`vs_` `ps_` `gs_` `cs_` `ms_` `as_`）、
  拡張子 `.fx`、エントリは `main`。**新規シェーダーはプロジェクトに追加しない**
  （追加するならビルドから除外する設定にする）

## モジュール構成

依存は上から下へ一方向。

| モジュール | 役割 |
|---|---|
| `Tsukino.Core` | ECS基盤（Registry / Entity / EntityRef / EventBus / ISystem）、Window、InputSystem、Log、Path、数学 |
| `Tsukino.GraphicsCommon` | API 非依存の描画データ（MeshData / ModelData / MaterialData） |
| `Tsukino.Engine` | アセット基盤（AssetManager / Handle / Ref / 各種 Loader）、Scene、SystemManager、PrefabFactory |
| `Tsukino.Renderer` | DX11 バックエンド。ゲームからは主に `Renderer.hpp` のみ |
| `Tsukino.Physics` | 物理。Jolt Physics のラッパ（PhysicsWorld / BodyHandle / IPhysicsDebugDraw）と SpringBone。Jolt の型はこのモジュールの中に閉じている |
| `Tsukino.BuiltIn` | 組み込みコンポーネント（データのみ）、BuiltInAssets、TransformUtility |
| `Tsukino.EngineIntegration` | アプリ向け層。EngineIntegration / EngineContext / EngineAPI / GameSceneBase と組み込みシステム群 |
| `Tsukino.Audio` | AudioManager |
| `Tsukino.Sandbox` | エンジン単体ビルド時のみのサンプル。ゲーム側から include されたときは自動的にスキップされる |

注意: 組み込み**システム**は `Tsukino.EngineIntegration/.../ECS/System/` に置かれているが、
名前空間は `Tsukino::BuiltIn::ECS`（パスと名前空間が一致していない）。

## ビルド

```
build.bat            Debug をビルド
build.bat Release
```

**MSBuild を直接叩かない。** 静音フラグを忘れると数千行出る。`build.bat` は成功時 0 行、
失敗時はエラー行だけを返し、NuGet の復元も必要なときだけ自動で走る。

ソースファイルを新規追加したときだけ premake の再生成が要る（`open.bat`）。
ドキュメント / マニフェスト生成は `generate-docs.bat`。

## ログ

`Tsukino::Core::Log::{Info, Warn, Error}` は既定で `OutputDebugStringA` にのみ出力する
（＝デバッガの外からは何も見えない）。

`Log::SetLogFile("Logs/Tsukino.log")` を呼ぶとファイルにも追記される。
実行して結果を確認したいときは必ずこれを有効にすること。
「Prefab file not found」「Unknown component type written in Prefab」といった
致命的な警告は、これが無いと完全に不可視になる。

一時的な調査ログも `Logs/` 配下に出すこと。デバッグ用の `ofstream` をカレント
ディレクトリ直下に開くと、リポジトリを汚し `git diff` を膨らませる。
