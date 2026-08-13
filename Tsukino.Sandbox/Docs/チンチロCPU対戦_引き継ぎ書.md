# チンチロ（CPU対戦）引き継ぎ書

作成日: 2026-08-12
最終更新: 2026-08-12（複数セッションにわたり、ダイス投下演出・リスポン・物理バグ修正・出目対応表の実測修正・カメラ/お椀レイアウト調整・自分/敵ラベル追加・静止後の出目ズレ修正・CPU/プレイヤー左右表示の再検証を実施。最新の状態にあわせて本書を整理済み）
対象ブランチ: `feature/add-luck-game-sample`
対象シーン: [LuckGameSampleScene.cpp](../src/Scene/LuckGameSampleScene.cpp)

このドキュメントは、`チンチロCPU対戦_設計まとめ.md` / `チンチロCPU対戦_実装手順.md`（同じ`Docs`フォルダに同梱）に基づいて実装を進めた結果の**現状スナップショット**と、**次に読む人が詰まりそうな箇所**をまとめたもの。まず設計まとめ→実装手順の順で読んでから、このドキュメントで差分を確認するのがおすすめ。

---

## 1. 現在の進捗

実装手順書のフェーズ0〜9まで完了。フェーズ10（通し確認・調整）はかなり進んだが完全ではない。フェーズ11（演出仕上げ）は未着手。

| フェーズ | 内容 | 状態 |
|---|---|---|
| 0 | 下準備（フォルダ構成） | ✅ 完了 |
| 1 | コンポーネント定義 | ✅ 完了 |
| 2 | 静止判定・出目確定（1個ダイス） | ✅ 完了（出目対応表の実測・出目ズレの修正など、後述5-10・5-12） |
| 3 | 役判定（1セット） | ✅ 完了 |
| 4 | 2セット化 | ✅ 完了 |
| 5 | 場外判定 | ✅ 完了 |
| 6 | ラウンド進行ロジック | ✅ 完了 |
| 7 | 入力とCPU挙動 | ✅ 完了（投下演出・リスポンを追加、後述5-7・5-8） |
| 8 | UI表示 | ✅ 完了（**ただし日本語ではなく英語表示**、後述5-1。自分/敵の判別ラベルとデバッグ用出目表示を追加、後述5-11） |
| 9 | リザルト〜再戦導線 | ✅ 完了 |
| 10 | 通し確認・調整 | 🟡 一部のみ。静止判定の速度未同期バグ（5-9）・静止後の出目ズレ（5-12）は修正したが、閾値の実機チューニングと静止判定の遅さ（6章）は未了 |
| 11 | 演出仕上げ（音・エフェクト・カメラ演出） | ❌ 未着手 |

一通りプレイは可能（スペースキーで開始→両者のダイスが空中で静止＋回転しながら待機→スペースで投下→静止判定→役判定→目なし/ヒフミはお椀中心上空へリスポンして再挑戦→3回失敗で敗北確定→勝敗比較→リザルト→スペースキーで再戦）。実機でスペースキー連打・長時間待機を含む通しプレイを複数回確認済み（クラッシュなし）。

---

## 2. ファイル構成

設計まとめの想定と異なり、実際のコードでは名前空間もフォルダ構成も**`Tsukino::Sandbox::` を省略した `LuckGameSampleScene::ECS`** になっている（フェーズ0〜3の時点で既にこの形で実装されていたため、以降もこれに合わせている）。

```
Tsukino.Sandbox/
├─ include/Tsukino/Sandbox/LuckGameSampleScene/ECS/
│  ├─ Component/
│  │  ├─ DiceComponent.hpp         (DiceRollState enum含む。Idle/Respawning/Hovering/Rolling/Settled)
│  │  ├─ RoundOwnerComponent.hpp
│  │  ├─ RoundComponent.hpp        (Hand enum含む)
│  │  ├─ PlayerComponent.hpp       (TurnPhase enum含む)
│  │  ├─ CPUControllerComponent.hpp (isDropPending フラグ含む。後述5-8)
│  │  ├─ GameStateComponent.hpp    (GamePhase/RoundOutcome enum含む)
│  │  └─ UILabelTags.hpp
│  ├─ System/
│  │  ├─ DiceRestDetectionSystem.hpp
│  │  ├─ DiceFaceReadSystem.hpp
│  │  ├─ HandJudgeSystem.hpp
│  │  ├─ DiceDebugOverrideSystem.hpp   (#ifdef _DEBUG)
│  │  ├─ DiceBoundsSystem.hpp
│  │  ├─ DiceRespawnSystem.hpp     (新規。後述5-8)
│  │  ├─ TurnRuleSystem.hpp
│  │  ├─ CompareSystem.hpp
│  │  ├─ RollTriggerSystem.hpp
│  │  ├─ CPURerollSystem.hpp
│  │  ├─ LuckGameSampleSceneUISystem.hpp
│  │  └─ ResultInputSystem.hpp
│  └─ Util/
│     └─ DiceThrowUtil.hpp         (SetupDiceHover/DropDiceSet/RespawnDiceSet/ComputeDiceSpawnOffset等)
├─ src/LuckGameSampleScene/
│  ├─ System/*.cpp  (上記システムの実装)
│  └─ Util/DiceThrowUtil.cpp
├─ src/Scene/LuckGameSampleScene.cpp   (シーン本体。エンティティ生成・システム登録)
└─ Assets/LuckGameSample/
   ├─ Models/Bowl.fbx        (表示用お椀モデル)
   ├─ Models/Bowl_col.fbx    (コリジョン専用の軽量お椀モデル。後述)
   ├─ Models/Dice.fbx
   └─ Prefabs/3DCamera/{Camera.json, Transform.json}
```

