# API ダイジェスト

**このファイルは自動生成です。直接編集しないでください。**

再生成: `generate-docs.bat`（doxygen で XML を出してから gen-api-digest）

エンジンのヘッダを直接読む前に、まずここを引くこと。
コンポーネントと Prefab JSON のフィールドは `components.md` を見る。

## 主要な型

ゲーム側から実際に触る型。メンバを全て展開している。

### Tsukino::Audio::AudioManager

`Tsukino.Audio/include/Tsukino/Audio/AudioManager.hpp`

**公開関数**

| シグネチャ | 説明 |
|---|---|
| `AudioManager()` | コンストラクタ |
| `~AudioManager()` | デストラクタ |
| `AudioManager(const AudioManager &)=delete` |  |
| `AudioManager & operator=(const AudioManager &)=delete` |  |
| `AudioManager(AudioManager &&)=delete` |  |
| `AudioManager & operator=(AudioManager &&)=delete` |  |
| `bool Initialize()` | 初期化 |
| `void Update(float deltaTime)` | 更新関数 |
| `void Play(const Tsukino::Asset::AudioAsset &audioAsset, bool isLoop=false, float volume=1.0f)` | 音声を再生する |
| `void Stop(const Tsukino::Asset::AudioAsset &audioAsset)` | 特定の音声を停止する |
| `void StopAll()` | 全ての音声を停止する |
| `bool IsPlaying(const Tsukino::Asset::AudioAsset &audioAsset) const` | 特定の音声が再生中か確認する |
| `void SetMasterVolume(float volume)` | マスター音量を設定する |
| `float GetMasterVolume() const` | マスター音量を取得する |

### Tsukino::Core::Log

`Tsukino.Core/include/Tsukino/Core/Log.hpp`

**公開関数**

| シグネチャ | 説明 |
|---|---|
| `void Info(const std::string &msg)` | 情報ログ出力 |
| `void Warn(const std::string &msg)` | 警告ログ出力 |
| `void Error(const std::string &msg)` | エラーログ出力 |
| `void SetLogFile(const std::string &filePath)` | ログの出力先ファイルを設定します。 |
| `void CloseLogFile()` | ファイルへのログ出力を停止します。 |

### Tsukino::Core::Path

`Tsukino.Core/include/Tsukino/Core/Path.hpp`

**公開関数**

| シグネチャ | 説明 |
|---|---|
| `Path()=default` | コンストラクタ |
| `Path(const std::string &path)` | コンストラクタ |
| `const std::string & string() const noexcept` | パス文字列を取得する関数 |
| `Path parent() const` | パスの親ディレクトリを取得する関数 |
| `std::string filename() const` | パスのファイル名を取得する関数 |
| `std::string extension() const` | パスの拡張子を取得する関数 |
| `std::string stem() const` | 拡張子を除いたファイル名を取得する関数 |
| `Path parent_path() const` | パスの親ディレクトリを取得する関数 |
| `std::wstring ToWString() const` | パスをwide string (UTF-16) に変換して取得する関数 |
| `Path operator/(const std::string &rhs) const` | パスを結合する演算子 |
| `Path operator/(const Path &rhs) const` | パスを結合する演算子 |
| `void replace_extension(const std::string &newExt)` | 拡張子を置き換える関数 |
| `std::pair< std::string, std::string > SplitPathAndFragment(const std::string &rawPath)` | フラグメントなしとフラグメントを分割して取得する関数 |
| `std::string ToLower(std::string value)` | パスを正規化して小文字に変換する関数 |

### Tsukino::Core::Window

`Tsukino.Core/include/Tsukino/Core/Window.hpp`

**公開関数**

| シグネチャ | 説明 |
|---|---|
| `Window()` | コンストラクタ |
| `~Window()` | デストラクタ |
| `bool Create(const std::string &title, int width, int height, WindowStyle style=WindowStyle::Default)` | ウィンドウ生成 |
| `bool ProcessMessages()` | メッセージ処理 |
| `void SetMessageCallback(MessageCallback callback)` |  |
| `void SetResizeCallback(ResizeCallback callback)` |  |
| `void SetFocusLostCallback(FocusLostCallback callback)` |  |
| `void SetHitTestCallback(HitTestCallback callback)` |  |
| `HWND GetHWND() const` |  |
| `int GetWidth() const` |  |
| `int GetHeight() const` |  |
| `void SetTopmost(bool enable)` | 動的に最前面表示を切り替える関数 |
| `bool IsTopmost() const` |  |
| `void SetUpdateMode(UpdateMode mode)` | ウィンドウ更新モードを設定する関数 |
| `void InvokeCallback(UINT msg, WPARAM wParam, LPARAM lParam)` | フックからコールバックを呼び出すための公開メソッド |
| `void SetClickThroughEnabled(bool enabled)` | クリック透過（WS_EX_TRANSPARENT）を動的に切り替える関数 |
| `void UpdateClickThroughFromHitTest(int screenX, int screenY)` | フックから呼ばれ、登録済みの HitTestCallback を使って クリック透過の有効/無効を更新する関数 |
| `void EnqueueInput(UINT msg, WPARAM wParam, LPARAM lParam)` | 低レベルフックから呼ばれる、入力をキューへ積むだけの関数 |
| `void SetFullscreen(bool enable)` | 全画面表示の切り替え |
| `void SetCursorVisible(bool visible)` | OSカーソルの表示/非表示を切り替える関数 |
| `bool IsFocused() const` | ウィンドウがフォアグラウンド（フォーカスされている）かを取得する関数 |
| `void CenterCursor()` | OSカーソルをクライアント領域の中央へ移動する関数 |

### Tsukino::ECS::EntityRef

`Tsukino.Core/include/Tsukino/Core/ECS/EntityRef/EntityRef.hpp`

**公開メンバ**

| メンバ | 説明 |
|---|---|
| `Entity entity` |  |
| `std::string localName` |  |

**公開関数**

| シグネチャ | 説明 |
|---|---|
| `EntityRef()=default` |  |
| `EntityRef(Entity e)` |  |
| `EntityRef & operator=(Entity e)` |  |
| `EntityRef & operator=(entt::null_t)` |  |
| `operator Entity() const` |  |
| `bool operator==(const EntityRef &other) const` |  |
| `bool operator!=(const EntityRef &other) const` |  |
| `bool operator==(Entity other) const` |  |
| `bool operator!=(Entity other) const` |  |

### Tsukino::ECS::EventBus

`Tsukino.Core/include/Tsukino/Core/ECS/Event/EventBus.hpp`

**公開関数**

| シグネチャ | 説明 |
|---|---|
| `EventBus()=default` | デフォルトコンストラクタ |
| `~EventBus()` | デストラクタ |
| `EventBus(const EventBus &)=delete` |  |
| `EventBus & operator=(const EventBus &)=delete` |  |
| `EventBus(EventBus &&)=delete` |  |
| `EventBus & operator=(EventBus &&)=delete` |  |
| `template <typename TEvent> ScopedConnection Subscribe(std::function< void(const TEvent &)> handler)` | イベントの購読 |
| `template <typename TEvent> void Publish(const TEvent &event)` | イベントの発火（即時・同期） |
| `template <typename TEvent> void Clear()` | 特定イベント型のハンドラを全解除 |
| `void ClearAll()` | 全ハンドラを解除 |

### Tsukino::ECS::ISystem

`Tsukino.Core/include/Tsukino/Core/ECS/System/ISystem.hpp`

**公開関数**

| シグネチャ | 説明 |
|---|---|
| `~ISystem()=default` | 仮想デストラクタ |
| `void Update(Tsukino::ECS::Registry &registry, float deltaTime)=0` | システムの更新 |

### Tsukino::ECS::Registry

`Tsukino.Core/include/Tsukino/Core/ECS/Registry/Registry.hpp`

**公開関数**

| シグネチャ | 説明 |
|---|---|
| `Entity CreateEntity()` | エンティティの作成 |
| `void DestroyEntity(Entity entity)` | エンティティの破棄（即時） |
| `void QueueDestroy(Entity entity)` | エンティティの破棄を予約する |
| `void FlushDestroyQueue()` | 破棄予約されたエンティティをまとめて破棄する |
| `void Clear()` | 全エンティティ・全コンポーネントを破棄する |
| `template <typename T> auto OnConstruct()` | コンポーネント追加シグナルの取得 |
| `template <typename T> auto OnDestroy()` | コンポーネント破棄シグナルの取得 |
| `template <typename T, typename... Args> T & AddComponent(Entity entity, Args &&... args)` | コンポーネントの追加 |
| `template <typename T> T & GetComponent(Entity entity)` | コンポーネントの取得 |
| `template <typename T> bool HasComponent(Entity entity) const` | コンポーネントの存在確認 |
| `template <typename T> T * try_get(Entity entity)` | コンポーネントのポインタ取得（存在しない場合はnullptr） |
| `template <typename T> void RemoveComponent(Entity entity)` | コンポーネントの削除 |
| `template <typename... Components> auto View()` | 特定のコンポーネントのあるエンティティの列挙 |
| `template <typename T, typename... Args> T & SetContext(Args &&... args)` | コンテキスト変数の設定（グローバルデータの登録） |
| `template <typename T> T & GetContext()` | コンテキスト変数の取得 |
| `template <typename T> bool HasContext() const` | コンテキスト変数の存在確認 |
| `template <typename T> void RemoveContext()` | コンテキスト変数の削除 |
| `bool IsValid(Entity entity) const` | エンティティが有効（生存）しているか確認 |

