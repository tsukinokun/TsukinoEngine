# TsukinoEngine 品質レポート

**初版:** 2026-08-17
**最終更新:** 2026-09-05
**対象:** TsukinoEngine 全 8 モジュール（約 25,000 行）

エンジン全体のコードレビューで見つかった問題と、その後の対応状況の記録です。

項目番号（A-1 / B-1 / C-1 …）は初版から変えていません。
過去のコミットメッセージや議論から参照されるためです。

---

## 0. 現在の状況

### 解消済み

| ID | 項目 |
|---|---|
| A-1 〜 A-6 | 致命的な欠陥（use-after-free / リーク / イテレータ破壊）6 件 |
| B-1 〜 B-13 | 重大バグ 13 件 |
| C-1 | Renderer のレイヤ違反（`IPostWorldPass` で解消） |
| C-2 | AssetManager の重複ロードと乱数ハンドル |
| C-3 | TransformSystem の O(N²) |
| C-9 | `Tsukino.Physics` が空プロジェクト |
| C-12 | DrawCommand の生ポインタが依存していた暗黙のフレーム契約 |

### 部分的に解消

| ID | 項目 | 残り |
|---|---|---|
| C-6 | 毎フレームのヒープ確保 | 実質解消。残りは対応しない方針（後述） |
| C-7 | EventBus | 正しさの欠陥は解消。性能は未着手 |
| C-8 | ビルド品質ゲート | 警告 89 → 59 件。テストと CI は未着手 |
| C-10 | Log | ファイル出力は解消。レベルフィルタは未実装 |
| C-11 | Sandbox のシーン | 5 シーン中 3 つが PrefabFactory へ移行済み |

### 未着手

| ID | 項目 | 規模 |
|---|---|---|
| C-4 | Renderer の神クラス化（1,999 行 / 104KB / 47 メソッド） | 大 |
| C-5 | マテリアルソートとフラスタムカリングの不在 | 大 |
| C-13 | モーションブラーが素朴な gather 実装 | 中 |

---

## 1. 初版で修正した欠陥

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
Scene:              EventBus -> Registry -> SystemManager
                    （破棄は逆順なので System が最初に消える）

EngineIntegration:  Window -> Renderer -> AssetManager -> BuiltInAssets
                    -> InputSystem -> AudioManager -> PrefabFactory
                    -> GameSceneManager
                    （GameSceneManager が最初に消え、シーンの後始末が
                      Renderer や AudioManager より先に走る）
```

**ECS 外のリソースはイベントバスではなく EnTT の破棄シグナルで回収する**

`Registry` に `OnDestroy<T>()` / `OnConstruct<T>()` を追加しました。
イベントバスは「`Scene::DestroyEntity()` を通ったときだけ」発火するのに対し、
EnTT の破棄シグナルは**どの破棄経路でも必ず発火する**ため、
所有権が ECS の外にあるリソースはこちらで回収します。

- `PhysicsSystem` -> `OnDestroy<CollisionComponent>` で Jolt Body を `RemoveBody` + `DestroyBody`
- `PhysicsSystem` -> `OnDestroy<CharacterControllerComponent>` で `CharacterVirtual` を破棄
- `EffectSystem` -> `OnDestroy<EffectComponent>` で Effekseer の再生ハンドルを停止

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
| B-5 | `Window::SetUpdateMode` の条件式が常に真だった | `UpdateMode` の意図どおりに評価するよう修正 |
| B-7 | `RenderPass::Transparent` が一度も描画されていなかった | `Renderer::Render()` に World と Water の間で Transparent パスを追加 |
| B-8 | 深度テクスチャ / DSV の `HRESULT` が未チェックだった | 全て検査してログ付きで失敗を返すようにした。生成手順も `CreateSizeDependentResources()` に集約 |
| B-9 | `InputSystem` の判定関数が配列の境界チェックをしていなかった | 3 関数にガードを追加（書き込み側の `SetKeyState` だけ検査していた） |
| B-10 | `#pragma once` 欠落 5 ファイル | 3 ファイルに追加。中身が BOM だけの `RootMotionSystem.hpp` / `.cpp` は削除 |
| B-11 | `CoInitializeEx` に対応する `CoUninitialize` が無かった | `EngineIntegration` にデストラクタを定義して呼ぶようにした |
| B-12 | `RigidBodyComponent.hpp` という実在しない大文字小文字での include | 実ファイル名 `RigidbodyComponent.hpp` に合わせた |
| B-13 | `AssetManager` の非 static メンバに `s_` プレフィックス | `m_assets` / `m_loaders` / `m_importers` にリネーム（規約 4-2） |

