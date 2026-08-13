# チンチロ（CPU対戦）設計まとめ

対象エンジン: [TsukinoEngine](https://github.com/tsukinokun/TsukinoEngine)（EnTTベースECS + Jolt Physics統合）
ベースシーン: `Tsukino.Sandbox/src/Scene/LuckGameSampleScene.cpp`

> 以降、独自コンポーネント/システムを置く名前空間も`LuckGameSampleScene`という名前にする（シーンファイル名と同名）。文中で単に「`LuckGameSampleScene`」と書いた場合、シーンファイル（既存の`LuckGameSampleScene.cpp`）を指すのか名前空間を指すのかは文脈で判断すること。

---

## 1. ゲーム概要

- プレイヤー1人 vs CPU1人の対戦
- お椀を左右に2つ配置し、**両者が同時に**サイコロ3個を振る（伝統的な椀振りスタイル）
- チップ・賭け金システムはなし。1ラウンドで勝敗（または引き分け）が決まったら結果を表示し、スペースキーでシーンごと読み直して再戦する

---

## 2. 確定ルール

| 項目 | 内容 |
|---|---|
| 投げ方 | 1人3個を同時に投げる |
| 目なし/ヒフミ（役なし） | 最大3回まで振り直し可能。3回とも役なしならそのプレイヤーは即敗北確定 |
| 引き分け（役・値が同じ） | そのままResult表示（自動再振りはしない） |
| 両者とも3回失敗 | 引き分け扱い |
| ラウンド終了後の遷移 | 結果表示のまま待機。スペースキーでシーンを丸ごと読み直して再戦（個別リセット処理は書かない） |
| 場外判定 | お椀中心から一定半径を超えたサイコロは自動で上空に戻し、ノーカウントで振り直し（rollCountは増やさない） |
| CPUの振り直し | 即座にではなく0.6〜1.2秒程度の「考える間」を演出として挟む |
| CPU難易度 | 今回は実装しない（`Difficulty`は導入しない） |
| チップ/賭け金 | なし |

### 役の強さ（比較順位）

```
ピンゾロ（1,1,1） > アラシ（ゾロ目 2〜6） > シゴロ（4,5,6） > 目（2つ揃い+1つ、値が大きいほど強い）
```
※ヒフミ・目なしは「役なし」として、確定した役同士の比較には出てこない（3回以内に決着するか、3回失敗で敗北扱いになるため）

---

## 3. エンティティ構成

| エンティティ | 主なコンポーネント |
|---|---|
| お椀 ×2（CPU側/プレイヤー側） | `TransformComponent`, `ModelComponent`, `CollisionComponent`(Heightfield), `TerrainGenerationRequestComponent`, `RigidbodyComponent`(Static) |
| サイコロ ×3 × 2セット | `TransformComponent`, `ModelComponent`, `CollisionComponent`(Box), `RigidbodyComponent`(Dynamic, freezeRotation解除), `DiceComponent`, `RoundOwnerComponent` |
| プレイヤー（人間） | `PlayerComponent` |
| プレイヤー（CPU） | `PlayerComponent`, `CPUControllerComponent` |
| ラウンド管理 ×2（各プレイヤーに1つ） | `RoundComponent` |
| UIラベル ×3 | `TransformComponent`, `FontComponent`, タグ(`CpuHandLabelTag`/`PlayerHandLabelTag`/`MessageLabelTag`) |
| 3Dカメラ | `TransformComponent`, `CameraComponent`（両方のお椀が入る引き位置） |
| 2Dカメラ | `TransformComponent`, `CameraComponent`（UI描画用の正射影） |

---

## 4. 独自コンポーネント一覧（`LuckGameSampleScene::ECS`名前空間）

```cpp
enum class DiceRollState { Idle, Rolling, Settled };

struct DiceComponent {
    hlslpp::float3 faceNormal[6];
    u8             faceValue[6];
    DiceRollState  state          = DiceRollState::Idle;
    float          settleTimer    = 0.0f;
    u8             confirmedValue = 0;
    bool           confirmed      = false;
};

struct RoundOwnerComponent {
    hlslpp::float3 bowlCenter;    // 所属するお椀のワールド座標（場外判定に使用）
};

enum class Hand : u8 { None, MeNashi, Me, HiFuMi, Shigoro, Arashi, PinZoro };

struct RoundComponent {
    std::array<Tsukino::ECS::Entity, 3> dice{};
    Hand kind     = Hand::None;
    u8   subValue = 0;
    bool judged   = false;
};

enum class TurnPhase { Waiting, Rolling, Resolved };

struct PlayerComponent {
    TurnPhase             phase       = TurnPhase::Waiting;
    u8                     rollCount   = 0;         // 目なし/ヒフミでの振り直し回数（3で即敗北）
    bool                   eliminated  = false;
    Tsukino::ECS::Entity   roundEntity = entt::null;
};

struct CPUControllerComponent {
    float rerollDelayTimer = 0.0f;    // 0より大きい間は「考え中」
};

enum class GamePhase { Ready, Rolling, Compare, Result };
enum class RoundOutcome { None, PlayerWin, CpuWin, Draw };

struct GameStateComponent {    // registry.SetContext<GameStateComponent>() で登録
    GamePhase             phase   = GamePhase::Ready;
    RoundOutcome           outcome = RoundOutcome::None;
    Tsukino::ECS::Entity   player  = entt::null;
    Tsukino::ECS::Entity   cpu     = entt::null;
};

// UIラベル識別用タグ（データを持たないマーカー）
struct CpuHandLabelTag {};
struct PlayerHandLabelTag {};
struct MessageLabelTag {};
```

---

## 5. システム一覧と実行優先度

`Scene::AddSystem(system, priority)` の優先度は**小さいほど先に実行**される。既存の`LuckGameSampleScene`の並びに、チンチロ固有システムを差し込む形。

| 優先度 | システム | 備考 |
|---|---|---|
| 0 | `TransformSystem` | 既存 |
| 2 | `AnimationSystem` | 既存 |
| 3 | `HeightmapGenerationSystem` | 既存（お椀の地形生成） |
| 4 | `DebugCameraSystem` | 既存（Debugビルドのみ） |
| 5 | `CameraSystem` | 既存 |
| 6 | `RollTriggerSystem` | **新規**：人間の入力を検知し両者に`ImpulseRequestComponent`を発行 |
| 7 | `CPURerollSystem` | **新規**：`rerollDelayTimer`を消化してCPU側のみ振り直しを実行 |
| 8 | `LuckGameSampleSceneUISystem` | **新規**：ラベルの`FontComponent.text`を更新（**FontRendererSystemより前**に置くこと） |
| 9 | `FontRendererSystem` | 既存 |
| 10 | `SpriteRenderSystem` / `ModelSystem` | 既存 |
| 11 | `AudioSystem` | 既存 |
| 12 | `PhysicsSystem` | 既存（Jolt統合、ここで物理更新が反映される） |
| 13 | `DiceBoundsSystem` | **新規**：場外に出たサイコロを検知しお椀中心へ戻す |
| 14 | `DirectionalLightSystem` | 既存 |
| 15 | `SkyAtmosphereSystem` | 既存 |
| 16 | `DiceRestDetectionSystem` | **新規**：速度・角速度から静止判定 |
| 17 | `DiceFaceReadSystem` | **新規**：静止したサイコロの出目を確定 |
| 18 | `HandJudgeSystem` | **新規**：3つの出目から役を判定 |
| 19 | `TurnRuleSystem` | **新規**：目なし/ヒフミの再挑戦・3回失敗の判定 |
| 20 | `CompareSystem` | **新規**：両者の役を比較し勝敗/引き分けを`GameStateComponent`に確定 |
| 21 | `ResultInputSystem` | **新規**：Result中のスペース入力で`GameSceneManager::ChangeScene`によりシーン再読込 |

> **既知の注意点**: `LuckGameSampleSceneUISystem`(8)は`FontRendererSystem`(9)より前だが、判定系システム(16〜20)は`PhysicsSystem`(12)より後に動くため、**その日の判定結果がUIに反映されるのは次フレーム**になる（1フレーム遅延）。チンチロは決着まで秒単位の余裕があるため実用上は気にならない想定だが、気になれば`LuckGameSampleSceneUISystem`を判定系の後にもう一度呼ぶなどで解消可能。

---

## 6. ゲームフロー（状態遷移）

```
[Ready]
  ├─ プレイヤーがSpace押下
  │     → 両者のRoundComponentにImpulseRequestを同時発行、phase = Rolling
  ↓
[Rolling]
  ├─ DiceRestDetectionSystem / DiceFaceReadSystem / HandJudgeSystem が進行
  ├─ TurnRuleSystemが目なし/ヒフミを検知
  │     ├─ 人間側 → 即座にIdleへ戻し、再度Space待ち（手動振り直し）
  │     └─ CPU側  → rerollDelayTimerをセットし「考え中」演出、CPURerollSystemが自動で振り直し
  ├─ rollCount >= 3 → eliminated = true として TurnPhase::Resolved 扱いに
  ├─ 両者が TurnPhase::Resolved になったら CompareSystem が起動
  ↓
[Compare / Result]
  ├─ 片方だけ eliminated → 相手の勝ち
  ├─ 両方 eliminated → Draw
  ├─ 役の強さを比較 → 勝敗 or 同値ならDraw
  ├─ state.phase = Result、メッセージ表示
  ↓
  └─ プレイヤーがSpace押下 → GameSceneManager::ChangeScene(同じシーンを再生成) → [Ready]に戻る
```

---

## 7. レイアウト・演出メモ

- お椀は `x = -150`（CPU側） / `x = +150`（プレイヤー側）に配置、地形生成の`seed`を変えて左右で同じ形にならないようにする
- 3Dカメラは `position = (0, 220, -400)`、`lookAtTarget = (0,0,0)` で両方のお椀を画角に収める引き位置
- UIラベルは2Dカメラ（`orthoSize = 1000`）のスクリーン座標系に配置
  - CPUの役表示: 左上
  - プレイヤーの役表示: 右上
  - メッセージ（開始案内/考え中/結果）: 下部中央
- サイコロの`RigidbodyComponent`は`freezeRotationX/Y/Z`を**明示的にfalse**にする（デフォルトtrueだと転がらない）

---

## 8. 未着手・今後の課題（優先度低め、後回しでよいもの）

- 効果音・エフェクト演出（`AudioSystem` / `EffectSystem`は統合済みなので着手コストは低い）
- ゲームパッド対応の要否
- リザルト演出の凝り具合（カメラズーム等）
- 「考え中」演出のCPU吹き出し表示など、UIの見栄え向上

---

## 9. 実装順の目安

1. `DiceComponent` / `RoundComponent` / `RoundOwnerComponent` 等のコンポーネント群を追加
2. `LuckGameSampleScene`のダイス・お椀生成部分をヘルパー関数化し2セット分に拡張
3. `DiceRestDetectionSystem` → `DiceFaceReadSystem` → `HandJudgeSystem` の判定パイプラインを組んで、まず「投げたら役が判定される」ところまで動作確認
4. `TurnRuleSystem` / `CompareSystem` でラウンド進行ロジックを追加
5. `RollTriggerSystem` / `CPURerollSystem` で入力・CPU挙動を実装
6. `LuckGameSampleSceneUISystem` + UIラベルエンティティで画面表示
7. `DiceBoundsSystem`（場外対策）と`ResultInputSystem`（再戦導線）を仕上げに追加