---

## 3. システム優先度（実装値）

`Scene::AddSystem(system, priority)` は**値が小さいほど先に実行**。

| 優先度 | システム | 備考 |
|---|---|---|
| 0 | TransformSystem | 既存 |
| 2 | AnimationSystem | 既存 |
| 3 | HeightmapGenerationSystem | 既存（お椀の地形生成） |
| 4 | DebugCameraSystem | 既存（Debugビルドのみ） |
| 5 | CameraSystem | 既存 |
| 6 | **RollTriggerSystem** | 新規：スペース入力で投下/個別投下 |
| 7 | **CPURerollSystem** | 新規：CPUの「考え中」タイマー消化・自動投下 |
| 8 | **LuckGameSampleSceneUISystem** | 新規：UIラベルのテキスト更新 |
| 9 | FontRendererSystem | 既存 |
| 10 | SpriteRenderSystem / ModelSystem / EffectSystem | 既存 |
| 11 | AudioSystem | 既存 |
| 12 | PhysicsSystem | 既存（Dynamic速度の書き戻しバグを修正。後述5-9） |
| 13 | **DiceBoundsSystem** | 新規：場外復帰 |
| 13 | **DiceRespawnSystem** | 新規：リスポン中（Kinematic）のダイスをHoveringへ引き継ぐ。後述5-8 |
| 14 | DirectionalLightSystem | 既存 |
| 15 | SkyAtmosphereSystem / DiceDebugOverrideSystem(DEBUGのみ) | 同一優先度で共存（既存コードにも同様の例あり、問題なし） |
| 16 | DiceRestDetectionSystem | 既存（フェーズ2） |
| 17 | DiceFaceReadSystem | 既存（フェーズ2。5-12で毎フレーム読み直す方式に変更） |
| 18 | HandJudgeSystem | 既存（フェーズ3） |
| 19 | **TurnRuleSystem** | 新規：目なし/ヒフミ再挑戦・3回失敗判定 |
| 20 | **CompareSystem** | 新規：勝敗比較 |
| 21 | **ResultInputSystem** | 新規：リザルト中のスペース入力でシーン再読込 |

設計まとめの表とは一部ズレている（例：DiceBoundsSystemが13、SkyAtmosphereが15など）が、依存関係（PhysicsSystemの後→判定系の前、など）は満たしている。DiceBoundsSystemとDiceRespawnSystemは同じ優先度13だが互いに関与しないエンティティ状態（Rolling vs Respawning）しか見ないため、実行順が入れ替わっても問題ない。

---

## 4. エンティティ構成（実装値）

設計まとめの表とほぼ同じだが、以下は明示しておく。

- お椀2体・ダイス2セット(3個×2)は`LuckGameSampleScene.cpp`内の`CreateBowl()` / `CreateDiceSet()`ヘルパーで生成
- `GameStateComponent`は`registry.SetContext<GameStateComponent>()`でシングルトン登録（`player`/`cpu`にそれぞれのPlayerComponentエンティティを保持）
- UIラベルは`CreateLabel()`ヘルパー＋各タグ(`CpuHandLabelTag`/`PlayerHandLabelTag`/`MessageLabelTag`)で生成
- CPU側`PlayerComponent`エンティティにのみ`CPUControllerComponent`を付与

---

## 5. 設計まとめからの主な変更点・逸脱点（重要）

実装を進める中でエンジンの制約や実機確認で判明した問題により、設計まとめから意図的に外れた箇所がいくつかある。**次に触る人が同じ地雷を踏まないよう必読。**

### 5-1. UIラベルは日本語ではなく英語表示

`Tsukino.BuiltIn`の唯一のフォントアセットは`Arial.spritefont`（DirectXTKのMakeSpriteFontで生成、ASCII相当のみ収録）で、**日本語グリフを含まない**。日本語文字列を`FontComponent::text`に渡すと`DirectX::SpriteFont::DrawString`が`std::exception("Character not in font")`を投げ、キャッチされずに**実機がクラッシュする**（`abort()` → 終了コード3）。

シーンロード後10〜30秒程度でランダムに落ちる不具合の原因がこれだった（フォントアセットの非同期ロード完了タイミングに依存して発生タイミングがブレていた）。[LuckGameSampleSceneUISystem.cpp](../src/LuckGameSampleScene/System/LuckGameSampleSceneUISystem.cpp)の表示文字列は全て英語に変更して回避済み。

**日本語UIに戻したい場合**は、まず日本語グリフを含む`.spritefont`アセットを用意し、`FontComponent::fontHandle`に明示的に設定する必要がある（`Tsukino::BuiltIn::BuiltInAssets::fonts::defaultFont`はASCII専用のまま）。

### 5-2. 場外復帰（DiceBoundsSystem）はテレポートではなく補正インパルス

`RigidbodyComponent::type == Dynamic`のエンティティは、`TransformComponent::position`を直接書き換えても**次の物理フレームでJoltボディの実座標に上書きされる**（`PhysicsSystem`が毎フレーム`bodyInterface.GetPosition()`でTransformへ同期しているため）。そのため「テレポートで戻す」という設計まとめの記述どおりの実装はできない。

代替として、[DiceThrowUtil.cpp](../src/LuckGameSampleScene/Util/DiceThrowUtil.cpp)の`RepositionDiceAboveBowl()`で、**現在の速度を打ち消しつつお椀中心方向へ一定速度で押し戻すインパルス**を`ImpulseRequestComponent`経由で与える形にしている。1フレームでは戻り切らず数フレームかけて戻る点に注意（`kOutOfBoundsRadius`の範囲外にいる限り毎フレーム再度インパルスがかかるので、いずれは戻る）。