---

## 2. 解消した設計負債

### C-1. レイヤ違反 / 循環依存 — 解消済み

**当時の状況**
`premake5.lua` の `Tsukino.Renderer` の `includedirs` に `Tsukino.EngineIntegration/include` と
`Tsukino.BuiltIn/include` が入っており、`Renderer.cpp` が `EffectSystem.hpp` を include していた。
`Renderer::Render(EffectSystem*)` という上位層の型への直接依存もあった。
`CODING_GUIDELINES.md` の「依存関係は一方向」に反しており、
Renderer が EngineIntegration をビルドしないと成立しない状態だった。

**採った対応**
`Tsukino.Physics` の `IPhysicsDebugDraw` と同じく、下層にインターフェースを置いて上層が実装する形にした。

1. `Tsukino/Renderer/IPostWorldPass.hpp` を新設。World パスの直後に差し込む描画を
   `RenderPostWorld(dc, view, projection)` の 1 メソッドで受け取る
2. `EffectSystem` が `IPostWorldPass` を実装する（`RenderEffects()` を `RenderPostWorld()` へリネーム）
3. `Renderer::Render()` の引数を `IPostWorldPass*` に変更
4. `premake5.lua` から Renderer の `EngineIntegration` / `BuiltIn` の includedirs を外した

実際の依存は 1 メソッドだけで、呼び出し側も `EngineAPI.cpp` の 1 箇所しか無かったため、
差し替えの影響は小さかった。

**結果**
`Tsukino.Renderer` から上位層への include は 0 件になり、includedirs からも外れている。

### C-2. AssetManager の重複ロードと乱数ハンドル — 解消済み

**当時の状況**
`AssetManager::Load()` にパスからハンドルへのキャッシュが無く、同じパスを Load するたびに
デコードから GPU リソース確保までやり直していた。
さらに `AssetHandleGenerator::Generate()` が `std::mt19937_64` で乱数のハンドルを払い出していた。

**採った対応**
重複ロードとシリアライズ不能は、別々の手段で解いた。

1. **重複ロード** — `AssetManager` に `m_pathToHandle`（パス -> ハンドル）を持たせ、
   `Load()` の冒頭でヒットしたら既存ハンドルを返す
2. **ハンドルの決定化** — `Generate()`（乱数）を廃し、`GenerateFromKey()` が
   識別キーの FNV-1a 64bit ハッシュからハンドルを作るようにした。
   キーの正規化（区切り文字を `/` へ、大文字小文字を小文字へ）は
   `AssetHandleGenerator::NormalizeKey()` に集約し、**`m_pathToHandle` のキーも同じ関数を通す**。
   両者が食い違うと同じアセットに別ハンドルが出るため、正規化の実装は 1 箇所に限ること
3. **プレハブへの搭載** — ハンドルの値そのものを JSON へ書くのではなく、
   パス文字列を保持する `AssetRef` を経由する。
   `PrefabFactory` が `AssetRefResolverArchive` でパスをハンドルへ解決する

`AssetHandleSerialization.hpp` は**実装せず削除した**。
ハンドルの u64 をそのまま書き出すだけの実装が残っており、プロセス固有の乱数が
プレハブ JSON へ書き込まれて次回起動時に無効なハンドルとして黙って読まれる状態だった
（エラーもログも出ない）。唯一の利用箇所だった `EffectComponent` を `AssetRef` へ移行し、
このヘッダを削除している。

> `EffectComponent` は `ModelComponent` / `SpriteComponent` が `AssetRef` へ移行した際の
> 取りこぼしで、「プレハブ機能が完成していない」と言われていた実体はこれだった。

**マテリアルのハンドル**
`ModelLoader` が作る `MaterialAsset` は単体のファイルを持たないため、
`モデルのパス + "|material|" + 番号` を識別キーにしている。

