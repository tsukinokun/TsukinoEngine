# レシピ（やりたいことごとの短いお手本）

ゲームを作るときに、ヘッダや実装を読む前にここを見る。コードはどれも実際にビルドされ、
`Tsukino.Sandbox.exe recipes` で1つのシーンにまとめて動く（`--frames=N` で N フレーム後に終了）。

| やりたいこと | 見るファイル（`Tsukino.Sandbox/src/Recipes/`） |
|---|---|
| シーンを作る・システムを正しい順に登録する・カメラとライト | `RecipeScene.cpp` |
| 立方体・平面・球を置く（組み込みモデル） | `PhysicsRecipe.cpp`、`SpawnRecipe.cpp` |
| 重力・当たり判定・動かない床 | `PhysicsRecipe.cpp` |
| 衝突した瞬間に何かする・音を鳴らす | `CollisionSoundRecipe.cpp` |
| 別のエンティティを参照する（`EntityRef`）・自作のコンポーネントとシステム | `FollowRecipe.cpp` |
| 自作シェーダーで描く・値を毎フレーム渡す | `CustomShaderRecipe.cpp`、`../Assets/Shaders/RecipeColor.ps.sc` |
| 実行中に生成・破棄する・時間で消す | `SpawnRecipe.cpp` |

宣言は `Tsukino.Sandbox/include/Tsukino/Sandbox/Recipes/Recipes.hpp` にまとめてある。

## 基本

- **座標系**は左手系（+X 右・+Y 上・+Z 奥）。単位は 1 = 1 メートル。
- **シーン**は `GameSceneBase` を継承し、`OnInitialize` でシステムとエンティティを作り、
  `OnUpdate` で `m_scene.Update(deltaTime)` を呼ぶ。エンティティは `m_scene.CreateEntity()`、
  コンポーネントは `m_scene.GetRegistry().AddComponent<T>(entity)`。
- **エンジンの機能**（AssetManager・AudioManager・Renderer・組み込みアセット）は
  `registry.GetContext<Tsukino::EngineIntegration::EngineContext*>()` から取る。
- **自作のコンポーネント**は普通の struct（登録は要らない）。**自作のシステム**は
  `Tsukino::ECS::ISystem` を継承して `Update(Registry&, float)` を書き、`m_scene.AddSystem(…, 優先度)` で足す。
- **システムは優先度の小さい順**に毎フレーム回る。組み込みの順番は `AddCoreSystems`（`RecipeScene.cpp`）
  のとおり：Physics → Transform → Camera → Model → Light。
  物理は動く剛体の位置を書き換えるので Transform より前（逆だと描画が1フレーム遅れる）。
  `AudioComponent` を使うなら `AudioSystem`、`LifetimeComponent` を使うなら `LifetimeSystem` も要る。
- **TransformComponent** の `position` / `rotation` / `scale` を書き換えたら `dirty = true` にする
  （立てないとワールド行列が作り直されない）。`worldMatrix` は TransformSystem が作る。
- **アセットのパス**は `AssetManager::Load(Tsukino::Core::Path("Assets/…"))` のように、
  アセットのルート（Debug ではリポジトリの直下）からの相対パス。失敗すると無効なハンドルが返り、
  理由はログに出る。

## 組み込みモデル（立方体・平面・球）

`context->builtinAssets->models.cube` / `.plane` / `.sphere` を `ModelComponent::modelHandle` に入れる。
どれも原点中心・大きさ 1（立方体は 1×1×1、平面は XZ の 1×1 で +Y 向き、球は直径 1）。
大きさは `TransformComponent::scale` で変える。モデルのファイルを自分で作る必要は無い。

## 物理

- 見た目は `ModelComponent`、当たり判定は `CollisionComponent`、動き方は `RigidbodyComponent`。
- `CollisionComponent::extent` は**半分の大きさ**で、**Transform の scale を見ない**（ワールドの大きさで書く）。
  Box は各辺の半分、Sphere は `extent.x` が半径、Capsule は `extent.x` が半径・`extent.y` が円柱部分の高さの半分。
  `offsetPosition` で当たり判定だけをずらせる。
- `RigidbodyType::Dynamic` は重力で動く。**PhysicsSystem が毎フレーム `TransformComponent::position` を
  物理の位置で上書きする**ので、動かしたいときは位置ではなく `ImpulseRequestComponent` などで力を加える。
  `Static` は動かない。`Kinematic` は Transform の位置へ物理側が付いてくる（自分で動かす物）。