なお、`RigidbodyComponent::type`をKinematicに切り替えてから位置を強制し、翌フレームでDynamicに戻す「疑似テレポート」も理論上は可能（`PhysicsSystem`がKinematic用のTransform→Body同期を持っているため）で、**実際に`RespawnDiceSet` + `DiceRespawnSystem`（後述5-8）で採用した**。DiceBoundsSystem自体は改修していないため、場外復帰は引き続きインパルス方式のまま。同じ「テレポートしたい」ニーズが今後DiceBoundsSystemにも出てきたら、5-8のKinematic方式を流用できる。

### 5-3. モデルは実寸スケール。座標系の定数は要注意

お椀モデル（`Bowl.fbx`）の実測AABBは概ね**12×12×6ユニット**（`HeightmapGenerationSystem`の`Bounds -> Min/Max`ログで確認可能）。これを踏まえて以下を実寸に合わせている。当初は1桁以上大きい値（お椀間隔150、カメラ距離400など）で実装していたが、ユーザー指摘を受けて修正した。

| 定数 | 現在値 | 経緯 |
|---|---|---|
| お椀の間隔 `kBowlOffsetX`（[LuckGameSampleScene.cpp](../src/Scene/LuckGameSampleScene.cpp)） | **16.0f**（中心間32ユニット） | 当初20.0f（中心間40ユニット）。ユーザーから「もっと大きく・近くに見せたい」との要望で縮小。`kOutOfBoundsRadius`(14.0f)より必ず大きい値にする必要がある（それより狭いと、自分のお椀の場外判定が確定する前に隣のお椀へ到達してしまう） |
| 場外判定半径 `kOutOfBoundsRadius`（[DiceBoundsSystem.cpp](../src/LuckGameSampleScene/System/DiceBoundsSystem.cpp)） | 14.0f | 未変更 |
| メインの3Dカメラ高さ（[Transform.json](../Assets/LuckGameSample/Prefabs/3DCamera/Transform.json)の`position.y`） | **30.0** | 当初40.0。上記と同じ要望で縮小。22まで下げると寄りすぎて画面端でダイスが切れたため、30で「大きく見える」と「画面に収まる」を両立させた |
| デバッグ用フリーカメラの初期位置 | `(0, 50, -50)` | 未変更。上記メインカメラとは別物（Debugビルドのみのフリーカメラ機能） |
| ダイスの投下待ち間隔 `kDiceSpawnSpacingX`（[DiceThrowUtil.cpp](../src/LuckGameSampleScene/Util/DiceThrowUtil.cpp)） | 2.5f | 当初4.0f。3個ともお椀に収まらず外にはみ出ていたため縮小 |

このあたりはまだ「実測値からの逆算」段階で、実際にプレイしながらの微調整はしていない（フェーズ10の残タスク）。カメラ高さ・お椀間隔は画面1700x1000を前提にした感覚的な調整のため、他の解像度・DPI設定では再調整が必要になる可能性がある（6章・8章の既知の解像度問題も参照）。

### 5-4. 3Dカメラは真上（トップダウン）視点。回転指定が特殊

`CameraSystem`は`up`ベクトルを固定`(0,1,0)`ではなく**カメラ自身の`rotation`から算出**する（`up = rotation * (0,1,0)`）。そのため、カメラを原点の真上に置いて回転なしのまま原点を注視させると、forward `(0,-1,0)` とup `(0,1,0)` が平行になり外積がゼロベクトルになって**ビュー行列が破綻する**（特異点）。

回避のため、[3DCamera/Transform.json](../Assets/LuckGameSample/Prefabs/3DCamera/Transform.json)の`rotation`をX軸90°回転のクォータニオン`(0.7071068, 0, 0, 0.7071068)`にしている。これにより`up`が`(0,0,1)`になり特異点を回避しつつ、`lookAtLH`の計算上**CPU側（`kBowlOffsetX`が負の側）が画面左、プレイヤー側（`kBowlOffsetX`が正の側）が画面右**に来る（UIラベル配置：CPU=左上、YOU=右上と整合）。この左右対応は、5-13節で`lookAtLH`の実装に実際のカメラパラメータを代入して手計算で再確認済み。

このカメラ回転の意味を知らずに「回転が変」と思って`(0,0,0,1)`（無回転）に戻すと、上記の特異点バグが再発するので注意。

### 5-5. コリジョン専用の軽量メッシュ（`Bowl_col.fbx`）とHeightmapGenerationSystemの高速化

[HeightmapGenerationSystem](../../Tsukino.EngineIntegration/src/ECS/System/HeightmapGenerationSystem.cpp)は、お椀モデルの全三角形に対して64×64グリッド全点で総当たりレイキャストする実装だった（空間分割なし）。フェーズ4で1個→2個に増やした結果、**シーンロードに10秒以上**かかるようになった（実測：`Bowl.fbx`4,722三角形×4096サンプルで約5.1秒/個）。

対策として2つ実施:

1. `TerrainGenerationRequestComponent`に`collisionModelHandle`を追加（[TerrainGenerationRequestComponent.hpp](../../Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/TerrainGenerationRequestComponent.hpp)）。指定があれば表示用モデルとは別の軽量メッシュで地形生成する。`LuckGameSampleScene.cpp`の`CreateBowl()`で`Bowl_col.fbx`（140三角形、ユーザー提供の軽量版）を指定している。未指定時は従来どおり表示用モデルを使うため、他シーン（WaterGameSampleScene等）への影響はない。
2. `BuildTriangleBuckets()`を追加し、サンプルグリッドと同じ解像度でXZ平面をセル分割、各三角形をXZ投影AABBが重なる全セルに登録。サンプル点は自セルの候補三角形だけ判定する（レイが常に真下方向なのでXZ包含判定だけで正しく候補を絞れる）。

