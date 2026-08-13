# チンチロ（CPU対戦）実装手順

設計内容は `チンチロCPU対戦_設計まとめ.md` を参照。ここでは**どの順番で・何を作って・どう動作確認するか**をステップ化する。

各フェーズは「前のフェーズが動くこと」を前提にしているので、飛ばさずに順番に進める。特にフェーズ3までは**お椀1個・サイコロ1組**という`LuckGameSampleScene`の現状構成のまま検証し、そこから初めて2セット化するのがおすすめ（同時にいじる変数を減らすため）。

---

## フェーズ0：下準備

- [ ] `Tsukino.Sandbox`内に、今回のゲーム固有コード（コンポーネント/システム）をまとめる`LuckGameSampleScene`という名前のフォルダを切る（既存のシーンファイル`LuckGameSampleScene.cpp`とは別物なので混同しないよう注意）
  - `Tsukino.Sandbox/include/Tsukino/Sandbox/LuckGameSampleScene/ECS/Component/`
  - `Tsukino.Sandbox/include/Tsukino/Sandbox/LuckGameSampleScene/ECS/System/`
  - `Tsukino.Sandbox/include/Tsukino/Sandbox/LuckGameSampleScene/ECS/Util/`
  - `Tsukino.Sandbox/src/LuckGameSampleScene/ECS/System/`
  - `Tsukino.Sandbox/src/LuckGameSampleScene/ECS/Util/`
- [ ] 名前空間は`Tsukino::Sandbox::LuckGameSampleScene::ECS`とする（設計まとめ内では省略して`LuckGameSampleScene::ECS`と書いている）
- [ ] ビルド設定（CMakeLists等）に新規ファイルを追加する場所を確認しておく

**確認**: 空のヘッダ1つを追加してビルドが通ることを確認してからフェーズ1へ。

---

## フェーズ1：コンポーネント定義（ロジックなし、データだけ）

対象ファイル（すべて新規）:

- `DiceComponent.hpp`
- `RoundOwnerComponent.hpp`
- `RoundComponent.hpp`（`Hand`enum含む）
- `PlayerComponent.hpp`（`TurnPhase`enum含む）
- `CPUControllerComponent.hpp`
- `GameStateComponent.hpp`（`GamePhase`/`RoundOutcome`enum含む）
- `UILabelTags.hpp`（3つのタグ構造体）

この段階ではまだシステムは書かない。**構造体を定義してコンパイルが通ることだけ確認する。**

**確認**:
- [ ] 全ヘッダをインクルードする空の`.cpp`を1つ作ってビルドが通る
- [ ] `LuckGameSampleScene::OnInitialize`内で試しに`registry.AddComponent<DiceComponent>(someEntity)`を1行呼んでみて、既存のダイスエンティティに問題なく追加できることを確認

---

## フェーズ2：判定パイプラインを既存の1個ダイス構成で試す

**目的**: まずサイコロ1個で「転がる→静止判定→出目確定」までを動かし、物理まわりの数値（閾値・スケール）を先に詰める。3個・2セットに増やすのは後回し。

対象ファイル:

- `DiceRestDetectionSystem.hpp/.cpp`
- `DiceFaceReadSystem.hpp/.cpp`

作業手順:

1. `DiceRestDetectionSystem`を実装し、`LuckGameSampleScene`の既存ダイスエンティティに`DiceComponent`を追加した上で、`Scene::AddSystem`に**優先度16**で登録
2. 実行して、`RigidbodyComponent.linearVelocity`/`angularVelocity`の実際の値をログ出力（`Tsukino::Core::Log`）で確認し、`kVelThreshold`/`kAngThreshold`/`kSettleSeconds`を調整する
   - サンプルシーンのダイスは`10.0f`の高さから落下するので、着地直後にバウンドして再度動き出すケースがある。**「静止継続フレーム数」を短くしすぎないこと**
3. `DiceFaceReadSystem`を実装し、優先度17で登録。`faceNormal[6]`の初期値がモデルの向きと一致しているか確認する
   - **注意**: Dice.fbxのモデル座標系とここで決め打ちしている法線配置(`{1,0,0}〜{0,0,-1}`)が一致しているとは限らない。目視で結果が合わなければ、モデルを回転させて各面がどの軸を向いているか確認し、`faceValue`の対応を実測して直す