### C-3. TransformSystem が O(N²) — 解消済み

**当時の状況**
`TransformSystem::UpdateWorldMatrixRecursive()` が、子を探すために
エンティティ 1 つにつき Transform view を全走査していた。
親子関係にループがあると無限再帰でスタックオーバーフローする状態でもあった。

**採った対応**
1 パス目で親から子への隣接リスト（`m_childrenByParent`）を構築し、2 パス目でルートから走査する。
親子ループは `kMaxHierarchyDepth` による深さ上限で打ち切る。

### C-9. `Tsukino.Physics` が空プロジェクト — 解消済み

**当時の状況**
`Tsukino.Physics/` には 0 バイトの `pch.cpp` しか無く、premake の定義も
`Tsukino.Core` にしかリンクしていなかった（Jolt の includedirs も links も無し）。
参照側は全てコメントアウトされており、誰もリンクしないシンボル 0 個の `.lib` を毎回ビルドしていた。
物理は全て `Tsukino.EngineIntegration/src/ECS/System/PhysicsSystem.cpp`（53KB / 1120行）に入っていた。

**採った方針: Jolt のラッパをこちらへ移す**

`PhysicsSystem::Impl` をそのまま移すことはできない。`Impl` は EngineIntegration で
宣言された `PhysicsSystem` の入れ子型であり、下層へ置くと
**Physics -> EngineIntegration という逆向きの依存**が生まれる。
そのため「移動」ではなく境界を設計し直した。

- `Tsukino.Physics` は `Tsukino.Core` と `JoltPhysics` にのみ依存する。
  `entt` / `Renderer` / `BuiltIn` / `EngineIntegration` は一切参照しない
- 公開ヘッダ（`PhysicsWorld.hpp` / `PhysicsTypes.hpp` / `BodyHandle.hpp` /
  `IPhysicsDebugDraw.hpp` / `PhysicsMath.hpp`）に Jolt の型は現れない。
  座標と回転は hlslpp、エンティティの同一性は `uint64_t` のユーザーデータで受け渡す
- `JPH::BodyID` は `Tsukino::Physics::BodyHandle` に置き換えた。これにより
  データのみのはずだった `Tsukino.BuiltIn` から Jolt の include が消えている
- デバッグ描画は下層に `IPhysicsDebugDraw` を置き、`RendererPhysicsDebugDraw`
  （EngineIntegration）が実装して `Renderer` へ流す。
  このパターンは後に C-1 の `IPostWorldPass` でも踏襲した

**結果**

| | 移動前 | 移動後 |
|---|---|---|
| `PhysicsSystem.cpp` | 1120行（Jolt ヘッダ19本） | 約600行（Jolt を include しない） |
| `Tsukino.Physics` | `pch.cpp` 0バイトのみ | `PhysicsWorld` ほか約900行 |
| Jolt を include するモジュール | BuiltIn / EngineIntegration | Physics のみ |

`PhysicsSystem` は pimpl が不要になったため、`struct Impl; Impl* m_impl;` を廃して
`std::unique_ptr<PhysicsWorld>` と素直なメンバに置き換えた。

> **注意**: `cMaxBodies` / `cMaxBodyPairs` / `cMaxContactConstraints` は 1024 のまま
> `PhysicsWorld` のコンストラクタへ移してある。値は変えていない。

**SpringBone も同じモジュールへ寄せた**

`Tsukino::Physics` 名前空間は SpringBone が既に使っていた。実体は
`Tsukino.Engine/{include,src}/.../Physics/SpringBone/` に置かれており、
名前の衝突こそ無いものの、同じ名前空間の型が 2 モジュールに分かれる状態になっていた。

調べたところ SpringBone は `Tsukino.Engine` の中身に一切依存していなかったため、
名前空間をリネームするのではなく **4 ファイルを `Tsukino.Physics` へ移した**。

- `Tsukino/Engine/Physics/SpringBone/*` -> `Tsukino/Physics/SpringBone/*`
- 名前空間 `Tsukino::Physics` / `Tsukino::Physics::SpringBonePhysics` は変更なし。
  これでパスと名前空間が一致する
- `Tsukino.Engine` から物理コードが無くなり、アセット基盤モジュールとして純化した