実測：**約5.1秒/個 → 約25〜29ms/個**（お椀2個で合計10秒以上 → 合計60ms程度）。このシステムは`Tsukino.EngineIntegration`の共有コードなので、他シーンの地形生成にも高速化の恩恵がある（ただし軽量コリジョンメッシュを指定していないシーンは表示用メッシュの三角形数次第でまだ重いまま）。

### 5-6. UILabelTagsに`bool dummy`フィールドが必要だった

`entt`は空の構造体（`struct Foo {};`）を**ストレージなしのタグ**として最適化する。これにより:
- `Registry::AddComponent<T>()`が`T&`を返せない（`registry.emplace<T>()`が`void`を返すため、`return`文がコンパイルエラーになる）
- `view.each()`のコールバックに空型の参照が渡されない（引数の数が合わずコンパイルエラー）

[UILabelTags.hpp](../include/Tsukino/Sandbox/LuckGameSampleScene/ECS/Component/UILabelTags.hpp)の3つのタグ構造体には`bool dummy = true;`を追加してこれを回避している（既存の`Tsukino::BuiltIn::ECS::DebugCameraTag`も同じ理由で同じ回避策を使っている）。**今後空のタグ構造体を追加するときは同様の対処が必要。**

### 5-7. ダイスは「投下待ち（空中で静止＋回転）」→「投下」の二段階になった

当初は`GamePhase::Ready`中のスペース入力で即座に`ThrowDiceSet()`（着地状態から軽く揺する）を呼んでいたが、ユーザー要望で「スペースを押すまで空中で回転を続け、押したタイミングで落とす」演出に変更した。

- `DiceRollState`に`Hovering`を追加。`SetupDiceHover()`（[DiceThrowUtil.cpp](../src/LuckGameSampleScene/Util/DiceThrowUtil.cpp)）が、ダイスの`RigidbodyComponent::freezePositionX/Y/Z`を全てtrueにして重力があっても空中に留め、`torque`に継続的なランダムトルクを与えてその場で回転させ続ける
- `DropDiceSet()`が位置フリーズを解除し、トルクを止めて重力とその時点の角速度に任せて落とす。**`DiceRollState::Hovering`のダイスにしか作用しない**ガード付きなので、誤って複数回呼ばれても多重投下にならない
- `RollTriggerSystem`の`GamePhase::Ready`分岐が`ThrowDiceSet`→`DropDiceSet`に変更
- 投下待ち中の回転トルクの強さ`kHoverSpinTorque`は最初5.0fにしていたが、**待機時間が長いほど角速度が蓄積し、着地時にお椀の外まで弾き飛ばされる**問題が実機で確認できたため0.8fまで下げた（実機で6秒待ってから投下する最悪ケースも確認済み）。まだ暫定値なので、演出的にもっと速く回したい場合は再度チューニングが必要

### 5-8. 振り直しは「その場で揺する」ではなく「リスポン」になった

元の実装は、目なし/ヒフミで振り直しになったダイスを`ResetRoundToIdle()`で`DiceRollState::Idle`にするだけで、**着地した位置のまま**次のスペース入力を待ち、`ThrowDiceSet()`で揺すり直していた。3個が近接した着地位置から同時にシェイクすると**ダイス同士が衝突して激しく暴れる**問題がユーザーから報告されたため、「着地位置に関係なく、お椀中心上空の決まった投下待ち位置へ戻してからHoveringに入る」リスポン方式に変更した。

- `RespawnDiceSet()`（[DiceThrowUtil.cpp](../src/LuckGameSampleScene/Util/DiceThrowUtil.cpp)）が、まず`RigidbodyComponent::type`を`Kinematic`に切り替えて`isTypeDirty=true`にし、`TransformComponent::position`を直接お椀中心上空の投下待ち位置（`ComputeDiceSpawnOffset()`）へ書き換える。Kinematicなら「TransformSystem→PhysicsSystemへ位置を反映」の向きで動くため、5-2で触れた疑似テレポートが機能する。`DiceRollState::Respawning`にマークする
- 新設の`DiceRespawnSystem`（優先度13、PhysicsSystemより後段）が、**前フレームまでにテレポートが反映されたはずの**`Respawning`ダイスを見つけて、`RigidbodyComponent::type`を`Dynamic`へ戻し、`SetupDiceHover()`を呼んでHovering（空中で静止＋回転）へ引き継ぐ
- **なぜ2段階必要か**: `RespawnDiceSet`を呼んだのと同じフレーム内でDynamicへ戻すと、Kinematic→Dynamicの切り替え自体が`PhysicsSystem::Update`内でまだ処理されていない状態でテレポート先の位置が上書きされてしまう（Kinematic用の位置同期処理が先に走らないため）。1フレーム分待ってからDynamic化することで、確実にテレポート後の位置から投下待ちへ移行できる
- `DropDiceSet()`は`Hovering`のダイスにしか作用しないため、リスポン直後のごく短い遷移中（1〜2フレーム）にスペースが押されても何も起きない。呼び出し側（`RollTriggerSystem`・`CPURerollSystem`）は`DropDiceSet()`の戻り値（`bool`、実際に投下できたかどうか）を見て、投下できた場合のみ手番を進めるようガードしている。CPU側は`CPUControllerComponent::isDropPending`で「考え中タイマーは消化済みだがまだHoveringに達していない」状態を区別している
- `TurnRuleSystem`の`ResetRoundToIdle()`呼び出しは`RespawnDiceSet()`に置き換えた。旧`ThrowDiceSet()`/`ResetRoundToIdle()`は呼び出し元が無くなったため削除済み