### Tsukino::Input::InputSystem

`Tsukino.Core/include/Tsukino/Core/Input/InputSystem.hpp`

**公開関数**

| シグネチャ | 説明 |
|---|---|
| `InputSystem()` | コンストラクタ |
| `~InputSystem()=default` | デストラクタ |
| `InputSystem(const InputSystem &)=delete` |  |
| `InputSystem & operator=(const InputSystem &)=delete` |  |
| `void Update()` | 更新関数 |
| `bool IsKeyDown(KeyCode code) const` | ボタンが押されているか（押しっぱなし） |
| `bool IsKeyPressed(KeyCode code) const` | ボタンが押された瞬間か |
| `bool AnyKeyPressed() const` | どのキーでも押された瞬間か |
| `bool IsKeyReleased(KeyCode code) const` | ボタンが離された瞬間か |
| `void GetMousePosition(i32 *x, i32 *y) const` | マウスのスクリーン座標を取得 |
| `void GetMouseDelta(i32 *dx, i32 *dy) const` | 前フレームからのマウス移動量を取得 |
| `float GetWheelDelta() const` |  |
| `void SetKeyState(KeyCode code, bool isDown)` | キー状態を直接書き換える |
| `void SetMousePosition(i32 x, i32 y)` | マウスの座標を直接書き換える |
| `void AddWheelDelta(float delta)` | ホイールの回転量を加算する |
| `void ClearAllKeys()` | 全てのキー・ボタンの押下状態をクリアする |

### Tsukino::Asset::AssetHandle

`Tsukino.Engine/include/Tsukino/Engine/Asset/AssetHandle.hpp`

**公開関数**

| シグネチャ | 説明 |
|---|---|
| `AssetHandle()=default` | デフォルトコンストラクタ |
| `AssetHandle(u64 value)` | 引数付きコンストラクタ |
| `u64 Value() const` | 値を取得する関数 |
| `bool IsValid() const` | ハンドルが有効か確認する関数 |
| `bool operator==(const AssetHandle &other) const` | 同一のハンドルか比較する演算子オーバーロード |
| `bool operator!=(const AssetHandle &other) const` | 異なるハンドルか比較する演算子オーバーロード |
| `AssetHandle Invalid()` | 無効なハンドルを取得する関数 |

### Tsukino::Asset::AssetManager

`Tsukino.Engine/include/Tsukino/Engine/Asset/AssetManager.hpp`

**公開関数**

| シグネチャ | 説明 |
|---|---|
| `AssetManager()=default` |  |
| `~AssetManager()` | デストラクタ |
| `void Initialize()` | AssetManagerを初期化する関数 |
| `AssetHandle Load(const Tsukino::Core::Path &path)` | アセットをロードする関数 |
| `Tsukino::Core::Ref< IAsset > Get(AssetHandle handle)` | 任意のアセットハンドルからアセットを取得する関数 |
| `bool Exists(AssetHandle handle)` | アセットハンドルが存在するか確認する関数 |
| `void RegisterImporter(AssetType type, Tsukino::Core::Ref< IAssetImporter > importer)` | インポーターを登録する関数 |
| `void RegisterAsset(AssetHandle handle, Tsukino::Core::Ref< IAsset > asset)` | アセットを登録する関数 |

### Tsukino::Asset::AssetRef

`Tsukino.Engine/include/Tsukino/Engine/Asset/AssetRef.hpp`

**公開メンバ**

| メンバ | 説明 |
|---|---|
| `AssetHandle handle` |  |
| `std::string path` |  |

**公開関数**

| シグネチャ | 説明 |
|---|---|
| `AssetRef()=default` |  |
| `AssetRef(AssetHandle h)` |  |
| `AssetRef & operator=(AssetHandle h)` |  |
| `operator AssetHandle() const` |  |
| `bool IsValid() const` |  |
| `u64 Value() const` |  |
| `bool operator==(const AssetRef &other) const` |  |
| `bool operator!=(const AssetRef &other) const` |  |
| `bool operator==(AssetHandle other) const` |  |
| `bool operator!=(AssetHandle other) const` |  |

### Tsukino::ECS::Scene

`Tsukino.Engine/include/Tsukino/Engine/ECS/Scene.hpp`

**公開関数**

| シグネチャ | 説明 |
|---|---|
| `Scene()=default` | デフォルトコンストラクタ |
| `~Scene()=default` | デストラクタ |
| `void Initialize()` | シーン初期化 |
| `void Update(float deltaTime)` | 毎フレームの更新 |
| `void Shutdown()` | シーンの終了処理 |
| `Entity CreateEntity()` | エンティティの生成 |
| `void DestroyEntity(Entity entity)` | エンティティの破棄 |
| `Registry & GetRegistry()` |  |
| `void AddSystem(std::shared_ptr< ISystem > system, int priority=0)` | システムの追加 |
| `EventBus & GetEventBus()` | EventBus へのアクセス |

### Tsukino::Engine::ECS::Prefab::PrefabFactory

`Tsukino.Engine/include/Tsukino/Engine/ECS/Prefab/PrefabFactory.hpp`

**公開関数**

| シグネチャ | 説明 |
|---|---|
| `PrefabFactory()=default` | デフォルトコンストラクタ |
| `~PrefabFactory()=default` | デストラクタ |
| `PrefabFactory(const PrefabFactory &)=delete` |  |
| `PrefabFactory & operator=(const PrefabFactory &)=delete` |  |
| `PrefabFactory(PrefabFactory &&)=default` |  |
| `PrefabFactory & operator=(PrefabFactory &&)=default` |  |
| `template <typename ComponentType> void RegisterComponent(const std::string &typeName)` | コンポーネントの型名とロード処理を工場に登録する |
| `void SetAssetManager(Tsukino::Asset::AssetManager *assetManager)` | AssetRefフィールドの解決に使うAssetManagerを設定する |
| `template <typename T> void ApplyOverride(Tsukino::ECS::Registry &registry, entt::entity entity, const T &overrideValue)` | 生成済みエンティティの指定コンポーネントを、任意の値で上書きする |
| `entt::entity Instantiate(const std::string &prefabJsonPath, Tsukino::ECS::Registry &registry)` | PrefabのJSON（目次）から動的にコンポーネントを組み立ててエンティティを生成する |
| `PrefabInstance InstantiateGroup(const std::vector< GroupEntry > &entries, Tsukino::ECS::Registry &registry)` | 複数のPrefabを名前付きでまとめて生成し、コンポーネント内のEntityRefフィールドを バッチ内の他エンティティへ解決する |
| `PrefabInstance InstantiateGroup(const std::string &groupJsonPath, Tsukino::ECS::Registry &registry)` | グループ定義JSON（"Entities": {名前: Prefabパス} のマップ）からバッチ生成を行う |
| `bool CaptureEntity(Tsukino::ECS::Registry &registry, entt::entity entity, const std::string &outDir)` | 生成済みエンティティが持つ、セーブ対応済みコンポーネントをすべてJSONへ書き出し、 それらをまとめるPrefab（目次）JSONも生成する（開発者向けブートストラップ用途） |
| `template <typename T> bool Load(const std::string &jsonPath, const std::string &keyName, T &outData)` | 任意のComponent（T）をJSONファイルからロードする（個別用） |
| `template <typename T> bool Save(const std::string &jsonPath, const std::string &keyName, const T &desc)` | 任意のComponent（T）をJSONファイルにセーブする（個別用） |

### Tsukino::EngineIntegration::EngineAPI

`Tsukino.EngineIntegration/include/Tsukino/EngineIntegration/EngineAPI.hpp`

**公開関数**

| シグネチャ | 説明 |
|---|---|
| `EngineAPI(EngineContext &context)` | コンストラクタ |
| `void ChangeScene(std::unique_ptr< GameSceneBase > newScene)` | シーン遷移関数 |
| `bool ProcessMessages()` | メッセージ処理関数 |
| `void Update(float deltaTime)` |  |
| `void Render()` | 描画関数 |

### Tsukino::EngineIntegration::EngineContext

`Tsukino.EngineIntegration/include/Tsukino/EngineIntegration/EngineContext.hpp`

**公開メンバ**

| メンバ | 説明 |
|---|---|
| `Tsukino::Renderer::Renderer * renderer` |  |
| `Tsukino::Core::Window * window` |  |
| `Tsukino::Input::InputSystem * inputSystem` |  |
| `Tsukino::Asset::AssetManager * assetManager` |  |
| `Tsukino::BuiltIn::BuiltInAssets * builtinAssets` |  |
| `GameSceneManager * gameSceneManager` |  |
| `Tsukino::Audio::AudioManager * audioManager` |  |
| `Tsukino::Engine::ECS::Prefab::PrefabFactory * prefabFactory` |  |
| `Tsukino::BuiltIn::ECS::EffectSystem * effectSystem` |  |
| `Tsukino::BuiltIn::ECS::PhysicsSystem * physicsSystem` |  |

### Tsukino::EngineIntegration::EngineIntegration

`Tsukino.EngineIntegration/include/Tsukino/EngineIntegration/EngineIntegration.hpp`

**公開関数**

