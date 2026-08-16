# TsukinoEngine 品質レポート

**作成日:** 2026-08-17
**対象:** TsukinoEngine 全 8 モジュール（約 25,000 行）

エンジン全体のコードレビューで見つかった問題の記録です。
前半（**修正済み**）は既に対応が入っています。
後半（**設計負債**）は未着手で、今後の移行方針をここに残しています。

---

## 1. 修正済みの欠陥

### 1-1. 致命的（クラッシュ・メモリ破壊・リーク）

| ID | 問題 | 症状 |
|---|---|---|
| A-1 | `Scene` のメンバ破棄順序が依存の逆順になっていなかった | `EventBus` が System より先に破棄され、System が持つ `ScopedConnection` のデストラクタが破棄済み `EventBus` へ `Unsubscribe` を投げていた（use-after-free）。シーン切替とアプリ終了のたびに発生 |
| A-2 | `EngineContext::effectSystem` がシーン破棄後も残る | シーンが所有する `EffectSystem` の生ポインタをコンテキストへ書き込んでいたが、切替時にクリアしていなかった。次フレームの `Renderer::Render()` が解放済みメモリを触る |
| A-3 | Jolt の Body がエンティティ破棄時に解放されない | `cMaxBodies = 1024` に到達すると Body 生成に失敗する。`prevPositions` マップも無限に成長していた |
| A-4 | `EntityDestroyedEvent` が一度も発火していなかった | ゲームコードは全て `Registry::DestroyEntity()` を直接呼んでおり、イベントを発火する `Scene::DestroyEntity()` を通っていなかった。結果 Effekseer のエフェクトハンドルが漏れ続けていた |
| A-5 | View の反復中にエンティティを破棄していた | EnTT はプールの要素を入れ替えるため、反復中の破棄でイテレータが壊れる |
| A-6 | `s_modelMeshCache` がファイルスコープの static | GPU リソースが D3D11 デバイスより後に解放される静的破棄順序の問題。シーンを作り直しても解放されず増え続けていた |

#### 対応の要点

**破棄順序をコードで表明する**

`Scene` と `EngineIntegration` のメンバ宣言順を、依存の逆順（依存される側ほど上）に並べ替えました。
両方に「この順序は破棄順序の設計であり並べ替えてはならない」というコメントを入れています。

```
Scene:              EventBus → Registry → SystemManager
                    （破棄は逆順なので System が最初に消える）

EngineIntegration:  Window → Renderer → AssetManager → BuiltInAssets
                    → InputSystem → AudioManager → PrefabFactory
                    → GameSceneManager
                    （GameSceneManager が最初に消え、シーンの後始末が
                      Renderer や AudioManager より先に走る）
```

**ECS 外のリソースはイベントバスではなく EnTT の破棄シグナルで回収する**

`Registry` に `OnDestroy<T>()` / `OnConstruct<T>()` を追加しました。
イベントバスは「`Scene::DestroyEntity()` を通ったときだけ」発火するのに対し、
EnTT の破棄シグナルは**どの破棄経路でも必ず発火する**ため、
所有権が ECS の外にあるリソースはこちらで回収します。

- `PhysicsSystem` → `OnDestroy<CollisionComponent>` で Jolt Body を `RemoveBody` + `DestroyBody`
- `PhysicsSystem` → `OnDestroy<CharacterControllerComponent>` で `CharacterVirtual` を破棄
- `EffectSystem` → `OnDestroy<EffectComponent>` で Effekseer の再生ハンドルを停止

> **注意**: System は Registry より先に破棄されるため、**購読解除を必ずデストラクタ（または `Finalize()`）で行うこと**。
> 解除を忘れると、今度はレジストリ側の後始末で破棄済みの `this` が呼ばれます。

**破棄は予約制にする**

`Registry` に `QueueDestroy()` / `FlushDestroyQueue()` を追加しました。
`Scene::Update()` が全 System の更新後に `FlushDestroyQueue()` を呼びます。