**確認**:
- [ ] コンソールログで「静止判定→出目1〜6のいずれかが確定」が毎回安定して出る
- [ ] 出目がモデルの見た目と一致している（目視で上を向いている面の数字と`confirmedValue`が一致）

---

## フェーズ3：役判定を1セットで試す

対象ファイル:

- `HandJudgeSystem.hpp/.cpp`

作業手順:

1. 既存の1個ダイスを**3個に増やす**（`LuckGameSampleScene`のダイス生成ブロックをforループ化）
2. `RoundComponent`を1つ作り、3つのダイスエンティティを紐付ける
3. `HandJudgeSystem`を優先度18で登録
4. ログで役判定結果（`Hand`enumと`subValue`）を出力し、既知の目の組み合わせ（1,1,1 / 4,5,6 / 2,2,5等）を意図的に作って正しく判定されるか確認
   - 物理挙動で狙った目を出すのは難しいので、**デバッグ用に`FaceResult`を強制上書きするデバッグキー**を一時的に仕込むと検証が速い

**確認**:
- [ ] ピンゾロ・アラシ・シゴロ・ヒフミ・目・目なしの6パターンすべてで正しい`Hand`が出ることを確認
- [ ] デバッグ用の強制上書きコードを消す（もしくは`#ifdef _DEBUG`で囲う）

---

## フェーズ4：お椀・ダイスを2セット化

**目的**: ここで初めて「プレイヤー側/CPU側」の左右構成にする。物理・判定ロジックはフェーズ2/3で検証済みなので、ここでは配置とエンティティ紐付けに集中する。

対象:

- `LuckGameSampleScene.cpp`の生成部分を`CreateBowl()` / `CreateDiceSet()`ヘルパー関数に切り出す
- `RoundOwnerComponent`をダイス生成時に付与（`bowlCenter`をセット）
- `PlayerComponent`を持つ2エンティティ（human/cpu）を生成し、`RoundComponent`と紐付け
- カメラ位置を両方のお椀が映る引き位置に変更

**確認**:
- [ ] 左右にお椀2つ、それぞれにダイス3個が表示される
- [ ] 2セットとも独立して`DiceRestDetectionSystem`〜`HandJudgeSystem`が動く（片方だけ判定が壊れていないか確認）

---

## フェーズ5：場外判定

対象ファイル:

- `DiceBoundsSystem.hpp/.cpp`

作業手順:

1. `kOutOfBoundsRadius`を仮の値（60.0f）で実装し、優先度13（`PhysicsSystem`の直後）で登録
2. わざとダイスを強く弾いて场外に出るケースを作り、正しくお椀中心へ戻るか確認
3. 半径の値をお椀のモデルサイズに合わせて調整

**確認**:
- [ ] 場外に出たダイスが自動でお椀中心上空へ戻り、再度転がる
- [ ] 戻った際に`rollCount`が増えていないことをログで確認

---

## フェーズ6：ラウンド進行ロジック

対象ファイル:

- `TurnRuleSystem.hpp/.cpp`
- `CompareSystem.hpp/.cpp`
- `Util/DiceThrowUtil.hpp/.cpp`（`ThrowDiceSet()` / `ResetRoundToIdle()`を共通関数化）

作業手順:

1. `DiceThrowUtil`を先に実装（`ImpulseRequestComponent`を3個のダイスへ付与する処理、およびダイスを`Idle`状態にリセットする処理）。この関数はフェーズ5の`DiceBoundsSystem`でも使っているはずなので、そちらとの重複がないか確認
2. `TurnRuleSystem`を優先度19で実装。目なし/ヒフミの再挑戦（人間は即Idle、CPUは`rerollDelayTimer`セット）と3回失敗の敗北確定を実装
3. `CompareSystem`を優先度20で実装。両者`Resolved`が揃うのを待ち、`GameStateComponent.outcome`を確定

**確認**:
- [ ] 片方だけ目なしを引いた場合に、そちらだけ振り直しになり、もう片方は待機する
- [ ] 3回連続で目なしを引いたプレイヤーが自動敗北になる
- [ ] 役の強さが正しく比較され、`outcome`が正しく確定する（ログで`PlayerWin`/`CpuWin`/`Draw`を確認）