| シグネチャ | 説明 |
|---|---|
| `EngineIntegration()` | コンストラクタ |
| `~EngineIntegration()` | デストラクタ |
| `EngineIntegration(const EngineIntegration &)=delete` |  |
| `EngineIntegration & operator=(const EngineIntegration &)=delete` |  |
| `EngineIntegration(EngineIntegration &&)=delete` |  |
| `EngineIntegration & operator=(EngineIntegration &&)=delete` |  |
| `bool Initialize(int width, int height, const std::string &title="TsukinoEngine", Tsukino::Core::Window::WindowStyle style=Tsukino::Core::Window::WindowStyle::Default)` | エンジンの初期化関数 |
| `EngineContext & GetContext()` | エンジン全体で共有されるクラスを取得する関数 |

### Tsukino::EngineIntegration::GameSceneBase

`Tsukino.EngineIntegration/include/Tsukino/EngineIntegration/Scene/GameSceneBase.hpp`

**公開関数**

| シグネチャ | 説明 |
|---|---|
| `~GameSceneBase()=default` | デフォルトコンストラクタ |
| `void Initialize(Tsukino::EngineIntegration::EngineAPI &api, Tsukino::EngineIntegration::EngineContext *context)` | シーンの初期化インターフェース（外部から呼ばれる非仮想関数） |
| `void OnUpdate(Tsukino::EngineIntegration::EngineAPI &api, float deltaTime)=0` | シーンの更新インターフェース |
| `void OnExit()=0` | シーンの終了インターフェース |
| `Tsukino::ECS::Scene & GetScene()` | シーンへのアクセス |

### Tsukino::EngineIntegration::GameSceneManager

`Tsukino.EngineIntegration/include/Tsukino/EngineIntegration/Scene/GameSceneManager.hpp`

**公開関数**

| シグネチャ | 説明 |
|---|---|
| `GameSceneManager()` | コンストラクタ |
| `~GameSceneManager()` | デストラクタ |
| `GameSceneManager(const GameSceneManager &)=delete` |  |
| `GameSceneManager & operator=(const GameSceneManager &)=delete` |  |
| `GameSceneManager(GameSceneManager &&)=delete` |  |
| `GameSceneManager & operator=(GameSceneManager &&)=delete` |  |
| `void Initialize(EngineContext *context)` | 初期化 |
| `void ChangeScene(std::unique_ptr< GameSceneBase > newScene)` | 次のシーンへの遷移を予約する |
| `void Update(Tsukino::EngineIntegration::EngineAPI &api, float deltaTime)` | シーンの更新 |
| `GameSceneBase * GetCurrentScene() const` | 現在アクティブなシーンを取得する |

### Tsukino::Renderer::Renderer

`Tsukino.Renderer/include/Tsukino/Renderer/Renderer.hpp`

**公開関数**

| シグネチャ | 説明 |
|---|---|
| `Renderer()=default` | コンストラクタ |
| `~Renderer()=default` | デストラクタ |
| `bool Initialize(HWND hwnd, uint32_t width, uint32_t height, const RendererShaderSet &shaders)` | レンダラーの初期化 |
| `void Render(class Tsukino::BuiltIn::ECS::EffectSystem *effectSystem=nullptr)` | 描画処理 |
| `void Resize(uint32_t width, uint32_t height)` | 描画領域のリサイズ |
| `void SetClearColor(float r, float g, float b, float a)` | クリアカラー設定 |
| `void PushDrawCommand(const DrawCommand &cmd)` | 描画コマンドの追加 |
| `const FrameStats & GetFrameStats() const` | 直前のフレームの描画統計を取得する関数 |
| `void SetVSyncEnabled(bool enabled)` | 垂直同期の有無を設定する関数 |
| `bool IsVSyncEnabled() const` | 垂直同期が有効かを取得する関数 |
| `void DrawDebugLine(const Tsukino::GraphicsCommon::DebugVertex &v1, const Tsukino::GraphicsCommon::DebugVertex &v2)` | デバッグラインの追加 |
| `void DrawDebugTriangle(const Tsukino::GraphicsCommon::DebugVertex &v1, const Tsukino::GraphicsCommon::DebugVertex &v2, const Tsukino::GraphicsCommon::DebugVertex &v3)` | デバッグ三角形の追加 |
| `void FlushDebugDraw()` | デバッグ描画の実行 |
| `PipelineFactory * GetPipelineFactory()` |  |
| `ID3D11Device * GetDevice() const` |  |
| `ID3D11DeviceContext * GetContext() const` |  |
| `MeshBuffer * GetPrimitiveMesh(Tsukino::GraphicsCommon::PrimitiveType type)` |  |
| `ID3D11SamplerState * GetSampler(Tsukino::GraphicsCommon::SamplerType type) const` |  |
| `ID3D11ShaderResourceView * GetTextureSRV(const Tsukino::Asset::TextureAsset &textureAsset)` | テクスチャ（SRV）の取得（なければ生成してキャッシュ） |
| `void UpdateSceneBuffer(const CBufferScene &sceneData)` | シーン定数バッファの更新 |
| `std::unique_ptr< DirectX::SpriteFont > CreateSpriteFont(const u8 *data, size_t size)` | SpriteFontの作成 |
| `void SetWorldCameraMatrix(const CBufferScene &data)` | ワールドカメラ行列のセット |
| `void SetOverlayCameraMatrix(const CBufferScene &data)` | オーバーレイカメラ行列のセット |
| `std::unique_ptr< DirectX::SpriteBatch > CreateSpriteBatch()` | SpriteBatchの作成 |
| `DirectX::CommonStates * GetCommonStatesTK() const` | 共通ステートの取得 |
| `void SetDirectionalLight(const hlslpp::float3 &direction, const hlslpp::float3 &color, float intensity)` | ディレクショナルライトの設定 |
| `void SetShadowPipeline(std::shared_ptr< PipelineState > staticPipeline, std::shared_ptr< PipelineState > skeletalPipeline)` | シャドウパイプラインのセット |
| `ID3D11ShaderResourceView * GetWhiteTextureSRV()` | 白テクスチャのSRVを取得 |
| `ID3D11ShaderResourceView * GetFlatNormalTextureSRV()` | フラット法線テクスチャのSRVを取得 |
| `void SetSkyParameters(const CBufferSky &sky)` | 大気散乱パラメータのセット |
| `void SetSkyPipeline(const Tsukino::Asset::ShaderAsset *vs, const Tsukino::Asset::ShaderAsset *ps)` | スカイパイプラインのセット |
| `void UpdateWaterTime(float deltaTime)` | 水面の時間経過を更新（波のアニメーションなどに使用） |
| `void SetWaterParameters(const CBufferWater &water)` | 水面パラメータのセット |
| `void SetWaterPipeline(const Tsukino::Asset::ShaderAsset *vs, const Tsukino::Asset::ShaderAsset *ps)` | 水面パイプラインのセット |
| `void SetLights(const GPULight *lights, u32 count)` | 点光源・スポットライト配列のセット（ディファードLightingパス用） |
| `bool SetMotionBlurPipeline(const Tsukino::Asset::ShaderAsset *ps)` | モーションブラーパイプラインのセット |
| `void SetMotionBlurParameters(const CBufferMotionBlur &params)` | モーションブラーパラメータのセット |
| `void SetMotionBlurEnabled(bool enabled) noexcept` | モーションブラーの有効・無効を切り替える |
| `void SetFogParameters(const CBufferFog &params)` | フォグパラメータのセット |
| `void SetFogEnabled(bool enabled) noexcept` | フォグの有効・無効を切り替える |

## 全公開型の索引

メンバ名のみ。詳細が要るときはヘッダを開く。


### Tsukino.Audio

- **Tsukino::Audio::AudioContext** — `Tsukino.Audio/src/Audio/AudioManager.cpp`
  - engine, waveBanks, Initialize(), Update(), GetOrLoadWaveBank()
- **Tsukino::Audio::AudioManager** — `Tsukino.Audio/include/Tsukino/Audio/AudioManager.hpp`
  - AudioManager(), ~AudioManager(), AudioManager(), operator=(), AudioManager(), operator=(), Initialize(), Update(), Play(), Stop(), StopAll(), IsPlaying(), SetMasterVolume(), GetMasterVolume()

### Tsukino.BuiltIn

- **Tsukino::BuiltIn::BuiltInAssets** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/BuiltInAssets.hpp`
  - shaders, fonts, Initialize()
- **Tsukino::BuiltIn::BuiltInFonts** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/BuiltInFonts.hpp`
  - defaultFont, Initialize()
- **Tsukino::BuiltIn::BuiltInShaders** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/BuiltInShaders.hpp`
  - spriteVS, spritePS, spriteWorldVS, modelVS, modelPS, staticModelVS, debugVS, debugPS, shadowVS, shadowStaticVS, shadowPS, skyVS, skyPS, tonemapVS, tonemapPS, waterPS, gbufferPS, lightingPS, motionBlurPS, fogPS, Initialize()
- **Tsukino::BuiltIn::ECS::AnimationControllerComponent** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/AnimationControllerComponent.hpp`
  - blend_alpha, is_transitioning, next, outgoing
- **Tsukino::BuiltIn::ECS::AnimationControllerComponent::NextAnimation** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/AnimationControllerComponent.hpp`
  - clip, animation_index, fade_time, immediate, is_looping, clip_start_time, clip_end_time, in_place
- **Tsukino::BuiltIn::ECS::AnimationControllerComponent::OutgoingAnimation** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/AnimationControllerComponent.hpp`
  - clip, animation_index, elapsed_time, is_looping, clip_start_time, clip_end_time, in_place