### C-12. DrawCommand の生ポインタが依存していた暗黙のフレーム契約 — 解消済み

**当時の状況**
`DrawCommand::material` / `materialData` は、System が所有する `std::deque` の要素を指す生ポインタだった。

- `ModelSystem` と `SpriteRendererSystem` が **Update の冒頭で自前の arena を `clear()`**
- コマンド側の破棄は `Renderer::Render()` の中の `m_drawQueue.Clear()` だけ

つまり「毎フレーム必ず Update -> Render の順で回り、Render がコマンドを消費しきる」
という契約に依存していたが、**その契約はコード上のどこにも表現されていなかった**。
Render を 1 フレーム飛ばすと、次の Update 冒頭の `clear()` で即ダングリングになる状態だった。

**採った対応: 寿命の管理点を1つに集約する**

arena を `DrawCommandQueue` へ移した。

```cpp
std::deque<Material>        m_materialArena;
std::deque<CBufferMaterial> m_materialDataArena;

Material&        AllocMaterial()     { return m_materialArena.emplace_back(); }
CBufferMaterial& AllocMaterialData() { return m_materialDataArena.emplace_back(); }

void Clear() {   // 3つを必ず同時に捨てる
    m_commands.clear();
    m_materialArena.clear();
    m_materialDataArena.clear();
}
```

`Renderer::AllocMaterial()` / `AllocMaterialData()` が転送し、System 側は
`ctx->renderer->AllocMaterial()` で確保する。System からは arena が消えた。

コマンドとそれが指す実体を同じオブジェクトが所有するため、
`Clear()` が呼ばれない限りポインタは有効で、Render を飛ばしても壊れない。

`deque` を選ぶ理由（追加が続いても既存要素への参照が無効化されない）は
キュー側のコメントへ移して残してある。

**`DrawCommand` は生ポインタのままにした。** 値で持たせると構造体が肥大化して
キューのコピーコストが上がるため、ここでは所有権の一元化だけを目的にしている。

**影響範囲**: `DrawCommandQueue.hpp`、`Renderer.hpp/cpp`、
`ModelSystem.hpp/cpp`、`SpriteRendererSystem.hpp/cpp`

---

## 3. 残っている設計負債

### C-4. Renderer の神クラス化 — 未着手（v1.x へ送る）

**現状**
`Renderer.cpp` は 1,999 行 / 104KB / 47 メソッド。`Renderer.hpp` の宣言は 73 個。
Device・SwapChain・シャドウ・スカイ・水面・トーンマップ・テクスチャキャッシュ・
スプライト・デバッグ描画・フォント生成を 1 クラスが所有している。

**移行手順**
影響が小さい順に抽出していく。

1. `TextureCache`（`m_textureCache` + `GetTextureSRV`）— 依存が閉じているので最初に切り出せる
2. `ShadowPass`（`m_shadowMap*` + `CreateShadowMap` + `ExecuteShadowCommand`）
3. `SkyPass` / `TonemapPass` / `WaterPass`

**影響範囲**: 抽出のたびに `Renderer.hpp` の公開 API が変わるため、呼び出し側の System も追従が必要。
このため v1.0.0 では手を付けず、README に「1.x の間も Renderer の公開 API は変わりうる」と
明記する方針にしている。

### C-5. マテリアルソートとフラスタムカリングの不在 — 未着手

**解消済みの部分**
初版で挙げていた「Transparent パスに奥から手前へのソートが無く、半透明同士の前後関係が
正しくならない」は、ソートではなく **`TransparentDepth` の深度事前パス**で対処済み。
色を書かずに深度だけ埋めてから、Transparent 側を EqualReadOnly で描いている。

`DrawCommand::sortOrder` のコメントにある通り、3D パスの前後は深度バッファが決めるため、
順序キーを持ち込むと「深度とキーのどちらが正か」が二重定義になる。
ソートキーを見て並べ替えるのは、深度を使わない Overlay パスだけでよい。

**残っている問題**
- マテリアル単位のソートが無いためステート変更が最大化される
- フラスタムカリングが無く、画面外のオブジェクトも全て描画される