System の中からエンティティを破棄する場合は**必ず `QueueDestroy()` を使ってください**。
`DestroyEntity()` は即時破棄なので、View の反復中に呼ぶとイテレータが壊れます。

### 1-2. 重大バグ

| ID | 問題 | 対応 |
|---|---|---|
| B-1 | ウィンドウリサイズ未対応（`WM_SIZE` も `ResizeBuffers` も無かった）| `GraphicsContext::Resize()` / `Renderer::Resize()` を追加。`Window::SetResizeCallback()` 経由で接続 |
| B-2 | Alt+Tab で入力が固着（`WM_KEYUP` が移動先ウィンドウへ行くため）| `InputSystem::ClearAllKeys()` を追加。`WM_KILLFOCUS` / `WM_ACTIVATEAPP(FALSE)` で呼ぶ |
| B-3 | `Scene::Initialize()` がどこからも呼ばれていなかった | `GameSceneBase::Initialize()` から `OnInitialize()` の前に呼ぶようにした |
| B-4 | `WinMain` が初期化失敗時に `return false`（= 0 = 成功）を返していた | `EXIT_FAILURE` に修正 |
| B-5 | `Window::SetUpdateMode` の条件式が `... \|\| true` で常に真だった | `UpdateMode` の意図どおりに評価するよう修正 |
| B-7 | `RenderPass::Transparent` が一度も描画されていなかった | `Renderer::Render()` に World と Water の間で Transparent パスを追加 |
| B-8 | 深度テクスチャ / DSV の `HRESULT` が未チェックだった | 全て検査してログ付きで失敗を返すようにした。生成手順も `CreateSizeDependentResources()` に集約 |
| B-9 | `InputSystem` の判定関数が配列の境界チェックをしていなかった | 3 関数にガードを追加（書き込み側の `SetKeyState` だけ検査していた） |
| B-10 | `#pragma once` 欠落 5 ファイル | 3 ファイルに追加。中身が BOM だけの `RootMotionSystem.hpp` / `.cpp` は削除 |
| B-11 | `CoInitializeEx` に対応する `CoUninitialize` が無かった | `EngineIntegration` にデストラクタを定義して呼ぶようにした |
| B-12 | `RigidBodyComponent.hpp` という実在しない大文字小文字での include | 実ファイル名 `RigidbodyComponent.hpp` に合わせた |
| B-13 | `AssetManager` の非 static メンバに `s_` プレフィックス | `m_assets` / `m_loaders` / `m_importers` にリネーム（規約 §4-2） |

---

## 2. 設計負債（未着手）

以下は今回は手を入れていません。着手する際の指針を残します。

### C-1. レイヤ違反 / 循環依存

**現状**
`premake5.lua` の `Tsukino.Renderer` の `includedirs` に `Tsukino.EngineIntegration/include` と
`Tsukino.BuiltIn/include` が入っており、`Renderer.cpp` が `EffectSystem.hpp` を include している。
`Renderer::Render(EffectSystem*)` という上位層の型への直接依存もある。

**なぜ問題か**
`CODING_GUIDELINES.md` §5 の「依存関係は一方向」に反する。
Renderer が EngineIntegration をビルドしないと成立しない状態で、Renderer 単体でのテストや差し替えができない。

**移行手順**
1. Renderer 側に `IPostWorldPass`（仮）のような純粋仮想インターフェースを定義する
2. `EffectSystem` にそれを実装させる
3. `Renderer::Render()` の引数を `IPostWorldPass*` に変える
4. `premake5.lua` から Renderer の `EngineIntegration` / `BuiltIn` の includedirs を外す

**影響範囲**: `Renderer.hpp/cpp`、`EffectSystem.hpp/cpp`、`EngineAPI.cpp`、`premake5.lua`

### C-2. AssetManager の重複ロードと乱数ハンドル

**現状**
`AssetManager::Load()` にパス→ハンドルのキャッシュが無く、同じパスを Load するたびに
デコードから GPU リソース確保までやり直す。
さらに `AssetHandleGenerator::Generate()` が `std::mt19937_64` で**乱数**のハンドルを払い出している。