- **回転は既定ですべての軸が固定**（`freezeRotationX/Y/Z = true`）。転がしたいなら false にする。
- 重力は -9.81（`PhysicsSystem::SetUnitsPerMeter` で単位を変えると合わせて変わる）。
  反発は `restitution`（0 で跳ねない）、摩擦は `friction`。
- 薄い当たり判定は速い物がすり抜けやすい。床は厚みのある Box にして `offsetPosition` で上面を合わせる。
- `CollisionComponent::isSensor = true` は押し返さずに重なりだけを検出する。

## 衝突イベント

- PhysicsSystem が、触れ始めた瞬間に `Tsukino::BuiltIn::ECS::CollisionEnterEvent`（`self`・`other`・`normal`）を
  シーンの `EventBus`（`m_scene.GetEventBus()`）へ出す。離れたときのイベントは無い。
- **1回の衝突で A→B と B→A の2通り**が届く。`self` に注目して処理すれば二重にならない。
- イベントは PhysicsSystem の更新の中で**同期で**届く。
- 購読は `eventBus.Subscribe<CollisionEnterEvent>(関数)`。戻り値の `ScopedConnection` は**メンバで持つ**
  （捨てるとすぐ購読が外れる。持ち主が破棄されると自動で外れる）。

## 音

- `AssetManager::Load` で `.wav` / `.mp3` / `.flac` を読み、
  `std::dynamic_pointer_cast<Tsukino::Asset::AudioAsset>(assetManager->Get(handle))` で取り出して
  `context->audioManager->Play(*audio, ループするか, 音量)` で鳴らす。
- 置いた物に鳴らさせるなら `AudioComponent`（`playOnAwake` / `playTrigger`）と `AudioSystem` でもよい。

## 参照（別のエンティティを指す）

- コンポーネントに `Tsukino::ECS::EntityRef` を持たせ、コードで作るなら `entity` に相手の Entity を入れる。
- Prefab JSON を `PrefabFactory::InstantiateGroup` でまとめて作るときは、`localName`（`"#名前"`）が
  生成後に `entity` へ解決される（例: `Tsukino.Sandbox/src/Scene/SceneSample1.cpp` の EntityRef のテスト）。
- 相手が破棄されていることがあるので、使う前に `registry.IsValid(entity)` で確かめる。

## 自作シェーダー

1. `.sc` を、ゲームの `premake5.lua` の `tsukino_shaders("Assets/Shaders")` のフォルダに置く
   （ビルド時に変換される。足したらビルドし直すだけ。ファイル名は `<名前>.ps.sc` / `<名前>.vs.sc`）。
2. ピクセルシェーダーの入力は組み込みの頂点シェーダーの出力に合わせる：
   `$input v_worldPos, v_normal, v_texcoord0, v_curClip, v_prevClip`。
   `#include` は自分のフォルダ → 組み込み（`Tsukino.BuiltIn/Assets/Shaders/bgfx`）の順に探される。
3. `ModelComponent` と同じエンティティに `CustomShaderComponent` を付け、
   `pixelShader = assetManager->Load(Path("Assets/Shaders/<名前>.ps.sc"))` を入れる。
   ModelSystem が、通常の描画の代わりにそのシェーダーで描く（描画コマンドを自分で積む必要は無い）。
4. `CustomShaderComponent::params[0〜3]`（float4）の値が、シェーダーの `uniform vec4 u_user0[4];` に届く。
   毎フレーム書き換えてよい。経過時間は `#include "Scene.sh"` して `timeParams.x`（秒）でも読める。
- ライティングは掛からない（色はシェーダーが決める）。影も落とさない。頂点シェーダーを変えたいときだけ `vertexShader` も入れる。
- 単に色だけを変えたいなら、シェーダーを書かずに `MaterialPropertyBlockComponent::baseColor` でよい。

## 生成と破棄

- 生成は `Scene::CreateEntity()`。View で回している最中に、その View の種類のコンポーネントを
  足すと反復が壊れるので、作りたいものはいったん集めてループの後で作る。
- 破棄は `Scene::DestroyEntity(entity)`。**予約**で、全システムの更新が終わってから消える
  （システムの中で `Registry::DestroyEntity`（即時）を呼んではいけない）。
- 一定時間で消すだけなら `LifetimeComponent::remaining`（秒）を付ける（`LifetimeSystem` が要る）。

## ログと状態の確認

- `Tsukino::Core::Log::Info/Warn/Error` は、`Log::SetLogFile("Logs/…")` を呼ぶまでデバッガの出力にしか出ない。
  実行して確かめるときは最初に必ず呼ぶ。アセットの読み込み失敗などの警告もここに出る。
- 状態は `registry.View<A, B>()` で回すか、`registry.try_get<T>(entity)`（無ければ nullptr）で読む。
