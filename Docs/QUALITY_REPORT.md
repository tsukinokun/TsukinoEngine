# TsukinoEngine 品質レポート

**作成日:** 2026-08-17
**最終棚卸し:** 2026-09-05
**対象:** TsukinoEngine 全 8 モジュール（約 25,000 行）

エンジン全体のコードレビューで見つかった問題の記録です。
前半（**修正済み**）は既に対応が入っています。
後半（**設計負債**）には、その後解消したものと未着手のものが混在しています。
各項目の見出しに現在の状態を明記しています。

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

作成時点では全て未着手でしたが、その後いくつかは解消しました。
未着手のものは、着手する際の指針をそのまま残しています。

### C-1. レイヤ違反 / 循環依存 — **解消済み**

**当時の状況**
`premake5.lua` の `Tsukino.Renderer` の `includedirs` に `Tsukino.EngineIntegration/include` と
`Tsukino.BuiltIn/include` が入っており、`Renderer.cpp` が `EffectSystem.hpp` を include していた。
`Renderer::Render(EffectSystem*)` という上位層の型への直接依存もあった。

`CODING_GUIDELINES.md` §5 の「依存関係は一方向」に反しており、
Renderer が EngineIntegration をビルドしないと成立しない状態だった。

**採った対応**

`Tsukino.Physics` の `IPhysicsDebugDraw` と同じく、下層にインターフェースを置いて上層が実装する形にした。

1. `Tsukino/Renderer/IPostWorldPass.hpp` を新設。World パスの直後に差し込む描画を
   `RenderPostWorld(dc, view, projection)` の 1 メソッドで受け取る
2. `EffectSystem` が `IPostWorldPass` を実装する
   （`RenderEffects()` は `RenderPostWorld()` へリネームした）
3. `Renderer::Render()` の引数を `IPostWorldPass*` に変更
4. `premake5.lua` から Renderer の `EngineIntegration` / `BuiltIn` の includedirs を外した

実際の依存は 1 メソッドだけで、呼び出し側も `EngineAPI.cpp` の 1 箇所しか無かったため、
差し替えの影響は小さかった。

**結果**
`Tsukino.Renderer` から上位層（`Tsukino/BuiltIn/*`・`Tsukino/EngineIntegration/*`）への
include は 0 件になり、includedirs からも外れている。

**影響範囲**: `Renderer.hpp/cpp`、`IPostWorldPass.hpp`（新規）、`EffectSystem.hpp/cpp`、`premake5.lua`

### C-2. AssetManager の重複ロードと乱数ハンドル — **解消済み**

**当時の状況**
`AssetManager::Load()` にパス→ハンドルのキャッシュが無く、同じパスを Load するたびに
デコードから GPU リソース確保までやり直していた。
さらに `AssetHandleGenerator::Generate()` が `std::mt19937_64` で**乱数**のハンドルを払い出していた。

**なぜ問題か**
- 同じテクスチャ・モデルが二重三重に確保される
- ハンドルがプロセスごとに変わるためシリアライズできない。
  `EngineIntegration.cpp` の `FontComponent` 登録箇所に
  「AssetHandle（プロセス内限定でシリアライズ不可）」というコメントが残っており、
  プレハブ機能が完成していない直接の原因になっている

**採った対応**

重複ロードとシリアライズ不能は、結果的に別々の手段で解いた。

1. **重複ロード** — `AssetManager` に `m_pathToHandle`（パス→ハンドル）を持たせ、
   `Load()` の冒頭でヒットしたら既存ハンドルを返す
2. **ハンドルの決定化** — `AssetHandleGenerator::Generate()`（乱数）を廃し、
   `GenerateFromKey()` が識別キーの FNV-1a 64bit ハッシュからハンドルを作るようにした。
   キーの正規化（区切り文字を `/` へ、大文字小文字を小文字へ）は
   `AssetHandleGenerator::NormalizeKey()` に集約し、**`m_pathToHandle` のキーも同じ関数を通す**。
   両者が食い違うと同じアセットに別ハンドルが出るため、正規化の実装は 1 箇所に限ること
3. **プレハブへの搭載** — ハンドルの値そのものを JSON へ書くのではなく、
   パス文字列を保持する `AssetRef` を経由する方式を採った。
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

**影響範囲**: `AssetManager.cpp`、`AssetHandleGenerator.hpp`、`ModelLoader.cpp`、
`EffectComponent.hpp`、`EffectComponentSerialization.hpp`、`AssetHandleSerialization.hpp`（削除）

### C-3. TransformSystem が O(N²) — **解消済み**

**当時の状況**
`TransformSystem::UpdateWorldMatrixRecursive()` が、子を探すために
エンティティ 1 つにつき Transform view を全走査していた。