### 5-9. PhysicsSystemがDynamicボディの実速度をRigidbodyComponentへ書き戻していなかった（重大バグ、修正済み）

[PhysicsSystem.cpp](../../Tsukino.EngineIntegration/src/ECS/System/PhysicsSystem.cpp)の「4. Dynamic同期」処理は、Jolt側の実際の位置・回転はTransformへ書き戻していたが、**`RigidbodyComponent::linearVelocity`/`angularVelocity`は一度も書き戻していなかった**。[DiceRestDetectionSystem.cpp](../src/LuckGameSampleScene/System/DiceRestDetectionSystem.cpp)はこの値を見て「遅くなったら静止」と判定しているため、この値が常に初期値の0のままだと**投げた瞬間から「もう十分遅い」と誤判定し続け**、`kSettleSeconds`（0.3秒）経過した時点で機械的に静止確定・出目確定してしまう。実際にはまだ転がっていても判定が早期に出てしまう不具合として、ユーザーから報告があった。

`PhysicsSystem.cpp`の「4. Dynamic同期」ループで`bodyInterface.GetLinearVelocity`/`GetAngularVelocity`を取得し、Dynamicボディの実速度を`RigidbodyComponent`へ書き戻すよう修正した。`Tsukino.EngineIntegration`の共有コードだが、KinematicはこのDynamic専用ブロックの外なので他シーン（ブロック崩しのボール等、Kinematic駆動）には影響しない。

副次効果として、5-2の`RepositionDiceAboveBowl()`（「現在の速度を打ち消しつつ押し戻す」）も、今まで打ち消す速度が常に0扱いだった（＝実質何もしていなかった）のが、この修正で初めて意図通りに動作するようになった。

**注意**: この修正により`DiceRestDetectionSystem`の閾値（`kLinearVelocityThreshold`/`kAngularVelocityThreshold`/`kSettleSeconds`）は今まで意味を持たなかった状態から一転して実際の物理量に反応するようになった。値そのものは未調整のままなので、実機でダイスの転がり方を見ながら再チューニングが必要（6章の残タスク）。

### 5-10. ダイスの出目対応表（faceValue）の実測（2回の再検証を経て確定）

[DiceComponent.hpp](../include/Tsukino/Sandbox/LuckGameSampleScene/ECS/Component/DiceComponent.hpp)の`faceNormal`/`faceValue`は、ローカル軸(+X,-X,+Y,-Y,+Z,-Z)ごとに出目を対応付けるテーブル。元の値`{1,6,2,5,3,4}`は「対面の和が7になる」という一般則からの**推測で決め打ちされた値**であり、`Dice.fbx`モデルに実際にどの面へ何のピップが描かれているかは一度も確認されていなかった（ユーザーから「UIの出目とモデルの見た目が合っていない」と指摘があり判明）。

**現在の確定値**（`DiceComponent::faceValue`に反映済み）:

| ローカル軸 | faceValue |
|---|---|
| +X | 5 |
| -X | 2 |
| +Y | 1（赤ピップ） |
| -Y | 6 |
| +Z | 3 |
| -Z | 4 |

対面の和はすべて7で自己整合的。**同一条件を2回以上（別プロセスで）再現して一致することを確認済み**。

**確定までの経緯（2回実測している。1回目は誤りだったので注意）**:

1. **1回目の実測**（結果は誤りと判明）: 物理・回転を一時的に止めて6方向すべてを順にワールド上向きへ強制回転させ、実際に描かれているピップ数を目視で数えた。この時点では`{2, 5, 4, 3, 6, 1}`という結果を得て、いったんコードに反映した。
2. **再検証**（ユーザーから「実際のモデルの目に合わせてほしい」との依頼を受けて実施）: 1回目の方法よりも機械的で再現性の高い手順に変更した。
   - `DiceDebugOverrideSystem`（`_DEBUG`限定）に一時デバッグコードを追加し、`F1〜F7`キーでそれぞれのローカル軸を指定。該当するサイコロを`RigidbodyComponent::type = Kinematic`にした上で、`hlslpp::quaternion::rotation_axis`でその軸をワールド上向き(+Y)に向ける回転を計算して姿勢を固定。
   - メインカメラは`(0,30,0)`から原点を見る透視投影のため、お椀中心付近（x=±16）だと遠近感でサイコロの側面まで写り込み出目を誤読しやすい。検証用サイコロは**カメラの光軸直下（ワールド原点付近）**へ一時的にテレポートさせてから撮影することで、真上から見た1面だけが写るようにした。
   - キー入力は`SendKeys`/`SetForegroundWindow`ではなく`PostMessage`で送信（理由は9章参照）。別プロセスで2回実行して同じ結果になることを確認した。
   - この再検証の結果、**1回目の実測`{2,5,4,3,6,1}`自体が誤っていた**ことが判明し、上表の`{5,2,1,6,3,4}`に再修正した。1回目が誤っていた原因は特定できていないが、9章で触れている「別ウィンドウが重なって写り込む」問題の影響を受けていた可能性が高い（1回目はこの対策をしていなかった）。

**もし将来`Dice.fbx`を差し替える場合**は、2回目（再検証）の手順で対応表を実測し直すこと。1回目の「物理を止めて目視するだけ」の方法は、この環境ではフォーカス誤取得やウィンドウの写り込みで誤った結論に至りやすいため避けること。

### 5-11. デバッグ用の出目表示、および自分/敵の判別ラベル