**移行手順**
`DrawCommandQueue` を pass ごとのバケットに分け、投入時に振り分ける。
マテリアルをキーにしたソートは Render の直前に一度だけ行う。
カリングはカメラの視錐台とメッシュのバウンディングボリュームで判定する
（現状 `MeshBuffer` がバウンディングボリュームを持っていないため、そこから必要）。

### C-6. 毎フレームのヒープ確保 — 実質解消（残りは対応しない）

**解消済みの部分**
- `m_drawEntries` は `clear()` で容量を維持するため、ウォームアップ後は追加確保が起きない
- `DrawCommand::customDraw` のラムダは `[this, states, runBegin, runEnd]` しか捉えないため
  `std::function` の小オブジェクト最適化に収まり、ヒープ確保は起きない
- 収集した文字列は `std::move` で `DrawEntry` へ渡している

**残っている確保と、対応しない理由**
`FontRendererSystem::Update()` の `safeText`（フォントが持たない文字を置換した描画用文字列）は
テキストエンティティ 1 個につき毎フレーム確保される。

これを消すには `m_drawEntries` を論理サイズで管理して文字列バッファを使い回す必要があるが、
**テキストエンティティ数十個ぶんでも数マイクロ秒**であり、
コードの明快さを失う割に得るものが無い。現状のままとする。

### C-7. EventBus の Publish コスト — 正しさは解消 / 性能は未着手

**解消済み（正しさに関わる欠陥）**
再入ガードの復帰が `typeid(void)` へのリセットになっており、異なるイベント型を
ネストして Publish した後にガードが外れる問題は修正済み（前の値を復元するようにした）。

**未着手（性能）**
- 発火のたびにハンドラ配列を丸ごとコピーしている（`const auto snapshot = it->second;`）
- ハンドラごとに `std::any(event)` を構築している

いずれも `EventBus.hpp` の冒頭コメントに既知の特性として明記してある。
着手するならスナップショットを「発火中フラグ + 遅延削除」に置き換え、
`std::any` は 1 回だけ構築して使い回す。

### C-8. ビルド品質ゲートの不在 — 警告の一部を解消 / テストと CI は未着手

**解消済み（C4834 を 30 件 -> 0 件）**

`[[nodiscard]]` 由来の警告 30 件は、2 つの異なる原因が混ざっていた。

1. **`[[nodiscard]]` の付け間違い（21 件）** — `Registry::AddComponent()` に付いていた。
   「コンポーネントを付けるだけで戻り値の参照は使わない」呼び出しは正当かつ大多数なので、
   これは外した。`OnConstruct()` / `OnDestroy()` の戻り値は `connect` / `disconnect` に
   必ず使うため、あちらの `[[nodiscard]]` は残してある
2. **本物の欠陥（9 件）** — 8 つの Importer が `FileSystem::CreateDirectories()` の
   戻り値を、`AssetManager` が `IAssetImporter::Import()` の戻り値を捨てていた。
   出力先を作れないまま書き込みへ進むため、失敗が原因から遠い場所で
   「キャッシュが無い」として現れる。全て検査してログを出すようにした
   （3 章の約束ごと「失敗を握り潰さない」に該当する）

> `[[nodiscard]]` が正しく機能していた例で、外すべきものと直すべきものの
> 区別が要るという教訓になった。件数だけ見て一律に外さないこと。

**現状**（2026-09-05 実測、Debug の全ビルド）

| 警告 | 件数 | 内容 |
|---|---|---|
| C4244 | 49 | 型変換によるデータ損失の可能性 |
| C4267 | 8 | `size_t` からより小さい型への変換 |
| C4018 | 2 | signed / unsigned の比較 |

ユニークな警告は **89 件 -> 59 件**。

> 初版では「23 件、大半は C4834」と記録していたが、実測すると件数も内訳も違っていた。
> 最大の群は C4834 ではなく C4244（型変換）である。

- Tsukino 各モジュールに `warnings` の指定は無いまま。警告のエラー化もしていない
- ユニットテスト 0 件、CI 無し

**残りの移行手順**
1. C4244 / C4267 は数が多いので、明示的なキャストか型の見直しで順に潰す
2. `warnings "High"` を設定し、残った警告を潰す
3. 純ロジックのモジュール（`Path`, `Matrix`, `EventBus`, `Registry`,
   `AssetHandleGenerator`）からテストを入れる
