# レシピ（実際にビルドされて動くお手本）

使い方と挙動の説明はエンジン直下の `GAMEDEV.md` にある（ここには写さない）。ここはコードの置き場所の索引。
`Tsukino.Sandbox.exe recipes` で全部が1つのシーンで動く（`--frames=N` で N フレーム後に終了。ログは `Logs/Tsukino.log`）。

| やりたいこと | 見るところ |
|---|---|
| シーンの土台（システム・カメラ・ライト・床） | `Tsukino.Sandbox/src/Recipes/RecipeScene.cpp` の冒頭 |
| 落ちて床に当たったら音を鳴らす（Dynamic × Static） | 同 1. |
| 別のエンティティを追いかける | 同 2. |
| 自作シェーダーで描き、値を毎フレーム渡す | 同 3. と `Tsukino.Sandbox/Assets/Shaders/RecipeColor.ps.sc` |
| 一定間隔で生成し、時間で消す | 同 4. |
| 自分で動かす物（Kinematic）で当たりを取る（センサー）・破棄 | 同 5. |
| Prefab JSON で複数作り、自作コンポーネントの `EntityRef`（`"#名前"`）で参照する | `Tsukino.Sandbox/src/Recipes/PrefabRecipe.cpp` と `Tsukino.Sandbox/Assets/Prefabs/Recipe/OrbitGroup.json`（1ファイル） |