[LuckGameSampleSceneUISystem.cpp](../src/LuckGameSampleScene/System/LuckGameSampleSceneUISystem.cpp)に`DiceValuesDebugLabel()`を追加。`_DEBUG`ビルド時のみ、手役ラベルに実際の出目を`[2,4,4]`形式で追記する（未確定は`?`）。当初はプレイヤー側のみに表示していたが、ユーザーから「エネミーの目も出してほしい」との要望を受けてCPU側にも適用した。**本来は隠し情報のため、Releaseビルドでは従来通り非表示のまま**。

またユーザーから「お椀のあたりに、自分/エネミーがどちらか分かるようにしてほしい」との要望があり、手役ラベルの先頭に`YOU: `/`CPU: `を常時（Release/Debug問わず）付けるようにした。CPU＝画面左のお椀、YOU＝画面右のお椀（5-4節・5-13節参照）。ラベル自体は既存のCPU/プレイヤー手役ラベルと同じ位置（左上x=40／右上x=1350）を流用しているため、新規のラベルエンティティ・新規のスクリーン座標計算は追加していない。

**既知の制約**: プレイヤー側ラベルは画面右上（x=1350）に配置されているが、これは`WinMain.cpp`が想定する1700x1000解像度が前提。実際のウィンドウ解像度・DPI設定によっては画面外にはみ出して見えないことがある（6章の既知課題も参照）。この環境の実機確認では、CPU側（画面左、x=40）の`CPU: [?,?,?]`表示は確認できたが、プレイヤー側は検証環境のウィンドウが1374px幅までしか描画されておらず`YOU:`ラベルを画面上で確認できなかった。コード上はCPU側と全く同じ仕組みなので機能自体は動くはずだが、実際の解像度・DPI設定の環境で見え方を確認すること。

### 5-12. 静止確定後にサイコロがズレて、表示される出目と実際の見た目が食い違うことがあった（修正済み）

ユーザーから「明らかに目の数字の判定が間違ってる」との報告があり、Visual Studioのデバッガでブレークポイント停止した実際の画面（`Draw! Press SPACE to play again` の状態）を見ると、CPU側・プレイヤー側とも `Me 5 [2,2,5]` と表示されているのに、実際にダイスに描かれているピップ数と食い違って見える、という指摘だった。

原因は、[DiceFaceReadSystem.cpp](../src/LuckGameSampleScene/System/DiceFaceReadSystem.cpp)が出目を最初の1回しか読んでいなかったこと。以下の2パターンで、静止確定後にダイスの姿勢がわずかに変わってしまい、`confirmedValue`が古いまま固定されていた。

1. 3個のダイスが近接して着地し、まだ`Rolling`中の他のダイスに接触されて姿勢が変わる
2. 不安定な姿勢で着地したダイスが、明確な衝突なしにゆっくり傾き続ける（速度しきい値を一度も超えないまま姿勢だけが変化する）

**検討した対策と、採用しなかった案**:
- 案1：`Settled`になった瞬間に`RigidbodyComponent`の位置・回転を全軸フリーズする。→ 実装して実機確認したところ、**まだ転がっている他のダイスが、フリーズされて動かないダイスにぶつかって行き場を失い、静止条件（速度がしきい値未満の状態が0.3秒継続）を満たせずいつまでも転がり続ける**という別の不具合を引き起こした（実機で20秒待っても静止しないケースを確認）。お椀という狭い空間で剛体を完全固定するのは物理的に危険と判断し、不採用。
- 案2：`Settled`後も速度を監視し、しきい値を超えて動いたら`Rolling`へ戻して読み直す（上記1のパターンだけを狙い撃ちする案）。→ 単体ダイスで検証したところ、**上記2のパターン（しきい値を超えないゆっくりとした傾き）には反応せず、根本原因を取りこぼす**ことが分かった。不採用。

**採用した対策**: `DiceFaceReadSystem`の「1回だけ読む」ガード（`dice.confirmed`）を外し、**`Settled`中は毎フレーム出目を読み直し続ける**ようにした。ただし`_DEBUG`限定の`DiceDebugOverrideSystem`（数字キーで役を強制発生させる機能）が、まだ物理的にHovering中で回転しているダイスに`state=Settled`を強制するケースがあり、そのまま毎フレーム読み直すとデバッグ機能が壊れる（強制した値が次のフレームで実際の姿勢に上書きされてしまう）。これを避けるため、**`RigidbodyComponent::freezePositionX`が立っている（＝Hovering中にデバッグ強制されただけで、本当の物理着地ではない）ダイスは読み直し対象から除外**するガードを追加した（実際の着地ではフリーズは使っていないため、正規の`Settled`ダイスには影響しない）。

**検証方法**: `_DEBUG`限定の一時デバッグキー（2個目3個目のダイスを空の彼方へ隔離→1個だけで転がす／静止確定したダイスを回転そのまま原点付近のカメラ直下へテレポートして真上から確認）を一時的に追加し、ダイス1個だけで検証した。`confirmedValue`が明確な「ぶつかり」なしに2→1→3と変化する様子を実際に観測でき、上記の原因2（ゆっくりとした傾き）を再現・確認できた。検証用の一時キーは確認後に削除済み。

実機確認では、`D1`キー（ピンゾロ強制）を押した直後・2秒後の両方で`CPU: PinZoro!! [1,1,1]`表示が変わらず保持されることを確認し、デバッグ機能が壊れていないことも確認済み。

自然な物理挙動での通しプレイでも複数回確認したが、**静止判定のしきい値自体（`kLinearVelocityThreshold`/`kAngularVelocityThreshold`/`kSettleSeconds`）が暫定値のままなこともあり、3個のダイスが近接した状態からお互いに接触を繰り返して静止確定までに20秒以上かかるケースがあった**（この現象は今回の修正前のコードでも再現するため、今回の修正で新たに埋め込んだ不具合ではなく、6章にある既存の既知課題）。出目のズレそのものは解消したはずだが、根本的な「静止判定が遅い／なかなか静止しない」問題は引き続き6章の残タスク。

