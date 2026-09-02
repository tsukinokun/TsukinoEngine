# Tsukino.Core の公開 API

**このファイルは自動生成です。直接編集しないでください。**

型の在処だけ知りたいときは `../api-index.md` を見る。

## 主要な型

ゲーム側から実際に触る型。メンバを全て展開している。

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

## 全公開型の索引

メンバ名のみ。詳細が要るときはヘッダを開く。

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