- **Tsukino::BuiltIn::ECS::AnimationPlayerComponent** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/AnimationPlayerComponent.hpp`
  - current_clip_id, animation_index, elapsed_time, playback_speed, is_looping, is_playing, is_finished, clip_start_time, clip_end_time, in_place, root_motion_node_name, root_motion_node_index, root_motion_resolved, root_motion_lock_active, root_motion_lock_x, root_motion_lock_z
- **Tsukino::BuiltIn::ECS::BoxCollider2DComponent** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/BoxCollider2DComponent.hpp`
  - offset, size
- **Tsukino::BuiltIn::ECS::CameraComponent** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/CameraComponent.hpp`
  - projectionType, orthoSize, fov, aspectRatio, nearZ, farZ, useLookAt, lookAtTarget, viewMatrix, projectionMatrix, viewProjMatrix, invViewProjMatrix, isPrimary, dirty
- **Tsukino::BuiltIn::ECS::CharacterControllerComponent** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/CharacterControllerComponent.hpp`
  - radius, halfHeight, maxSlopeDeg, mass, gravityFactor, centerOffset, isInitialized, moveInput, jumpRequested, jumpSpeed, verticalVelocity, isGrounded
- **Tsukino::BuiltIn::ECS::CollisionComponent** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/CollisionComponent.hpp`
  - bodyID, type, extent, offsetPosition, offsetRotation, heightfieldSamples, heightfieldSize, heightfieldOffset, heightfieldScale, isSensor, isInitialized, onCollisionEnter, IsValid()
- **Tsukino::BuiltIn::ECS::CollisionEnterEvent** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Event/CollisionEnterEvent.hpp`
  - self, other, normal
- **Tsukino::BuiltIn::ECS::DebugCameraComponent** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/DebugCameraComponent.hpp`
  - moveSpeed, sprintSpeed, mouseSens, yaw, pitch, isActive
- **Tsukino::BuiltIn::ECS::DebugCameraTag** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/DebugCameraTag.hpp`
  - dummy
- **Tsukino::BuiltIn::ECS::DirectionalLightComponent** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/DirectionalLightComponent.hpp`
  - direction, color, intensity, castShadow
- **Tsukino::BuiltIn::ECS::DraggableComponent** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/DraggableComponent.hpp`
  - isDragging, dragOffset
- **Tsukino::BuiltIn::ECS::EffectComponent** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/EffectComponent.hpp`
  - effectAsset, effectPath, handle, playSpeed, looping, stopped, active, scale, followRotation
- **Tsukino::BuiltIn::ECS::FogComponent** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/FogComponent.hpp`
  - enabled, color, density, startDistance, maxOpacity, heightFogEnabled, height, heightFalloff, heightDensity, sunColor, sunScatterPower, noiseEnabled, noiseScale, noiseIntensity, windDirection, windSpeed
- **Tsukino::BuiltIn::ECS::FontComponent** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/FontComponent.hpp`
  - text, fontHandle, color, origin, horizontalAlign, verticalAlign, outlineColor, outlineWidth, sortOrder
- **Tsukino::BuiltIn::ECS::HighlightComponent** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/HighlightComponent.hpp`
  - active, rimColor, rimIntensity, rimPower, glow
- **Tsukino::BuiltIn::ECS::ImpulseRequestComponent** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/ImpulseRequestComponent.hpp`
  - impulse, angularImpulse
- **Tsukino::BuiltIn::ECS::ModelComponent** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/ModelComponent.hpp`
  - modelHandle, visible, opacity
- **Tsukino::BuiltIn::ECS::MotionBlurComponent** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/MotionBlurComponent.hpp`
  - enabled, strength, maxBlurRadius, sampleCount, targetFps
- **Tsukino::BuiltIn::ECS::MotionVectorComponent** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/MotionVectorComponent.hpp`
  - MAX_BONES, prevWorld, prevBones, prevBoneCount, valid
- **Tsukino::BuiltIn::ECS::NodeWorldMatrixComponent** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/NodeWorldMatrixComponent.hpp`
  - matrices
- **Tsukino::BuiltIn::ECS::NodeWorldPoseComponent** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/NodeWorldPoseComponent.hpp`
  - poses
- **Tsukino::BuiltIn::ECS::PointLightComponent** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/PointLightComponent.hpp`
  - color, intensity, range, enabled
- **Tsukino::BuiltIn::ECS::RigidbodyComponent** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/RigidbodyComponent.hpp`
  - type, isTypeDirty, mass, friction, restitution, gravityFactor, linearVelocity, angularVelocity, force, torque, isInitialized, isGrounded, groundCheckDistance, groundCheckRadius, freezePositionX, freezePositionY, freezePositionZ, freezeRotationX, freezeRotationY, freezeRotationZ, isFreezeDirty
- **Tsukino::BuiltIn::ECS::RootMotionComponent** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/RootMotionComponent.hpp`
  - delta_position, delta_rotation
- **Tsukino::BuiltIn::ECS::SkeletonOutputComponent** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/SkeletonOutputComponent.hpp`
  - MAX_BONES, local_matrices, bone_count
- **Tsukino::BuiltIn::ECS::SkyAtmosphereComponent** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/SkyAtmosphereComponent.hpp`
  - rayleighScattering, mieScattering, mieAnisotropy, atmosphereHeight, planetRadius, sunIntensity, sunDiskSize, groundColor
- **Tsukino::BuiltIn::ECS::SpotLightComponent** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/SpotLightComponent.hpp`
  - color, intensity, range, innerConeDeg, outerConeDeg, enabled
- **Tsukino::BuiltIn::ECS::SpringBoneComponent** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/SpringBoneComponent.hpp`
  - chainDefs, chains, resolved, enabled
- **Tsukino::BuiltIn::ECS::SpringBoneComponent::ChainDef** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/SpringBoneComponent.hpp`
  - name, anchorNodeName, rootNodeName, excludeNodeNames, maxDepth, settings, colliders
- **Tsukino::BuiltIn::ECS::SpringBoneComponent::ColliderDef** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/SpringBoneComponent.hpp`
  - attachNodeName, localOffset, radius
- **Tsukino::BuiltIn::ECS::SpriteComponent** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/SpriteComponent.hpp`
  - textureHandle, blendMode, space, tintColor, uvRect, sortOrder
- **Tsukino::BuiltIn::ECS::TerrainGenerationRequestComponent** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/TerrainGenerationRequestComponent.hpp`
  - amplitude, noiseFrequency, seed, noiseType, collisionModelHandle
- **Tsukino::BuiltIn::ECS::TransformComponent** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp`
  - position, rotation, scale, localMatrix, worldMatrix, parent, dirty
- **Tsukino::BuiltIn::ECS::WorldAnchorComponent** — `Tsukino.BuiltIn/include/Tsukino/BuiltIn/ECS/Component/WorldAnchorComponent.hpp`
  - target, useFixedWorldPosition, fixedWorldPosition, worldOffset, screenOffset, visible

### Tsukino.Core

- **Tsukino::Core::DebugTools::FrameProfiler** — `Tsukino.Core/include/Tsukino/Core/DebugTools/FrameProfiler.hpp`
  - Get(), NowMilliseconds(), AddSample(), Tick(), GetAverageMs(), BeginSystemReport(), ReportSystemTime(), GetSystemTimings()
- **Tsukino::Core::DebugTools::FrameProfiler::SystemTiming** — `Tsukino.Core/include/Tsukino/Core/DebugTools/FrameProfiler.hpp`
  - name, averageMs
- **Tsukino::Core::DebugTools::ScopedProfileTimer** — `Tsukino.Core/include/Tsukino/Core/DebugTools/FrameProfiler.hpp`
  - ScopedProfileTimer(), ~ScopedProfileTimer(), ScopedProfileTimer(), operator=()
- **Tsukino::Core::Log** — `Tsukino.Core/include/Tsukino/Core/Log.hpp`
  - Info(), Warn(), Error(), SetLogFile(), CloseLogFile()
- **Tsukino::Core::Math::matrix** — `Tsukino.Core/include/Tsukino/Core/Math/Matrix.hpp`
  - identity(), translate(), translate(), scale(), scale(), scale(), rotateX(), rotateY(), rotateZ(), rotateXYZ(), rotateAxis(), lookAtLH(), perspectiveFovLH(), perspectiveFovInfiniteFarPlaneLH(), orthographicOffCenterLH(), fromQuaternion(), rotate(), decomposePositionRotation(), matrix()
- **Tsukino::Core::Path** — `Tsukino.Core/include/Tsukino/Core/Path.hpp`
  - Path(), Path(), string(), parent(), filename(), extension(), stem(), parent_path(), ToWString(), operator/(), operator/(), replace_extension(), SplitPathAndFragment(), ToLower()
- **Tsukino::Core::Window** — `Tsukino.Core/include/Tsukino/Core/Window.hpp`
  - Window(), ~Window(), Create(), ProcessMessages(), SetMessageCallback(), SetResizeCallback(), SetFocusLostCallback(), SetHitTestCallback(), GetHWND(), GetWidth(), GetHeight(), SetTopmost(), IsTopmost(), SetUpdateMode(), InvokeCallback(), SetClickThroughEnabled(), UpdateClickThroughFromHitTest(), EnqueueInput(), SetFullscreen(), SetCursorVisible(), IsFocused(), CenterCursor()
- **Tsukino::Core::Window::QueuedInputEvent** — `Tsukino.Core/include/Tsukino/Core/Window.hpp`
  - msg, wParam, lParam