### 5-13. CPU/プレイヤーの左右表示についての調査（不具合は未発見）

ユーザーから「CPUとプレイヤーの表示が逆に見える」との指摘があった。`CameraSystem`が使う`matrix::lookAtLH`の実装（[Matrix.cpp](../../Tsukino.Core/src/Math/Matrix.cpp)の`axis_x = normalize(cross(worldUp, axis_z))`）に、実際のカメラパラメータ（`eye=(0,30,0)`, `lookAt=(0,0,0)`, `up=(0,0,1)`）を代入して手計算したところ、`axis_x = (1,0,0)`となり、**ワールド+X方向が画面右に対応する**ことを確認した。`playerBowlCenter = (+kBowlOffsetX, 0, 0)`・`cpuBowlCenter = (-kBowlOffsetX, 0, 0)`であり、UIラベルも`CpuHandLabelTag`は画面左(x=40)・`PlayerHandLabelTag`は画面右(x=1350)に配置されているため、**コード上はCPU＝画面左、プレイヤー（YOU）＝画面右で一貫しており、数式上の矛盾は見つからなかった**。

また、CPU側だけを自動進行させる`CPUControllerComponent`は`cpuEntity`（`cpuRoundEntity`＝`cpuBowlCenter`＝画面左のはずの側）に付与されており、実機でスペースキーを1回だけ押して放置するテストでは、画面左のお椀が人間側の追加入力なしに自動で手役を確定・進行した（`CPU: PinZoro!!`等）ことも確認済みで、これも「CPU＝画面左」という理解と整合する。

以上より、**この調査ではCPU/プレイヤーの左右が入れ替わっている不具合は再現・特定できなかった**。もし引き続き逆に見える場合は、実際の画面のスクリーンショットを確認すること（デバッグ用フリーカメラ`(0,50,-50)`に切り替わっていてメインカメラと異なる視点だった可能性、解像度・DPI起因でラベルの位置がずれて見えた可能性などを疑う）。

---

## 6. 既知の課題・要調整項目（フェーズ10の残り）

- [ ] `DiceRestDetectionSystem`の静止判定閾値（`kLinearVelocityThreshold`/`kAngularVelocityThreshold`/`kSettleSeconds`）は実測ベースの暫定値のまま。5-9の修正で実際の速度に反応するようになったので、**今から初めて意味のあるチューニングができる状態**。実機で転がり方を見ながら調整すること
- [ ] 上記に関連し、3個のダイスが近接着地してお互いに接触を繰り返し、静止確定（全個体の`confirmed`）まで20秒以上かかるケースを複数回確認した（5-12節参照）。ゲームが壊れる・クラッシュするわけではないが体感的に長い。しきい値調整、またはお椀の間隔・ダイスの反発係数(`restitution`)などの見直しが必要
- [ ] `kHoverSpinTorque`（投下待ち中の回転の強さ、[DiceThrowUtil.cpp](../src/LuckGameSampleScene/Util/DiceThrowUtil.cpp)）は暫定値0.8f。もっと勢いよく回したい場合は要調整（ただし強くしすぎると5-7で説明した「弾き飛ばされる」問題が再発するので注意）
- [ ] `kOutOfBoundsRadius`(14.0f)は実寸スケールへの換算値であり、実際にダイスを強く弾いて場外復帰の挙動を確認していない
- [ ] UIラベルのスクリーン座標（画面1700x1000想定の目分量）は未調整。テキストの実際の描画幅を見て左上/右上/下部中央の位置を詰める。5-11で追加したデバッグ出目表示・自分/敵ラベルが環境によっては画面外に出てしまう問題も含む
- [ ] `DiceDebugOverrideSystem`（数字キー1〜6、Debugビルドのみ）は**両方のお椀に同時に**強制出目を適用する仕様のまま。プレイヤー側とCPU側を個別にテストしたい場合は改修が必要。また、強制発動時にダイスの物理状態（Kinematic/Dynamic、フリーズ、トルク）をリセットしないため、Hovering中に発動すると見た目と物理がちぐはぐになる場合がある
- [ ] フェーズ11（音・エフェクト・カメラ演出）は完全に未着手
- [ ] Releaseビルドでの動作確認は未実施（今回もDebugビルドのみで検証）
- [ ] CPU/プレイヤーの左右表示が逆に見えるという指摘（5-13節）は、コード・数式上は再現できなかった。もし再発を確認できたら、その時の画面スクリーンショットを添えて再調査すること

---

## 7. ビルド・実行方法

```bash
cd C:\Users\tamami197508\Desktop\TsukinoEngine
vendor\premake5.exe vs2022
```

**新規`.cpp`を追加した後は必ずpremake再実行**（Premakeはワイルドカードでソースを収集するが、`.vcxproj`生成時に一度スキャンするだけなので、後から追加したファイルは再生成しないとビルド対象に入らない。忘れると「ヘッダはコンパイルが通るのに実装.objが無くてLNK2001」という分かりにくい失敗になる）。JSONアセット（Transform.jsonなど）の変更は再ビルド不要で、実行時に読み込まれる。

MSBuildは本環境では以下のパスにあった（バージョン表記が特殊な"18\Community"だったので、通常のVisual Studio 2022環境ではパスが異なるはず。`C:\Program Files\Microsoft Visual Studio\2022\Community\...`等を探す）:

```
"C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\amd64\MSBuild.exe" .build\TsukinoEngine.sln -t:Tsukino_Sandbox -p:Configuration=Debug -p:Platform=x64
```

実行:

```
bin\Debug\Tsukino.Sandbox.exe
```

---

## 8. 操作方法

- **スペースキー**:
  - `Ready`中: 両者のダイスが空中で静止＋回転しながら投下を待っている状態から、両者同時に投下（5-7参照）
  - 自分（人間側）が目なし/ヒフミで振り直し待ちのとき: リスポンして投下待ちに戻った自分のダイスだけを投下（CPU側は自動、5-8参照）
  - `Result`表示中: シーンを丸ごと再読込して再戦
- **CPUの振り直し**: 0.6〜1.2秒のランダムな「考え中」演出＋リスポン完了待ちを挟んで自動投下
- **デバッグキー（Debugビルドのみ、`DiceDebugOverrideSystem`）**: `1`〜`6`キーで役を強制発生（1=ピンゾロ, 2=アラシ, 3=シゴロ, 4=ヒフミ, 5=目, 6=目なし）。**両方のお椀に同時適用**される点に注意（6章も参照）

---

## 9. この環境での動作確認の方法（参考）

このリポジトリはネイティブWin32/DirectXアプリで、ブラウザ経由のテストができない環境だったため、以下の方法で検証した。次に触る人が同様の検証をしたい場合の参考に。

- `vendor\premake5.exe vs2022` → MSBuildでビルド
- PowerShellから`Start-Process`で実際に`.exe`を起動し、スクリーンショット（`System.Drawing.Graphics.CopyFromScreen`）でカメラ位置やお椀・ダイスの見た目を目視確認しつつ、プロセスが生きているか(`$p.HasExited`)を監視してクラッシュの有無を確認

### キー入力の自動送信について（重要）

この環境はユーザーの実デスクトップを共有しており、**`SendKeys`/`keybd_event`によるキー送信は信頼できないことがある**。これらは`SetForegroundWindow`で対象ウィンドウへ実際にフォーカスを移せていることが前提だが、この環境では`AttachThreadInput`を併用してリトライしても`SetForegroundWindow`が恒常的に失敗し、`GetForegroundWindow`が別のウィンドウを指したまま変化しないケースがあった。この状態で`SendKeys`を送っても**エラーは出ないがキーはどこにも届かない**ため、「ダイスの見た目が変わった＝キーが効いた」と早合点すると誤り（実際にはHovering中の自然な回転で見た目がフレームごとに変わるだけで、デバッグ機能は一度も呼ばれていなかった、という誤検出が実際に起きたことがある）。

**対策**: 対象HWNDへ`PostMessage`で`WM_KEYDOWN`/`WM_KEYUP`（`wParam`=仮想キーコード）を直接送る方式なら、フォーカスの成否に関係なく確実に届く。このゲームの入力処理は`Window::WindowProc`内の標準的なWM_KEYFIRST〜WM_KEYLAST処理であり、フォーカス済みウィンドウへのメッセージキュー配送に依存しないため。デバッグ用のキー入力を自動送信して検証したい場合は、最初から`PostMessage`方式を使うこと。

ただし`PostMessage`方式でも、**直前のキー入力から数百ms程度しか間隔を空けないと後続のキーが処理されないことがある**現象を確認している（原因未特定。前のキー入力から2秒程度空けると確実に処理された）。**複数のキーを連続で送る場合はキーごとに1〜2秒程度の間隔を空けること。**

副作用の検証（本当に処理が呼ばれたか）は、一時的にファイル書き込みログを仕込んでフレームごとの状態をテキストファイルに出力し、`Read`ツールで直接確認するのが最も確実だった（`OutputDebugStringA`によるログはデバッガ非接続だと確認できない）。

### スクリーンショット・目視確認について

- この環境では`SetForegroundWindow`が信頼できないことがある（他のウィンドウ・ブラウザタブに実際のフォーカスがあることがあり、`GetForegroundWindow`で確認すると一致しないケースがあった）。スクリーンショットが正しく撮れているように見えても、実は別ウィンドウが重なっているだけで対象ウィンドウの内容ではない場合があるため注意。`SetWindowPos`で一時的に`HWND_TOPMOST`にしてから撮影し、撮影後に`HWND_NOTOPMOST`へ戻す方法がより確実だった。ユーザーの作業を妨げないよう、この種の操作は最小限に留めること
- ウィンドウが最小化されて`GetWindowRect`が極小サイズを返すことがあった。撮影前に`ShowWindow(hwnd, SW_RESTORE)`を呼んでおくと安全
- 出目対応表（5-10）の実測のように、物理現象を目視で正確に読み取りたい場合は、**同一条件を2回以上（できれば別プロセスで）再現して一致することを確認**してから結論を出すこと。1回だけの観測は、フォーカス誤取得やタイミング起因のノイズで誤った結論に至るリスクがある
- クラッシュ原因の特定には、`WinMain.cpp`に一時的に`try/catch`と`_CrtSetReportFile`（assertをダイアログでなくstderrへ）を仕込み、`HeightmapGenerationSystem.cpp`や`Log.cpp`に一時的に`std::chrono`計測とstdout出力を仕込んで実測。**いずれも調査後は元に戻し、リポジトリに調査用コードは残っていない**

---

## 10. 関連ドキュメント

- [チンチロCPU対戦_設計まとめ.md](./チンチロCPU対戦_設計まとめ.md) — 元の設計ドキュメント（本ディレクトリにコピー済み）
- [チンチロCPU対戦_実装手順.md](./チンチロCPU対戦_実装手順.md) — 元の実装手順書（本ディレクトリにコピー済み）