**採った対応**
1 パス目で `parent → children` の隣接リスト（`m_childrenByParent`）を構築し、
2 パス目でルートから走査する形に変えた。
親子関係のループは `kMaxHierarchyDepth` による深さ上限で打ち切る。

**なぜ問題か**
- Transform が N 個あると毎フレーム N × N の走査になる。
  地形やスケルトンのノードでエンティティ数が増えるほど急激に重くなる
- 親子関係にループがあると無限再帰でスタックオーバーフローする（検出も上限も無い）

**移行手順**
1. 1 パス目で `parent → children` の隣接リストを構築する（メンバに持って毎フレーム再利用）
2. 2 パス目でルートから走査する
3. 訪問済みフラグか深さ上限でループを検出し、ログを出して打ち切る

**影響範囲**: `TransformSystem.hpp/cpp` のみで閉じる

### C-4. Renderer の神クラス化 — **未着手（v1.x へ送る）**

**現状**
`Renderer.cpp` は 65KB / 44 メソッド。Device・SwapChain・シャドウ・スカイ・水面・トーンマップ・
テクスチャキャッシュ・スプライト・デバッグ描画・フォント生成を 1 クラスが所有している。

**移行手順**
影響が小さい順に抽出していく。
1. `TextureCache`（`m_textureCache` + `GetTextureSRV`）— 依存が閉じているので最初に切り出せる
2. `ShadowPass`（`m_shadowMap*` + `CreateShadowMap` + `ExecuteShadowCommand`）
3. `SkyPass` / `TonemapPass` / `WaterPass`

**影響範囲**: 抽出のたびに `Renderer.hpp` の公開 API が変わるため、呼び出し側の System も追従が必要

### C-5. 描画のソート・バッチング・カリング不在 — **未着手**

**現状**
- `Renderer::Render()` がコマンド列を pass ごとに線形走査する（現在 5 パス）
- マテリアル単位のソートが無いためステート変更が最大化される
- フラスタムカリングが無く、画面外のオブジェクトも全て描画される
- `DrawCommand` が `std::function customDraw` を値で持つため、キュー投入ごとにヒープ確保が起きる
- Transparent パスに奥→手前のソートが無く、半透明同士の前後関係が正しくならない

**移行手順**
`DrawCommandQueue` を pass ごとのバケットに分け、投入時に振り分ける。
ソートキー（pass / マテリアル / 深度）を `DrawCommand` に持たせて、Render の直前に一度だけソートする。

### C-6. 毎フレームのヒープ確保 — **未着手**

**現状**
`FontRendererSystem::Update()` が、フォント 1 個につき毎フレーム
`std::wstring` のコピーとラムダのヒープ確保を行っている。

**移行手順**
`FontComponent` にテキストのバージョン番号を持たせ、変わったときだけ
描画用の文字列を再構築してキャッシュする。

### C-7. EventBus の Publish コスト — **バグは解消済み / 性能は未着手**

**解消済み（正しさに関わる欠陥）**
再入ガードの復帰が `typeid(void)` へのリセットになっており、異なるイベント型を
ネストして Publish した後にガードが外れる問題は修正済み（前の値を復元するようにした）。

**未着手（性能）**
- 発火のたびにハンドラ配列を丸ごとコピーしている（`const auto snapshot = it->second;`）
- ハンドラごとに `std::any(event)` を構築している

いずれも `EventBus.hpp` の冒頭コメントに既知の特性として明記してある。
着手するならスナップショットを「発火中フラグ + 遅延削除」に置き換え、
`std::any` は 1 回だけ構築して使い回す。

### C-8. ビルド品質ゲートの不在 — **未着手**

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

### C-9. `Tsukino.Physics` が空プロジェクト → **解消済み**

**当時の状況**
`Tsukino.Physics/` には 0 バイトの `pch.cpp` しか無く、premake の定義も
`Tsukino.Core` にしかリンクしていなかった（Jolt の includedirs も links も無し）。
参照側は全てコメントアウトされており、誰もリンクしないシンボル0個の `.lib` を毎回ビルドしていた。
物理は全て `Tsukino.EngineIntegration/src/ECS/System/PhysicsSystem.cpp`（53KB / 1120行）に入っていた。

**採った方針: Jolt のラッパをこちらへ移す**

`PhysicsSystem::Impl` をそのまま移すことはできない。`Impl` は EngineIntegration で
宣言された `PhysicsSystem` の入れ子型であり、下層へ置くと
**Physics → EngineIntegration という逆向きの依存**が生まれる。
そのため「移動」ではなく境界を設計し直した。