- **Tsukino::ECS::EntityRef** — `Tsukino.Core/include/Tsukino/Core/ECS/EntityRef/EntityRef.hpp`
  - entity, localName, EntityRef(), EntityRef(), operator=(), operator=(), operator Entity(), operator==(), operator!=(), operator==(), operator!=()
- **Tsukino::ECS::EntityRefResolverArchive** — `Tsukino.Core/include/Tsukino/Core/ECS/EntityRef/EntityRefResolverArchive.hpp`
  - EntityRefResolverArchive(), operator()()
- **Tsukino::ECS::EventBus** — `Tsukino.Core/include/Tsukino/Core/ECS/Event/EventBus.hpp`
  - EventBus(), ~EventBus(), EventBus(), operator=(), EventBus(), operator=(), Subscribe(), Publish(), Clear(), ClearAll()
- **Tsukino::ECS::EventBus::HandlerEntry** — `Tsukino.Core/include/Tsukino/Core/ECS/Event/EventBus.hpp`
  - id, invoke
- **Tsukino::ECS::ISystem** — `Tsukino.Core/include/Tsukino/Core/ECS/System/ISystem.hpp`
  - ~ISystem(), Update()
- **Tsukino::ECS::Registry** — `Tsukino.Core/include/Tsukino/Core/ECS/Registry/Registry.hpp`
  - CreateEntity(), DestroyEntity(), QueueDestroy(), FlushDestroyQueue(), Clear(), OnConstruct(), OnDestroy(), AddComponent(), GetComponent(), HasComponent(), try_get(), RemoveComponent(), View(), SetContext(), GetContext(), HasContext(), RemoveContext(), IsValid()
- **Tsukino::ECS::ScopedConnection** — `Tsukino.Core/include/Tsukino/Core/ECS/Event/ScopedConnection.hpp`
  - ScopedConnection(), ScopedConnection(), ScopedConnection(), operator=(), ScopedConnection(), operator=(), ~ScopedConnection(), Disconnect(), IsConnected()
- **Tsukino::IO::FileSystem** — `Tsukino.Core/include/Tsukino/Core/IO/FileSystem.hpp`
  - ReadBinary(), ReadText(), Exists(), CreateDirectories(), GetLastWriteTime(), GetAssetRootPath(), GetEngineAssetRootPath(), ToEngineRelativePath()
- **Tsukino::Input::InputSystem** — `Tsukino.Core/include/Tsukino/Core/Input/InputSystem.hpp`
  - InputSystem(), ~InputSystem(), InputSystem(), operator=(), Update(), IsKeyDown(), IsKeyPressed(), AnyKeyPressed(), IsKeyReleased(), GetMousePosition(), GetMouseDelta(), GetWheelDelta(), SetKeyState(), SetMousePosition(), AddWheelDelta(), ClearAllKeys()

### Tsukino.Engine

- **Tsukino::Asset::AssetHandle** — `Tsukino.Engine/include/Tsukino/Engine/Asset/AssetHandle.hpp`
  - AssetHandle(), AssetHandle(), Value(), IsValid(), operator==(), operator!=(), Invalid()
- **Tsukino::Asset::AssetHandleGenerator** — `Tsukino.Engine/include/Tsukino/Engine/Asset/Util/AssetHandleGenerator.hpp`
  - Generate()
- **Tsukino::Asset::AssetManager** — `Tsukino.Engine/include/Tsukino/Engine/Asset/AssetManager.hpp`
  - AssetManager(), ~AssetManager(), Initialize(), Load(), Get(), Exists(), RegisterImporter(), RegisterAsset()
- **Tsukino::Asset::AssetRef** — `Tsukino.Engine/include/Tsukino/Engine/Asset/AssetRef.hpp`
  - handle, path, AssetRef(), AssetRef(), operator=(), operator AssetHandle(), IsValid(), Value(), operator==(), operator!=(), operator==(), operator!=()
- **Tsukino::Asset::AssetRefResolverArchive** — `Tsukino.Engine/include/Tsukino/Engine/Asset/AssetRefResolverArchive.hpp`
  - AssetRefResolverArchive(), operator()()
- **Tsukino::Asset::AudioAsset** — `Tsukino.Engine/include/Tsukino/Engine/Asset/Audio/AudioAsset.hpp`
  - metadata, waveBankPath, waveIndex, GetHandle(), SetHandle(), GetType()
- **Tsukino::Asset::AudioImporter** — `Tsukino.Engine/include/Tsukino/Engine/Asset/Audio/AudioImporter.hpp`
  - Import()
- **Tsukino::Asset::AudioLoader** — `Tsukino.Engine/include/Tsukino/Engine/Asset/Audio/AudioLoader.hpp`
  - AudioLoader(), CanLoad(), Load()
- **Tsukino::Asset::CubemapAsset** — `Tsukino.Engine/include/Tsukino/Engine/Asset/Cubemap/CubemapAsset.hpp`
  - ddsData, width, height, GetHandle(), GetType(), SetHandle()
- **Tsukino::Asset::CubemapDesc** — `Tsukino.Engine/include/Tsukino/Engine/Asset/Cubemap/CubemapDesc.hpp`
  - px, nx, py, ny, pz, nz, serialize()
- **Tsukino::Asset::CubemapImporter** — `Tsukino.Engine/include/Tsukino/Engine/Asset/Cubemap/CubemapImporter.hpp`
  - Import()
- **Tsukino::Asset::CubemapLoader** — `Tsukino.Engine/include/Tsukino/Engine/Asset/Cubemap/CubemapLoader.hpp`
  - CubemapLoader(), CanLoad(), Load()
- **Tsukino::Asset::DynamicFontAsset** — `Tsukino.Engine/include/Tsukino/Engine/Asset/Font/DynamicFontAsset.hpp`
  - m_faceName, m_pixelSize, m_fontFileData, GetHandle(), GetType(), SetHandle()
- **Tsukino::Asset::DynamicFontImporter** — `Tsukino.Engine/include/Tsukino/Engine/Asset/Font/DynamicFontImporter.hpp`
  - Import()
- **Tsukino::Asset::DynamicFontLoader** — `Tsukino.Engine/include/Tsukino/Engine/Asset/Font/DynamicFontLoader.hpp`
  - DynamicFontLoader(), CanLoad(), Load()
- **Tsukino::Asset::EffectAsset** — `Tsukino.Engine/include/Tsukino/Engine/Asset/Effect/EffectAsset.hpp`
  - binary, GetHandle(), GetType(), SetHandle()
- **Tsukino::Asset::EffectImporter** — `Tsukino.Engine/include/Tsukino/Engine/Asset/Effect/EffectImporter.hpp`
  - EffectImporter(), Import()
- **Tsukino::Asset::EffectLoader** — `Tsukino.Engine/include/Tsukino/Engine/Asset/Effect/EffectLoader.hpp`
  - EffectLoader(), CanLoad(), Load()
- **Tsukino::Asset::FontAsset** — `Tsukino.Engine/include/Tsukino/Engine/Asset/Font/FontAsset.hpp`
  - m_binaryData, GetHandle(), GetType(), SetHandle()
- **Tsukino::Asset::FontImporter** — `Tsukino.Engine/include/Tsukino/Engine/Asset/Font/FontImporter.hpp`
  - Import()
- **Tsukino::Asset::FontLoader** — `Tsukino.Engine/include/Tsukino/Engine/Asset/Font/FontLoader.hpp`
  - FontLoader(), CanLoad(), Load()
- **Tsukino::Asset::IAsset** — `Tsukino.Engine/include/Tsukino/Engine/Asset/IAsset.hpp`
  - ~IAsset(), GetHandle(), SetHandle(), GetType()
- **Tsukino::Asset::IAssetImporter** — `Tsukino.Engine/include/Tsukino/Engine/Asset/IAssetImporter.hpp`
  - ~IAssetImporter(), Import()
- **Tsukino::Asset::IAssetLoader** — `Tsukino.Engine/include/Tsukino/Engine/Asset/IAssetLoader.hpp`
  - ~IAssetLoader(), CanLoad(), Load()
- **Tsukino::Asset::MaterialAsset** — `Tsukino.Engine/include/Tsukino/Engine/Asset/Material/MaterialAsset.hpp`
  - data, albedoHandle, normalHandle, metallicRoughnessHandle, emissiveHandle, aoHandle, vertexShaderHandle, pixelShaderHandle, GetHandle(), GetType(), SetHandle()
- **Tsukino::Asset::MeshAsset** — `Tsukino.Engine/include/Tsukino/Engine/Asset/Mesh/MeshAsset.hpp`
  - data, MeshAsset(), GetHandle(), GetType(), SetHandle()
- **Tsukino::Asset::ModelAsset** — `Tsukino.Engine/include/Tsukino/Engine/Asset/Model/ModelAsset.hpp`
  - modelData, materialHandles, ModelAsset(), GetHandle(), GetType(), SetHandle()
- **Tsukino::Asset::ModelImporter** — `Tsukino.Engine/include/Tsukino/Engine/Asset/Model/ModelImporter.hpp`
  - Import()
- **Tsukino::Asset::ModelLoader** — `Tsukino.Engine/include/Tsukino/Engine/Asset/Model/ModelLoader.hpp`
  - ModelLoader(), CanLoad(), Load()
- **Tsukino::Asset::ShaderAsset** — `Tsukino.Engine/include/Tsukino/Engine/Asset/Shader/ShaderAsset.hpp`
  - shaderStage, entryPoint, profile, source, binary, filePath, ShaderAsset(), GetHandle(), GetType(), SetHandle()