**なぜ問題か**
- 同じテクスチャ・モデルが二重三重に確保される
- ハンドルがプロセスごとに変わるためシリアライズできない。
  `EngineIntegration.cpp` の `FontComponent` 登録箇所に
  「AssetHandle（プロセス内限定でシリアライズ不可）」というコメントが残っており、
  プレハブ機能が完成していない直接の原因になっている

**移行手順**
1. `AssetHandle` を「正規化した相対パス文字列の 64bit ハッシュ」から作る
2. `AssetManager` に `std::unordered_map<u64, AssetHandle>` の path→handle マップを持たせ、
   `Load()` の冒頭でヒットしたら既存ハンドルを返す
3. ハンドルが決定的になるので、`AssetHandleSerialization.hpp` を実装してプレハブに載せる

**影響範囲**: `AssetManager.hpp/cpp`、`AssetHandleGenerator.hpp`、`AssetHandleSerialization.hpp`、
`ModelSystem` のメッシュキャッシュ（キーが安定するので寿命の扱いが楽になる）

> 重複ロードとシリアライズ不能という別々に見える 2 つの問題が、この 1 つの変更で同時に解ける。

### C-3. TransformSystem が O(N²)

**現状**
`TransformSystem::UpdateWorldMatrixRecursive()` が、子を探すために
エンティティ 1 つにつき Transform view を全走査している。
`// キャッシュフレンドリーな実装: viewを1回だけ取得` というコメントが付いているが、実態は逆。

**なぜ問題か**
- Transform が N 個あると毎フレーム N × N の走査になる。
  地形やスケルトンのノードでエンティティ数が増えるほど急激に重くなる
- 親子関係にループがあると無限再帰でスタックオーバーフローする（検出も上限も無い）

**移行手順**
1. 1 パス目で `parent → children` の隣接リストを構築する（メンバに持って毎フレーム再利用）
2. 2 パス目でルートから走査する
3. 訪問済みフラグか深さ上限でループを検出し、ログを出して打ち切る

**影響範囲**: `TransformSystem.hpp/cpp` のみで閉じる

### C-4. Renderer の神クラス化

**現状**
`Renderer.cpp` は 65KB / 44 メソッド。Device・SwapChain・シャドウ・スカイ・水面・トーンマップ・
テクスチャキャッシュ・スプライト・デバッグ描画・フォント生成を 1 クラスが所有している。

**移行手順**
影響が小さい順に抽出していく。
1. `TextureCache`（`m_textureCache` + `GetTextureSRV`）— 依存が閉じているので最初に切り出せる
2. `ShadowPass`（`m_shadowMap*` + `CreateShadowMap` + `ExecuteShadowCommand`）
3. `SkyPass` / `TonemapPass` / `WaterPass`

**影響範囲**: 抽出のたびに `Renderer.hpp` の公開 API が変わるため、呼び出し側の System も追従が必要

### C-5. 描画のソート・バッチング・カリング不在

**現状**
- `Renderer::Render()` がコマンド列を pass ごとに線形走査する（現在 5 パス）
- マテリアル単位のソートが無いためステート変更が最大化される
- フラスタムカリングが無く、画面外のオブジェクトも全て描画される
- `DrawCommand` が `std::function customDraw` を値で持つため、キュー投入ごとにヒープ確保が起きる
- Transparent パスに奥→手前のソートが無く、半透明同士の前後関係が正しくならない

**移行手順**
`DrawCommandQueue` を pass ごとのバケットに分け、投入時に振り分ける。
ソートキー（pass / マテリアル / 深度）を `DrawCommand` に持たせて、Render の直前に一度だけソートする。

### C-6. 毎フレームのヒープ確保

**現状**
`FontRendererSystem::Update()` が、フォント 1 個につき毎フレーム
`std::wstring` のコピーとラムダのヒープ確保を行っている。