- `Tsukino.Physics` は `Tsukino.Core` と `JoltPhysics` にのみ依存する。
  `entt` / `Renderer` / `BuiltIn` / `EngineIntegration` は一切参照しない
- 公開ヘッダ（`PhysicsWorld.hpp` / `PhysicsTypes.hpp` / `BodyHandle.hpp` /
  `IPhysicsDebugDraw.hpp` / `PhysicsMath.hpp`）に Jolt の型は現れない。
  座標と回転は hlslpp、エンティティの同一性は `uint64_t` のユーザーデータで受け渡す
- `JPH::BodyID` は `Tsukino::Physics::BodyHandle` に置き換えた。これにより
  データのみのはずだった `Tsukino.BuiltIn` から Jolt の include が消えている
- デバッグ描画は下層に `IPhysicsDebugDraw` を置き、`RendererPhysicsDebugDraw`
  （EngineIntegration）が実装して `Renderer` へ流す。C-1 の移行手順に書いた
  `IPostWorldPass` と同じパターンで揃えてある

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

調べたところ SpringBone は `Tsukino.Engine` の中身に一切依存していなかった
（`Tsukino/Core/typedef.hpp`、`Tsukino/Core/Log.hpp`、`Tsukino/GraphicsCommon/Node/NodeData.hpp`、
hlslpp、cereal のみ）。アセット基盤モジュールに置かれている必然性が無いため、
名前空間をリネームするのではなく **4 ファイルを `Tsukino.Physics` へ移した**。

- `Tsukino/Engine/Physics/SpringBone/*` → `Tsukino/Physics/SpringBone/*`
- 名前空間 `Tsukino::Physics` / `Tsukino::Physics::SpringBonePhysics` は変更なし。
  これでパスと名前空間が一致する
- `Tsukino.Physics` は `Tsukino.GraphicsCommon` と cereal への依存が増えた（どちらも下層）
- `Tsukino.Engine` から物理コードが無くなり、アセット基盤モジュールとして純化した

include を書き換えた側は `SpringBoneComponent.hpp` / `NodeWorldPoseComponent.hpp`（BuiltIn）、
`AnimationSystem.cpp`（EngineIntegration）、ゲーム側の `CombatSystem.cpp` / `ProjectileSystem.cpp`
（`QuatFromToRotation` を使っている）の 5 ファイル。

### C-10. Log が OutputDebugString のみ — **ファイル出力は解消済み**

`Log::SetLogFile()` を呼ぶとファイルへも追記されるようになった。
デバッガを繋いでいない実行でもエラーを追えるようになっている。

ただし**既定では有効にならない**ため、呼び忘れると
「Prefab file not found」のような致命的な警告が引き続き不可視になる。
レベルフィルタは未実装。

### C-11. Sandbox のシーンが 1 ファイル 20〜32KB — **未着手**

エンティティ構築が全てハードコードされている。
`PrefabFactory`（26KB のヘッダオンリー実装）があるのに使われていない。
C-2 で `AssetHandle` が決定的になれば、プレハブへの移行が現実的になる。

### C-12. DrawCommand の生ポインタが暗黙のフレーム契約に依存 — **未着手**

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

### C-13. モーションブラーが素朴な gather 実装 — **未着手**

`MotionBlur.ps.hlsl` は中心ピクセルの速度だけを見て近傍を平均する
最小構成の実装になっている。演出としては十分機能するが、
本格的な絵作りをするなら以下が足りていない。

1. **シルエット外へ滲まない**
   動く物体の内側に背景が引き込まれる方向にしか滲まないため、
   物体が背景へはみ出して尾を引く絵にはならない。
2. **深度を考慮した重み付けが無い**
   手前と奥の物体が混ざるケースで、奥のピクセルが手前に滲む。
3. **フォワードパスが速度を書かない**
   `World` / `Transparent` / `Water` パスと Sky は G-Buffer を経由しないため、
   半透明・水面・デバッグ線・背景はブラー対象外（速度0）になる。

**移行手順**
1 と 2 は速度の dilation（NeighborMax タイルパス）＋深度比較による
重み付けを入れるのが定石。まず画面を 16×16 程度のタイルに分割して
タイル内の最大速度を求めるパスを足し、ブラーパスはそのタイル速度を
探索方向に使う。3 はフォワード側にも速度出力を足すことになるが、
半透明の速度をどう合成するかという別の設計判断が必要になるため、
1・2 を済ませてから検討する。

なお速度バッファそのもの（G-Buffer5・前フレームのボーン行列の退避）は
スキンメッシュまで含めて正しく動いているので、これはブラーパス単体の
品質課題であり、作り直しではなく差し替えで済む。

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