- **Tsukino::Asset::ShaderImporter** — `Tsukino.Engine/include/Tsukino/Engine/Asset/Shader/ShaderImporter.hpp`
  - Import()
- **Tsukino::Asset::ShaderLoader** — `Tsukino.Engine/include/Tsukino/Engine/Asset/Shader/ShaderLoader.hpp`
  - ShaderLoader(), CanLoad(), Load(), DetectStage()
- **Tsukino::Asset::TextureAsset** — `Tsukino.Engine/include/Tsukino/Engine/Asset/Texture/TextureAsset.hpp`
  - width, height, format, pixels, GetHandle(), GetType(), SetHandle()
- **Tsukino::Asset::TextureImporter** — `Tsukino.Engine/include/Tsukino/Engine/Asset/Texture/TextureImporter.hpp`
  - Import()
- **Tsukino::Asset::TextureLoader** — `Tsukino.Engine/include/Tsukino/Engine/Asset/Texture/TextureLoder.hpp`
  - TextureLoader(), CanLoad(), Load()
- **Tsukino::Asset::Vertex** — `Tsukino.Engine/src/Asset/Model/ModelImporter.cpp`
  - position, normal, texcoord
- **Tsukino::Asset::XWBEntry** — `Tsukino.Engine/include/Tsukino/Engine/Asset/Audio/AudioAsset.hpp`
  - offset, length, sampleRate, channels, formatTag
- **Tsukino::ECS::EngineEvent::EntityCreatedEvent** — `Tsukino.Engine/include/Tsukino/Engine/ECS/EngineEvent/EntityEvent.hpp`
  - entity
- **Tsukino::ECS::EngineEvent::EntityDestroyedEvent** — `Tsukino.Engine/include/Tsukino/Engine/ECS/EngineEvent/EntityEvent.hpp`
  - entity
- **Tsukino::ECS::EngineEvent::SceneChangeRequestedEvent** — `Tsukino.Engine/include/Tsukino/Engine/ECS/EngineEvent/SceneEvent.hpp`
  - nextSceneName
- **Tsukino::ECS::EngineEvent::SceneInitializedEvent** — `Tsukino.Engine/include/Tsukino/Engine/ECS/EngineEvent/SceneEvent.hpp`
  - （公開メンバなし）
- **Tsukino::ECS::Scene** — `Tsukino.Engine/include/Tsukino/Engine/ECS/Scene.hpp`
  - Scene(), ~Scene(), Initialize(), Update(), Shutdown(), CreateEntity(), DestroyEntity(), GetRegistry(), AddSystem(), GetEventBus()
- **Tsukino::ECS::SystemManager** — `Tsukino.Engine/include/Tsukino/Engine/ECS/SystemManager.hpp`
  - SystemManager(), ~SystemManager(), AddSystem(), Update(), Clear(), GetProfiles()
- **Tsukino::ECS::SystemManager::SystemEntry** — `Tsukino.Engine/include/Tsukino/Engine/ECS/SystemManager.hpp`
  - system, priority, name, accumulatedMs
- **Tsukino::ECS::SystemManager::SystemProfile** — `Tsukino.Engine/include/Tsukino/Engine/ECS/SystemManager.hpp`
  - name, averageMs
- **Tsukino::Engine::ECS::Prefab::PrefabFactory** — `Tsukino.Engine/include/Tsukino/Engine/ECS/Prefab/PrefabFactory.hpp`
  - PrefabFactory(), ~PrefabFactory(), PrefabFactory(), operator=(), PrefabFactory(), operator=(), RegisterComponent(), SetAssetManager(), ApplyOverride(), Instantiate(), InstantiateGroup(), InstantiateGroup(), CaptureEntity(), Load(), Save()
- **Tsukino::Engine::ECS::Prefab::PrefabFactory::GroupEntry** — `Tsukino.Engine/include/Tsukino/Engine/ECS/Prefab/PrefabFactory.hpp`
  - name, prefabPath
- **Tsukino::Physics::SpringBoneChain** — `Tsukino.Engine/include/Tsukino/Engine/Physics/SpringBone/SpringBoneData.hpp`
  - name, anchorNodeIndex, settings, nodes, colliders, previousAnchorPosition, anchorInitialized
- **Tsukino::Physics::SpringBoneNode** — `Tsukino.Engine/include/Tsukino/Engine/Physics/SpringBone/SpringBoneData.hpp`
  - nodeIndex, parentIndexInChain, restLength, currentPosition, previousPosition, correctedRotation, initialized
- **Tsukino::Physics::SpringBoneSettings** — `Tsukino.Engine/include/Tsukino/Engine/Physics/SpringBone/SpringBoneData.hpp`
  - stiffness, drag, inertia, gravityScale, gravityDir, boneRadius, angleLimitDeg, collisionIterations, serialize()
- **Tsukino::Physics::SpringColliderSphere** — `Tsukino.Engine/include/Tsukino/Engine/Physics/SpringBone/SpringBoneData.hpp`
  - attachNodeIndex, localOffset, radius, serialize()
- **Tsukino::Physics::WorldPose** — `Tsukino.Engine/include/Tsukino/Engine/Physics/SpringBone/SpringBoneData.hpp`
  - position, rotation

### Tsukino.EngineIntegration

- **Tsukino::BuiltIn::ECS::AnimationSystem** — `Tsukino.EngineIntegration/include/Tsukino/EngineIntegration/ECS/System/AnimationSystem.hpp`
  - Update()
- **Tsukino::BuiltIn::ECS::AnimationSystem::ChannelTableKey** — `Tsukino.EngineIntegration/include/Tsukino/EngineIntegration/ECS/System/AnimationSystem.hpp`
  - animation, skeleton, operator==()
- **Tsukino::BuiltIn::ECS::AnimationSystem::ChannelTableKeyHash** — `Tsukino.EngineIntegration/include/Tsukino/EngineIntegration/ECS/System/AnimationSystem.hpp`
  - operator()()
- **Tsukino::BuiltIn::ECS::BPLayerInterfaceImpl** — `Tsukino.EngineIntegration/src/ECS/System/PhysicsSystem.cpp`
  - BPLayerInterfaceImpl(), GetNumBroadPhaseLayers(), GetBroadPhaseLayer()
- **Tsukino::BuiltIn::ECS::CameraSystem** — `Tsukino.EngineIntegration/include/Tsukino/EngineIntegration/ECS/System/CameraSystem.hpp`
  - Update()
- **Tsukino::BuiltIn::ECS::CharacterContactListenerImpl** — `Tsukino.EngineIntegration/src/ECS/System/PhysicsSystem.cpp`
  - physicsSystem, OnContactAdded()
- **Tsukino::BuiltIn::ECS::ClipRangeTicks** — `Tsukino.EngineIntegration/src/ECS/System/AnimationSystem.cpp`
  - start, end
- **Tsukino::BuiltIn::ECS::EffectSystem** — `Tsukino.EngineIntegration/include/Tsukino/EngineIntegration/ECS/System/EffectSystem.hpp`
  - EffectSystem(), ~EffectSystem(), Update(), RenderEffects(), PlayEffect(), StopEffect(), PauseHandle(), ResumeHandle(), IsPlaying(), SetPlaySpeed(), SendTrigger(), StopAllEffects(), IsInitialized(), Initialize(), Finalize()
- **Tsukino::BuiltIn::ECS::FogSystem** — `Tsukino.EngineIntegration/include/Tsukino/EngineIntegration/ECS/System/FogSystem.hpp`
  - Update()
- **Tsukino::BuiltIn::ECS::FontRendererSystem** — `Tsukino.EngineIntegration/include/Tsukino/EngineIntegration/ECS/System/FontRendererSystem.hpp`
  - FontRendererSystem(), ~FontRendererSystem(), Update()
- **Tsukino::BuiltIn::ECS::FontRendererSystem::DrawEntry** — `Tsukino.EngineIntegration/include/Tsukino/EngineIntegration/ECS/System/FontRendererSystem.hpp`
  - atlas, spriteFont, text, position, origin, color, outlineColor, outlineWidth, scale, sortOrder
- **Tsukino::BuiltIn::ECS::HeightmapGenerationSystem** — `Tsukino.EngineIntegration/include/Tsukino/EngineIntegration/ECS/System/HeightmapGenerationSystem.hpp`
  - Update()
- **Tsukino::BuiltIn::ECS::InteractionSystem** — `Tsukino.EngineIntegration/include/Tsukino/EngineIntegration/ECS/System/InteractionSystem.hpp`
  - Update(), HitTest()
- **Tsukino::BuiltIn::ECS::JoltDebugRendererImpl** — `Tsukino.EngineIntegration/src/ECS/System/PhysicsSystem.cpp`
  - JoltDebugRendererImpl(), DrawLine(), DrawTriangle(), DrawText3D(), SetEngineRenderer()
- **Tsukino::BuiltIn::ECS::LightSystem** — `Tsukino.EngineIntegration/include/Tsukino/EngineIntegration/ECS/System/LightSystem.hpp`
  - Update()
- **Tsukino::BuiltIn::ECS::ModelSystem** — `Tsukino.EngineIntegration/include/Tsukino/EngineIntegration/ECS/System/ModelSystem.hpp`
  - Update()
- **Tsukino::BuiltIn::ECS::MotionBlurSystem** — `Tsukino.EngineIntegration/include/Tsukino/EngineIntegration/ECS/System/MotionBlurSystem.hpp`
  - Update()