4. GitHub Actions でビルドを回す。
   価値は「main がビルドできる」ことより「白紙の環境から clone して手順どおりに
   ビルドできる」ことの保証にある（MAX_PATH や submodule の取り漏らしもここで検出できる）

### C-10. Log が OutputDebugString のみ — ファイル出力は解消 / レベルフィルタは未実装

**解消済みの部分**
`Log::SetLogFile()` を呼ぶとファイルへも追記されるようになった。
デバッガを繋いでいない実行でもエラーを追える。

**残っている問題**
既定では有効にならない。呼び忘れると
「Prefab file not found」「Unknown component type written in Prefab」といった
致命的な警告が完全に不可視になる。

実行して結果を確認するときは必ず `Log::SetLogFile("Logs/Tsukino.log")` を呼ぶこと
（`CLAUDE.md` にも同じことが書いてある）。

レベルフィルタとタイムスタンプは未実装。

### C-11. Sandbox のシーンがハードコード — 部分的に解消

**現状**
5 シーン中 3 つ（`SceneSample1` / `JumpGameSampleScene` / `WaterGameSampleScene`）は
`PrefabFactory` を使うようになっている。
残る `BlockBreakinigSampleScene`（441 行）と `DeferredLightSampleScene`（316 行）は
エンティティ構築がハードコードのまま。

C-2 で `AssetRef` 経由のアセット参照が完成したため、残り 2 つの移行も現実的になっている。

**優先度**: 低。サンプルの可読性の問題であり、エンジンの動作には影響しない。

### C-13. モーションブラーが素朴な gather 実装 — 未着手

**現状**
`MotionBlur.ps.hlsl` は中心ピクセルの速度だけを見て近傍を平均する最小構成の実装。
演出としては機能するが、本格的な絵作りには以下が足りていない。

1. **シルエット外へ滲まない**
   動く物体の内側に背景が引き込まれる方向にしか滲まないため、
   物体が背景へはみ出して尾を引く絵にはならない
2. **深度を考慮した重み付けが無い**
   手前と奥の物体が混ざるケースで、奥のピクセルが手前に滲む
3. **フォワードパスが速度を書かない**
   `World` / `Transparent` / `Water` パスと Sky は G-Buffer を経由しないため、
   半透明・水面・デバッグ線・背景はブラー対象外（速度 0）になる

**移行手順**
1 と 2 は速度の dilation（NeighborMax タイルパス）と深度比較による重み付けを入れるのが定石。
まず画面を 16x16 程度のタイルに分割してタイル内の最大速度を求めるパスを足し、
ブラーパスはそのタイル速度を探索方向に使う。
3 はフォワード側にも速度出力を足すことになるが、半透明の速度をどう合成するかという
別の設計判断が必要になるため、1 と 2 を済ませてから検討する。

なお速度バッファそのもの（G-Buffer5・前フレームのボーン行列の退避）は
スキンメッシュまで含めて正しく動いているので、これはブラーパス単体の品質課題であり、
作り直しではなく差し替えで済む。

---

## 4. 開発時の約束ごと

破ると同じ欠陥が再発します。

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

5. **アセットをコンポーネントから参照するときは `AssetRef` を使う。**
   `AssetHandle` を直接持つとプレハブへ保存できません（C-2）。

6. **下層モジュールが上層の型を知る必要が出たら、下層にインターフェースを置く。**
   `IPhysicsDebugDraw` と `IPostWorldPass` が手本です（C-1 / C-9）。

---

## 5. このレポートの保守について

**着手する前に、必ず実コードを確認してください。**

2026-09-05 の棚卸しでは、初版で「未着手」としていた 13 項目のうち
C-3 / C-7 / C-10 は既に解消済み、C-5 / C-6 / C-11 は部分的に解消済みでした。
C-8 の警告件数（23 件）も実測値（89 件）と食い違っていました。

このレポートは自動生成物ではないため、コードの変更に自動では追従しません。
記述と実装が食い違っていたら、**実装のほうが正しい**と考えてレポートを直してください。

なお `Docs/` 配下の他のファイル（`api-index.md` / `components.md` / `api/*.md` /
`agent-manifest.json`）は `generate-docs.bat` による自動生成物です。手で編集しないでください。
このレポートだけが手書きです。