---

## フェーズ7：入力とCPU挙動

対象ファイル:

- `RollTriggerSystem.hpp/.cpp`
- `CPURerollSystem.hpp/.cpp`

作業手順:

1. `RollTriggerSystem`を優先度6で実装。`GamePhase::Ready`中のスペース入力で両者同時に`ThrowDiceSet`
2. `CPURerollSystem`を優先度7で実装。`rerollDelayTimer`のカウントダウンとタイムアップ時の自動振り直し

**確認**:
- [ ] スペースキーで両方のお椀が同時に揺れる
- [ ] CPU側が目なしを引いたとき、即座にではなく少し間を置いてから振り直す（体感で0.6〜1.2秒程度）
- [ ] 人間側が目なしを引いたときは自動で振り直さず、再度スペースキーを押すまで待機する

---

## フェーズ8：UI表示

対象ファイル:

- `LuckGameSampleSceneUISystem.hpp/.cpp`
- `LuckGameSampleScene.cpp`にラベルエンティティ生成コードを追加

作業手順:

1. `CreateLabel()`ヘルパーで3つのラベルエンティティ（CPU役/プレイヤー役/メッセージ）を生成
2. `LuckGameSampleSceneUISystem`を優先度8（`FontRendererSystem`より前）で実装
3. 表示テキストのパターン（待機中/考え中/振り直し待ち/結果）をひと通り実機で確認

**確認**:
- [ ] 役の名前が正しく表示される（ピンゾロ/アラシ/シゴロ/目◯/ヒフミ/目なし）
- [ ] CPUが考えている間だけ「CPUが考えています…」が出る
- [ ] 結果表示時に勝敗/引き分けのメッセージが正しく出る
- [ ] 1フレーム遅延（設計まとめ参照）が体感で気にならないか確認。気になる場合はここで対処を検討

---

## フェーズ9：リザルト〜再戦導線

対象ファイル:

- `ResultInputSystem.hpp/.cpp`

作業手順:

1. `GamePhase::Result`中のスペース入力で`GameSceneManager::ChangeScene`により`LuckGameSampleScene`を再生成
2. 優先度21で登録

**確認**:
- [ ] 結果表示中にスペースキーを押すとシーンが最初から読み直される
- [ ] メモリリークやエンティティの残留がないか（何度か連続でリセットして負荷やクラッシュがないか）確認

---

## フェーズ10：通し確認・調整

ここまでで一通り遊べる状態になっているはず。仕上げとして:

- [ ] 何度か通しでプレイし、静止判定の閾値・場外半径・カメラ位置などを微調整
- [ ] `#ifdef _DEBUG`のデバッグ用強制出目コードなどが残っていないか最終チェック
- [ ] システムの優先度番号が設計まとめの表と一致しているか再確認

---

## フェーズ11（任意・後回しでよい）：演出仕上げ

- 効果音（`AudioSystem`）：着地音、確定音、勝敗音
- エフェクト（`EffectSystem`）：ピンゾロ時の演出など
- カメラ演出：結果表示時のズームなど

---

## 進め方の補足

- **新しい`.cpp`ファイルを追加したら、ビルド前に必ず`premake5 vs2022`（使用しているVisual Studioのバージョンに合わせる）を再実行してプロジェクトファイルを再生成すること。** Premakeは`Tsukino.Sandbox/src/**.cpp`のようなワイルドカードでソースを収集するが、これは`.vcxproj`を生成する時点で一度スキャンされるだけなので、後から追加したファイルは再生成しないとビルド対象に入らない。これを忘れると「ヘッダの宣言はコンパイルが通るのに実装側の`.obj`が無くリンクエラー(LNK2001)になる」という分かりにくい失敗の仕方をするので注意
- **各フェーズの最後で必ず動作確認してから次へ進む**こと。特にフェーズ2（静止判定の閾値調整）はここで手を抜くと後工程すべてに影響するので念入りに
- デバッグ用のログ・強制上書きコードは`#ifdef _DEBUG`で囲うか、都度消してからコミットする
- フェーズ4（2セット化）以降で不具合が出た場合、まず「フェーズ2/3の1セット構成に戻して単独で再現するか」を切り分けると原因特定が早い