- **Tsukino::BuiltIn::ECS::MotionVectorSnapshotSystem** — `Tsukino.EngineIntegration/include/Tsukino/EngineIntegration/ECS/System/MotionVectorSnapshotSystem.hpp`
  - Update()
- **Tsukino::BuiltIn::ECS::MyContactListener** — `Tsukino.EngineIntegration/src/ECS/System/PhysicsSystem.cpp`
  - OnContactAdded(), DrainContacts()
- **Tsukino::BuiltIn::ECS::MyContactListener::PendingContact** — `Tsukino.EngineIntegration/src/ECS/System/PhysicsSystem.cpp`
  - entityA, entityB, normal
- **Tsukino::BuiltIn::ECS::ObjectLayerPairFilterImpl** — `Tsukino.EngineIntegration/src/ECS/System/PhysicsSystem.cpp`
  - ShouldCollide()
- **Tsukino::BuiltIn::ECS::ObjectVsBroadPhaseLayerFilterImpl** — `Tsukino.EngineIntegration/src/ECS/System/PhysicsSystem.cpp`
  - ShouldCollide()
- **Tsukino::BuiltIn::ECS::PhysicsSystem** — `Tsukino.EngineIntegration/include/Tsukino/EngineIntegration/ECS/System/PhysicsSystem.hpp`
  - PhysicsSystem(), ~PhysicsSystem(), Update(), OverlapCapsule()
- **Tsukino::BuiltIn::ECS::PhysicsSystem::Impl** — `Tsukino.EngineIntegration/src/ECS/System/PhysicsSystem.cpp`
  - tempAllocator, jobSystem, bpLayerInterface, objVsBpFilter, objPairFilter, physicsSystem, contactListener, debugRenderer, prevPositions, heightfieldCache, characterContactListener, characters, eventBus, drainedContacts
- **Tsukino::BuiltIn::ECS::PhysicsSystem::Impl::CharacterHandle** — `Tsukino.EngineIntegration/src/ECS/System/PhysicsSystem.cpp`
  - character
- **Tsukino::BuiltIn::ECS::SkyAtmosphereSystem** — `Tsukino.EngineIntegration/include/Tsukino/EngineIntegration/ECS/System/SkyAtmosphereSystem.hpp`
  - Update()
- **Tsukino::BuiltIn::ECS::SpriteRenderSystem** — `Tsukino.EngineIntegration/include/Tsukino/EngineIntegration/ECS/System/SpriteRendererSystem.hpp`
  - SpriteRenderSystem(), ~SpriteRenderSystem(), Update()
- **Tsukino::BuiltIn::ECS::SpriteRenderSystem::SpriteEntry** — `Tsukino.EngineIntegration/include/Tsukino/EngineIntegration/ECS/System/SpriteRendererSystem.hpp`
  - sortOrder, cmd
- **Tsukino::BuiltIn::ECS::TransformSystem** — `Tsukino.EngineIntegration/include/Tsukino/EngineIntegration/ECS/System/TransformSystem.hpp`
  - Update()
- **Tsukino::BuiltIn::ECS::WorldAnchorSystem** — `Tsukino.EngineIntegration/include/Tsukino/EngineIntegration/ECS/System/WorldAnchorSystem.hpp`
  - Update()
- **Tsukino::EngineIntegration::EffectFileInterface** — `Tsukino.EngineIntegration/include/Tsukino/EngineIntegration/IO/EffectFileInterface.hpp`
  - EffectFileInterface(), ~EffectFileInterface(), SetBaseDirectory(), OpenRead(), OpenWrite()
- **Tsukino::EngineIntegration::EffectFileReader** — `Tsukino.EngineIntegration/include/Tsukino/EngineIntegration/IO/EffectFileInterface.hpp`
  - EffectFileReader(), Read(), Seek(), GetPosition(), GetLength(), GetData()
- **Tsukino::EngineIntegration::EngineAPI** — `Tsukino.EngineIntegration/include/Tsukino/EngineIntegration/EngineAPI.hpp`
  - EngineAPI(), ChangeScene(), ProcessMessages(), Update(), Render()
- **Tsukino::EngineIntegration::EngineContext** — `Tsukino.EngineIntegration/include/Tsukino/EngineIntegration/EngineContext.hpp`
  - renderer, window, inputSystem, assetManager, builtinAssets, gameSceneManager, audioManager, prefabFactory, effectSystem, physicsSystem
- **Tsukino::EngineIntegration::EngineIntegration** — `Tsukino.EngineIntegration/include/Tsukino/EngineIntegration/EngineIntegration.hpp`
  - EngineIntegration(), ~EngineIntegration(), EngineIntegration(), operator=(), EngineIntegration(), operator=(), Initialize(), GetContext()
- **Tsukino::EngineIntegration::GameSceneBase** — `Tsukino.EngineIntegration/include/Tsukino/EngineIntegration/Scene/GameSceneBase.hpp`
  - ~GameSceneBase(), Initialize(), OnUpdate(), OnExit(), GetScene()
- **Tsukino::EngineIntegration::GameSceneManager** — `Tsukino.EngineIntegration/include/Tsukino/EngineIntegration/Scene/GameSceneManager.hpp`
  - GameSceneManager(), ~GameSceneManager(), GameSceneManager(), operator=(), GameSceneManager(), operator=(), Initialize(), ChangeScene(), Update(), GetCurrentScene()

### Tsukino.GraphicsCommon

- **Tsukino::GraphicsCommon::AABB** — `Tsukino.GraphicsCommon/include/Tsukino/GraphicsCommon/Mesh/MeshData.hpp`
  - min, max, serialize()
- **Tsukino::GraphicsCommon::AnimationChannel** — `Tsukino.GraphicsCommon/include/Tsukino/GraphicsCommon/Model/ModelData.hpp`
  - nodeName, positionKeys, rotationKeys, scaleKeys, serialize()
- **Tsukino::GraphicsCommon::AnimationData** — `Tsukino.GraphicsCommon/include/Tsukino/GraphicsCommon/Model/ModelData.hpp`
  - name, duration, ticksPerSecond, channels, serialize()
- **Tsukino::GraphicsCommon::BoneInfo** — `Tsukino.GraphicsCommon/include/Tsukino/GraphicsCommon/Model/ModelData.hpp`
  - name, nodeIndex, inverseBindPose, serialize()
- **Tsukino::GraphicsCommon::BoneWeight** — `Tsukino.GraphicsCommon/include/Tsukino/GraphicsCommon/Mesh/MeshData.hpp`
  - boneIndices, weights, serialize()
- **Tsukino::GraphicsCommon::DebugVertex** — `Tsukino.GraphicsCommon/include/Tsukino/GraphicsCommon/Vertex/DebugVertex.hpp`
  - position, color
- **Tsukino::GraphicsCommon::MaterialData** — `Tsukino.GraphicsCommon/include/Tsukino/GraphicsCommon/Material/MaterialData.hpp`
  - name, shadingModel, baseColor, emissive, metallic, roughness, specular, waterSpeed, waterScale, waterHeight, albedoMap, normalMap, metallicRoughnessMap, emissiveMap, aoMap, serialize()
- **Tsukino::GraphicsCommon::MeshData** — `Tsukino.GraphicsCommon/include/Tsukino/GraphicsCommon/Mesh/MeshData.hpp`
  - vertexData, indices, boneWeights, vertexStride, vertexCount, indexCount, materialIndex, format, bounds, serialize()
- **Tsukino::GraphicsCommon::ModelData** — `Tsukino.GraphicsCommon/include/Tsukino/GraphicsCommon/Model/ModelData.hpp`
  - nodes, meshes, materials, animations, skeleton, rootNodeIndex, serialize()
- **Tsukino::GraphicsCommon::NodeData** — `Tsukino.GraphicsCommon/include/Tsukino/GraphicsCommon/Node/NodeData.hpp`
  - name, meshIndices, translation, rotation, scale, parentIndex, childIndices, serialize()
- **Tsukino::GraphicsCommon::QuaternionKey** — `Tsukino.GraphicsCommon/include/Tsukino/GraphicsCommon/Model/ModelData.hpp`
  - time, value, serialize()
- **Tsukino::GraphicsCommon::SkeletonData** — `Tsukino.GraphicsCommon/include/Tsukino/GraphicsCommon/Model/ModelData.hpp`
  - bones, serialize()
- **Tsukino::GraphicsCommon::VectorKey** — `Tsukino.GraphicsCommon/include/Tsukino/GraphicsCommon/Model/ModelData.hpp`
  - time, value, serialize()
- **Tsukino::GraphicsCommon::VertexPNUV** — `Tsukino.GraphicsCommon/include/Tsukino/GraphicsCommon/Vertex/VertexPNUV.hpp`
  - position, normal, uv
- **Tsukino::GraphicsCommon::VertexPUV** — `Tsukino.GraphicsCommon/include/Tsukino/GraphicsCommon/Vertex/VertexPUV.hpp`
  - position, uv

### Tsukino.Renderer

- **Tsukino::Renderer::CBufferFog** — `Tsukino.Renderer/include/Tsukino/Renderer/ConstantBuffer.hpp`
  - color, distanceParams, heightParams, sunColor, noiseParams, windParams
- **Tsukino::Renderer::CBufferLights** — `Tsukino.Renderer/include/Tsukino/Renderer/ConstantBuffer.hpp`
  - lightCount, pad, lights