**移行手順**
`FontComponent` にテキストのバージョン番号を持たせ、変わったときだけ
描画用の文字列を再構築してキャッシュする。

### C-7. EventBus の Publish コスト

**現状**
- 発火のたびにハンドラ配列を丸ごとコピーしている（`const auto snapshot = it->second;`）
- ハンドラごとに `std::any(event)` を構築している
- 再入ガードの復帰が「前の値を戻す」ではなく `typeid(void)` へのリセットになっており、
  異なるイベント型をネストして Publish した後にガードが外れる

**移行手順**
スナップショットは「発火中フラグ + 遅延削除」に置き換える。
`std::any` は 1 回だけ構築して使い回す。ガードは RAII で前の値を復元する。

### C-8. ビルド品質ゲートの不在

**現状**
- Tsukino 各モジュールに `warnings` の指定が無い（既定のまま）。警告のエラー化もしていない
- 現状 23 件の警告が出ている。大半は `[[nodiscard]]` の戻り値破棄（C4834）
- ユニットテスト 0 件、CI 無し

**移行手順**
1. `Registry::AddComponent()` などの `[[nodiscard]]` が実用に合っているか見直す
   （コンポーネントを付けるだけで参照を使わない呼び出しが多く、C4834 の大半はこれ）
2. `warnings "High"` を設定し、残った警告を潰す
3. 純ロジックのモジュール（`Path`, `Matrix`, `EventBus`, `Registry`）からテストを入れる
4. GitHub Actions でビルドを回す

### C-9. `Tsukino.Physics` が空プロジェクト

`Tsukino.Physics/` には `pch.cpp` しか無い。
物理は全て `Tsukino.EngineIntegration/src/ECS/System/PhysicsSystem.cpp`（53KB）に入っている。

プロジェクトを削除するか、Jolt のラッパをこちらへ移すかを決める必要がある。
後者にすると C-1 のレイヤ違反解消とも相性が良い。

### C-10. Log が OutputDebugString のみ

レベルフィルタ・タイムスタンプ・ファイル出力が無い。
デバッガを繋いでいないリリース実行では**エラーが一切見えない**。

### C-11. Sandbox のシーンが 1 ファイル 20〜32KB

エンティティ構築が全てハードコードされている。
`PrefabFactory`（26KB のヘッダオンリー実装）があるのに使われていない。
C-2 で `AssetHandle` が決定的になれば、プレハブへの移行が現実的になる。

### C-12. DrawCommand の生ポインタが暗黙のフレーム契約に依存

`ModelSystem` / `SpriteRendererSystem` が積む `DrawCommand` の
`material` / `materialData` は、System が所有する `std::deque` の要素を指している。
`deque` なので追加しても参照は無効化されない（この選択自体は正しい）が、
「毎フレーム必ず Update → Render の順で回り、Render がコマンドを消費しきる」
ことが前提になっている。

Render を 1 フレーム飛ばすと、次の Update 冒頭の `clear()` で即ダングリングになる。
この契約がコード上で表現されていないのが問題。

**移行手順**
コマンドキュー側にフレーム単位のアリーナを持たせ、
System が「値を渡す」形にすれば契約が不要になる。

---

## 3. 開発時の約束ごと

今回の修正で導入されたルールです。破ると同じ欠陥が再発します。

1. **メンバ宣言順を並べ替えない。**
   `Scene` と `EngineIntegration` の宣言順は破棄順序の設計そのものです。
   両ファイルのコメントを読んでから触ってください。

2. **System の中からエンティティを破棄するときは `Registry::QueueDestroy()` を使う。**
   `DestroyEntity()` は即時破棄なので、View の反復中に呼ぶとイテレータが壊れます。

3. **ECS の外に実体があるリソースは `Registry::OnDestroy<T>()` で回収する。**
   イベントバス経由の回収は破棄経路によっては発火しません。
   そして**購読したら必ずデストラクタで解除する**こと。

4. **`HRESULT` は必ず検査する。**
   失敗を握り潰すと、原因から遠く離れた場所で null 参照として現れます。