- **Tsukino::Renderer::CBufferMaterial** — `Tsukino.Renderer/include/Tsukino/Renderer/ConstantBuffer.hpp`
  - baseColor, emissive, metallic, roughness, specular, rimColor, rimParams
- **Tsukino::Renderer::CBufferMotionBlur** — `Tsukino.Renderer/include/Tsukino/Renderer/ConstantBuffer.hpp`
  - strength, maxBlurRadius, shutterScale, sampleCount
- **Tsukino::Renderer::CBufferScene** — `Tsukino.Renderer/include/Tsukino/Renderer/ConstantBuffer.hpp`
  - view, projection, viewProj, invViewProj, lightViewProj, lightDir, lightColor, cameraPos, prevViewProj
- **Tsukino::Renderer::CBufferSkinning** — `Tsukino.Renderer/include/Tsukino/Renderer/ConstantBuffer.hpp`
  - bones
- **Tsukino::Renderer::CBufferSkinningPrev** — `Tsukino.Renderer/include/Tsukino/Renderer/ConstantBuffer.hpp`
  - bones
- **Tsukino::Renderer::CBufferSky** — `Tsukino.Renderer/include/Tsukino/Renderer/ConstantBuffer.hpp`
  - rayleighScattering, mieScattering, mieAnisotropy, sunIntensity, atmosphereHeight, planetRadius, sunDiskSize, padding0, groundColor, sunDirection
- **Tsukino::Renderer::CBufferTransform** — `Tsukino.Renderer/include/Tsukino/Renderer/ConstantBuffer.hpp`
  - world, prevWorld, motionFlags
- **Tsukino::Renderer::CBufferWater** — `Tsukino.Renderer/include/Tsukino/Renderer/ConstantBuffer.hpp`
  - time, waveSpeed, waveScale, fresnelPower, shallowColor, deepColor
- **Tsukino::Renderer::DX11Texture2D** — `Tsukino.Renderer/include/Tsukino/Renderer/DX11/Texture/DX11Texture2D.hpp`
  - DX11Texture2D(), Bind(), GetWidth(), GetHeight(), GetSRV()
- **Tsukino::Renderer::DrawCommand** — `Tsukino.Renderer/include/Tsukino/Renderer/DrawCommand.hpp`
  - material, mesh, customDraw, transform, pass, materialData, sortOrder, boneMatrices, boneCount, prevTransform, prevBoneMatrices, hasPrevFrame
- **Tsukino::Renderer::DrawCommandQueue** — `Tsukino.Renderer/include/Tsukino/Renderer/DrawCommandQueue.hpp`
  - Push(), GetCommands(), Clear(), Size()
- **Tsukino::Renderer::DynamicFontAtlas** — `Tsukino.Renderer/include/Tsukino/Renderer/Text/DynamicFontAtlas.hpp`
  - DynamicFontAtlas(), DynamicFontAtlas(), operator=(), DrawString(), MeasureString(), GetLineHeight(), GetAscent()
- **Tsukino::Renderer::DynamicFontAtlas::GlyphInfo** — `Tsukino.Renderer/include/Tsukino/Renderer/Text/DynamicFontAtlas.hpp`
  - atlasRect, page, bearingX, bearingY, advanceX, hasInk
- **Tsukino::Renderer::DynamicFontAtlas::Page** — `Tsukino.Renderer/include/Tsukino/Renderer/Text/DynamicFontAtlas.hpp`
  - texture, srv, cursorX, cursorY, shelfHeight
- **Tsukino::Renderer::GPULight** — `Tsukino.Renderer/include/Tsukino/Renderer/ConstantBuffer.hpp`
  - positionRange, colorIntensity, directionType, spotParams
- **Tsukino::Renderer::GraphicsContext** — `Tsukino.Renderer/include/Tsukino/Renderer/DX11/GraphicsContext.hpp`
  - GBufferCount, Initialize(), BeginFrame(), EndFrame(), GetDevice(), GetContext(), SetPipelineState(), SetMaterial(), GetHDRSRV(), BindBackBuffer(), BeginGBufferPass(), GetPostProcessSRV(), BindPostProcessTarget(), BindHDRRenderTarget(), BindHDRTargetOnly(), GetGBufferSRV(), GetDepthSRV(), Resize(), GetWidth(), GetHeight(), SetVSyncEnabled(), IsVSyncEnabled()
- **Tsukino::Renderer::Material** — `Tsukino.Renderer/include/Tsukino/Renderer/DX11/Material.hpp`
  - TextureSlotCount, SetPipeline(), SetTexture(), SetTexture(), SetSampler(), GetPipeline(), GetTexture(), GetTextures(), GetSampler()
- **Tsukino::Renderer::MeshBuffer** — `Tsukino.Renderer/include/Tsukino/Renderer/DX11/MeshBuffer.hpp`
  - vertexBuffer, indexBuffer, boneWeightBuffer, vertexCount, indexCount, stride
- **Tsukino::Renderer::PipelineFactory** — `Tsukino.Renderer/include/Tsukino/Renderer/DX11/PipelineFactory.hpp`
  - PipelineFactory(), Create()
- **Tsukino::Renderer::PipelineHash** — `Tsukino.Renderer/include/Tsukino/Renderer/DX11/PipelineFactory.hpp`
  - operator()()
- **Tsukino::Renderer::PipelineState** — `Tsukino.Renderer/include/Tsukino/Renderer/DX11/PipelineState.hpp`
  - vs, ps, inputLayout, rasterizer, blend, depth, topology
- **Tsukino::Renderer::Renderer** — `Tsukino.Renderer/include/Tsukino/Renderer/Renderer.hpp`
  - Renderer(), ~Renderer(), Initialize(), Render(), Resize(), SetClearColor(), PushDrawCommand(), GetFrameStats(), SetVSyncEnabled(), IsVSyncEnabled(), DrawDebugLine(), DrawDebugTriangle(), FlushDebugDraw(), GetPipelineFactory(), GetDevice(), GetContext(), GetPrimitiveMesh(), GetSampler(), GetTextureSRV(), UpdateSceneBuffer(), CreateSpriteFont(), SetWorldCameraMatrix(), SetOverlayCameraMatrix(), CreateSpriteBatch(), GetCommonStatesTK(), SetDirectionalLight(), SetShadowPipeline(), GetWhiteTextureSRV(), GetFlatNormalTextureSRV(), SetSkyParameters(), SetSkyPipeline(), UpdateWaterTime(), SetWaterParameters(), SetWaterPipeline(), SetLights(), SetMotionBlurPipeline(), SetMotionBlurParameters(), SetMotionBlurEnabled(), SetFogParameters(), SetFogEnabled()
- **Tsukino::Renderer::Renderer::FrameStats** — `Tsukino.Renderer/include/Tsukino/Renderer/Renderer.hpp`
  - commandCount, shadowDrawCalls, gbufferDrawCalls, worldDrawCalls, transparentDrawCalls, waterDrawCalls, overlayDrawCalls, skinnedDrawCalls, triangleCount, boneBytesUploaded, TotalDrawCalls()
- **Tsukino::Renderer::RendererShaderSet** — `Tsukino.Renderer/include/Tsukino/Renderer/Renderer.hpp`
  - debugVS, debugPS, tonemapVS, tonemapPS, shadowStaticVS, shadowSkeletalVS, shadowPS, lightingPS, motionBlurPS, fogPS
- **Tsukino::Renderer::Shader** — `Tsukino.Renderer/include/Tsukino/Renderer/Shader.hpp`
  - Shader(), ~Shader(), Bind()
- **Tsukino::Renderer::ShaderLoader** — `Tsukino.Renderer/include/Tsukino/Renderer/ShaderLoader.hpp`
  - LoadFromFile()
- **Tsukino::Renderer::SpriteRenderer** — `Tsukino.Renderer/include/Tsukino/Renderer/SpriteRenderer.hpp`
  - SpriteRenderer(), Draw()

### Tsukino.Sandbox

- **Tsukino::Sandbox::BlockBreakingSampleScene** — `Tsukino.Sandbox/include/Tsukino/Sandbox/Scene/BlockBreakingSampleScene.hpp`
  - BlockBreakingSampleScene(), ~BlockBreakingSampleScene(), OnUpdate(), OnExit()
- **Tsukino::Sandbox::DeferredLightSampleScene** — `Tsukino.Sandbox/include/Tsukino/Sandbox/Scene/DeferredLightSampleScene.hpp`
  - DeferredLightSampleScene(), ~DeferredLightSampleScene(), OnUpdate(), OnExit()
- **Tsukino::Sandbox::JumpGameSampleScene** — `Tsukino.Sandbox/include/Tsukino/Sandbox/Scene/JumpGameSampleScene.hpp`
  - JumpGameSampleScene(), ~JumpGameSampleScene(), OnUpdate(), OnExit()
- **Tsukino::Sandbox::SampleScene1** — `Tsukino.Sandbox/include/Tsukino/Sandbox/Scene/SampleScene1.hpp`
  - SampleScene1(), ~SampleScene1(), OnUpdate(), OnExit()
- **Tsukino::Sandbox::TestLinkComponent** — `Tsukino.Sandbox/src/Scene/SceneSample1.cpp`
  - target
- **Tsukino::Sandbox::WaterGameSampleScene** — `Tsukino.Sandbox/include/Tsukino/Sandbox/Scene/WaterGameSampleScene.hpp`
  - WaterGameSampleScene(), ~WaterGameSampleScene(), OnUpdate(), OnExit()
