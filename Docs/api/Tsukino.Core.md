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
| `Path(const char *path)` | コンストラクタ |
| `Path(const std::filesystem::path &path)` | std::filesystem::path から作るコンストラクタ |
| `const std::string & string() const noexcept` | パス文字列を取得する関数 |
| `Path parent() const` | パスの親ディレクトリを取得する関数 |
| `std::string filename() const` | パスのファイル名を取得する関数 |
| `std::string extension() const` | パスの拡張子を取得する関数 |
| `std::string stem() const` | 拡張子を除いたファイル名を取得する関数 |
| `Path parent_path() const` | パスの親ディレクトリを取得する関数 |
| `std::filesystem::path ToStdPath() const` | OS のファイル操作に渡せるパスを取得する関数 |
| `Path operator/(const Path &rhs) const` | パスを結合する演算子 |
| `void replace_extension(const std::string &newExt)` | 拡張子を置き換える関数 |
| `std::pair< std::string, std::string > SplitPathAndFragment(const std::string &rawPath)` | フラグメントなしとフラグメントを分割して取得する関数 |
| `std::string ToLower(std::string value)` | 文字列を小文字に変換する関数（ASCII のみ） |

### Tsukino::Core::Window

`Tsukino.Core/include/Tsukino/Core/Window.hpp`

**公開関数**

| シグネチャ | 説明 |
|---|---|
| `Window()` | コンストラクタ |
| `~Window()` | デストラクタ |
| `Window(const Window &)=delete` |  |
| `Window & operator=(const Window &)=delete` |  |
| `bool Create(const std::string &title, int width, int height, WindowStyle style=WindowStyle::Default)` | ウィンドウ生成 |
| `bool ProcessMessages()` | メッセージ処理 |
| `void SetInputCallback(InputCallback callback)` |  |
| `void SetResizeCallback(ResizeCallback callback)` |  |
| `void SetFocusLostCallback(FocusLostCallback callback)` |  |
| `void SetLifecycleCallback(LifecycleCallback callback)` |  |
| `SafeArea GetSafeArea() const` | UI を置いてよい四角を取得する関数 |
| `bool IsInBackground() const` |  |
| `void SetHitTestCallback(HitTestCallback callback)` |  |
| `SDL_Window * GetSDLWindow() const` |  |
| `void * GetNativeHandle() const` | OS のウィンドウハンドルを取得する関数 |
| `int GetWidth() const` |  |
| `int GetHeight() const` |  |
| `void SetTopmost(bool enable)` | 動的に最前面表示を切り替える関数 |
| `bool IsTopmost() const` |  |
| `void SetUpdateMode(UpdateMode mode)` | ウィンドウ更新モードを設定する関数 |
| `void SetClickThroughEnabled(bool enabled)` | クリック透過（WS_EX_TRANSPARENT）を動的に切り替える関数 |
| `void UpdateClickThroughFromHitTest(int screenX, int screenY)` | 登録済みの HitTestCallback を使って クリック透過の有効/無効を更新する関数 |
| `void EnqueueInput(const Tsukino::Input::InputEvent &event)` | 低レベルフックから呼ばれる、入力をキューへ積むだけの関数 |
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
| `template <typename TEvent> void Enqueue(const TEvent &event)` | イベントの発火を後回しにする（キューに積む） |
| `void FlushQueued()` | Enqueue で積んだイベントを、積んだ順に発火する |
| `void ClearQueued()` | Enqueue で積んだまま発火していないイベントを捨てる |
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
| `~InputSystem()` | デストラクタ |
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
| `const std::vector< TouchPoint > & GetTouches() const` |  |
| `void PollGamepads(float deltaTime, bool windowFocused)` | 全スロットのゲームパッドの状態を読み直します。 |
| `const Gamepad & GetGamepad(u32 index) const` | スロットを指定してゲームパッドを得ます。 |
| `bool IsButtonDown(GamepadButton button) const` | いずれかのゲームパッドでボタンが押されているかを返します。 |
| `bool IsButtonPressed(GamepadButton button) const` | いずれかのゲームパッドでボタンが押された瞬間かを返します。 |
| `bool IsButtonReleased(GamepadButton button) const` | いずれかのゲームパッドでボタンが離された瞬間かを返します。 |
| `hlslpp::float2 GetLeftStick() const` | 左スティックの傾きを返します。 |
| `hlslpp::float2 GetRightStick() const` | 右スティックの傾きを返します。 |
| `hlslpp::float2 GetStick(GamepadStick stick) const` | 指定したスティックの傾きを返します。 |
| `void SetGamepadVibration(float lowFrequency, float highFrequency, float durationSeconds)` | 接続されている全てのゲームパッドを振動させます。 |
| `void StopGamepadVibration()` | 全てのゲームパッドの振動を止めます。 |
| `void SetGamepadStickDeadZone(float deadZone)` | 全てのゲームパッドのスティックのデッドゾーンを設定します。 |
| `void SetGamepadTriggerThreshold(float threshold)` | 全てのゲームパッドのトリガーのしきい値を設定します。 |
| `bool IsAnyGamepadConnected() const` | いずれかのゲームパッドが接続されているかを返します。 |
| `InputDevice GetLastInputDevice() const` |  |
| `void SetKeyState(KeyCode code, bool isDown)` | キー状態を直接書き換える |
| `void SetMousePosition(i32 x, i32 y)` | マウスの座標を直接書き換える |
| `void AddWheelDelta(float delta)` | ホイールの回転量を加算する |
| `void SetTouchState(u64 fingerId, i32 x, i32 y, bool isDown)` | 指が触れた・動いた・離れたことを書き込みます。 |
| `void NotifyKeyboardMouseActivity()` | キーボードかマウスが操作されたことを知らせます。 |
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
  - Path(), Path(), Path(), Path(), string(), parent(), filename(), extension(), stem(), parent_path(), ToStdPath(), operator/(), replace_extension(), SplitPathAndFragment(), ToLower()
- **Tsukino::Core::Sequencer** — `Tsukino.Core/include/Tsukino/Core/Time/Sequencer.hpp`
  - Schedule(), Update(), Clear(), IsBusy()
- **Tsukino::Core::Sequencer::Entry** — `Tsukino.Core/include/Tsukino/Core/Time/Sequencer.hpp`
  - time, order, callback
- **Tsukino::Core::Window** — `Tsukino.Core/include/Tsukino/Core/Window.hpp`
  - Window(), ~Window(), Window(), operator=(), Create(), ProcessMessages(), SetInputCallback(), SetResizeCallback(), SetFocusLostCallback(), SetLifecycleCallback(), GetSafeArea(), IsInBackground(), SetHitTestCallback(), GetSDLWindow(), GetNativeHandle(), GetWidth(), GetHeight(), SetTopmost(), IsTopmost(), SetUpdateMode(), SetClickThroughEnabled(), UpdateClickThroughFromHitTest(), EnqueueInput(), SetFullscreen(), SetCursorVisible(), IsFocused(), CenterCursor()
- **Tsukino::Core::Window::SafeArea** — `Tsukino.Core/include/Tsukino/Core/Window.hpp`
  - x, y, width, height
- **Tsukino::ECS::EntityRef** — `Tsukino.Core/include/Tsukino/Core/ECS/EntityRef/EntityRef.hpp`
  - entity, localName, EntityRef(), EntityRef(), operator=(), operator=(), operator Entity(), operator==(), operator!=(), operator==(), operator!=()
- **Tsukino::ECS::EntityRefResolverArchive** — `Tsukino.Core/include/Tsukino/Core/ECS/EntityRef/EntityRefResolverArchive.hpp`
  - EntityRefResolverArchive(), operator()()
- **Tsukino::ECS::EventBus** — `Tsukino.Core/include/Tsukino/Core/ECS/Event/EventBus.hpp`
  - EventBus(), ~EventBus(), EventBus(), operator=(), EventBus(), operator=(), Subscribe(), Publish(), Enqueue(), FlushQueued(), ClearQueued(), Clear(), ClearAll()
- **Tsukino::ECS::EventBus::HandlerEntry** — `Tsukino.Core/include/Tsukino/Core/ECS/Event/EventBus.hpp`
  - id, invoke
- **Tsukino::ECS::ISystem** — `Tsukino.Core/include/Tsukino/Core/ECS/System/ISystem.hpp`
  - ~ISystem(), Update()
- **Tsukino::ECS::Registry** — `Tsukino.Core/include/Tsukino/Core/ECS/Registry/Registry.hpp`
  - CreateEntity(), DestroyEntity(), QueueDestroy(), FlushDestroyQueue(), Clear(), OnConstruct(), OnDestroy(), AddComponent(), GetComponent(), HasComponent(), try_get(), RemoveComponent(), View(), SetContext(), GetContext(), HasContext(), RemoveContext(), IsValid()
- **Tsukino::ECS::ScopedConnection** — `Tsukino.Core/include/Tsukino/Core/ECS/Event/ScopedConnection.hpp`
  - ScopedConnection(), ScopedConnection(), ScopedConnection(), operator=(), ScopedConnection(), operator=(), ~ScopedConnection(), Disconnect(), IsConnected()
- **Tsukino::IO::FileSystem** — `Tsukino.Core/include/Tsukino/Core/IO/FileSystem.hpp`
  - ReadBinary(), ReadBinaryHead(), ReadText(), WriteBinary(), WriteText(), Exists(), CreateDirectories(), GetLastWriteTime(), GetAssetRootPath(), GetEngineAssetRootPath(), ToEngineRelativePath(), GetUserDataPath()
- **Tsukino::IO::FileWatcher** — `Tsukino.Core/include/Tsukino/Core/IO/FileWatcher.hpp`
  - Watch(), Poll(), SetInterval(), GetInterval()
- **Tsukino::IO::FileWatcher::Entry** — `Tsukino.Core/include/Tsukino/Core/IO/FileWatcher.hpp`
  - path, onChanged, knownTime, pendingTime, pending
- **Tsukino::IO::HotReloadable** — `Tsukino.Core/include/Tsukino/Core/IO/HotReloadable.hpp`
  - HotReloadable(), HotReloadable(), operator=(), Get()
- **Tsukino::Input::Gamepad** — `Tsukino.Core/include/Tsukino/Core/Input/Gamepad.hpp`
  - kDefaultStickDeadZone, kDefaultTriggerThreshold, kStickButtonPressThreshold, kStickButtonReleaseThreshold, Gamepad(), Close(), Poll(), IsConnected(), IsButtonDown(), IsButtonPressed(), IsButtonReleased(), AnyButtonPressed(), GetLeftStick(), GetRightStick(), GetLeftTrigger(), GetRightTrigger(), SetVibration(), StopVibration(), SetStickDeadZone(), SetTriggerThreshold()
- **Tsukino::Input::InputActionMap** — `Tsukino.Core/include/Tsukino/Core/Input/InputActionMap.hpp`
  - SetBinding(), GetBinding(), IsDown(), IsPressed(), IsReleased()
- **Tsukino::Input::InputAxisBinding** — `Tsukino.Core/include/Tsukino/Core/Input/InputActionMap.hpp`
  - up, down, left, right, stick
- **Tsukino::Input::InputBinding** — `Tsukino.Core/include/Tsukino/Core/Input/InputActionMap.hpp`
  - keys, buttons, wheelDirection
- **Tsukino::Input::InputEvent** — `Tsukino.Core/include/Tsukino/Core/Input/InputEvent.hpp`
  - type, key, repeat, x, y, wheel, fingerId, fromTouch, text
- **Tsukino::Input::InputSystem** — `Tsukino.Core/include/Tsukino/Core/Input/InputSystem.hpp`
  - InputSystem(), ~InputSystem(), InputSystem(), operator=(), Update(), IsKeyDown(), IsKeyPressed(), AnyKeyPressed(), IsKeyReleased(), GetMousePosition(), GetMouseDelta(), GetWheelDelta(), GetTouches(), PollGamepads(), GetGamepad(), IsButtonDown(), IsButtonPressed(), IsButtonReleased(), GetLeftStick(), GetRightStick(), GetStick(), SetGamepadVibration(), StopGamepadVibration(), SetGamepadStickDeadZone(), SetGamepadTriggerThreshold(), IsAnyGamepadConnected(), GetLastInputDevice(), SetKeyState(), SetMousePosition(), AddWheelDelta(), SetTouchState(), NotifyKeyboardMouseActivity(), ClearAllKeys()
- **Tsukino::Input::TextInputSystem** — `Tsukino.Core/include/Tsukino/Core/Input/TextInputSystem.hpp`
  - Attach(), Begin(), End(), IsActive(), HandleEvent(), GetText(), SetText(), GetComposition(), SetCandidatePosition()
- **Tsukino::Input::TouchPoint** — `Tsukino.Core/include/Tsukino/Core/Input/InputSystem.hpp`
  - id, x, y, began, ended

## 列挙型

- **Tsukino::Core::DebugTools::ProfileSlot** — `Tsukino.Core/include/Tsukino/Core/DebugTools/FrameProfiler.hpp` — フレーム内の計測区間
  - `Frame = 0` — メインループ1周（アプリのメインループが計測するdeltaTime）
  - `Update` — EngineAPI::Update（＝全システムのUpdate）
  - `Render` — EngineAPI::Render（＝描画コマンドのサブミット）
  - `Count`
- **Tsukino::Core::Math::Easing::EaseType** — `Tsukino.Core/include/Tsukino/Core/Math/Easing.hpp` — イージングの種類です。
  - `Linear`
  - `InQuad`
  - `OutQuad`
  - `InOutQuad`
  - `InCubic`
  - `OutCubic`
  - `InOutCubic`
  - `OutBack`
  - `OutElastic`
  - `OutBounce`
  - `InOutSine`
  - `Punch`
- **Tsukino::Core::Window::AppLifecycle** — `Tsukino.Core/include/Tsukino/Core/Window.hpp` — アプリの状態の変化（主にスマホ）です。
  - `WillEnterBackground` — 裏へ回る（スマホのホーム画面へ戻ったときなど）。音と描画を止める
  - `DidEnterForeground` — 表へ戻った。描画先を付け直して再開する
  - `LowMemory` — OS からメモリが足りないと知らされた
- **Tsukino::Core::Window::UpdateMode** — `Tsukino.Core/include/Tsukino/Core/Window.hpp` — ウィンドウ更新モードの列挙型
  - `ActiveOnly`
  - `AlwaysResident`
- **Tsukino::Core::Window::WindowStyle** — `Tsukino.Core/include/Tsukino/Core/Window.hpp` — ウィンドウスタイルの列挙型
  - `Default`
  - `Popup`
  - `ClickThrough`
- **Tsukino::Input::GamepadButton** — `Tsukino.Core/include/Tsukino/Core/Input/GamepadCodes.hpp` — ゲームパッドのボタンの列挙型です。
  - `DPadUp = 0`
  - `DPadDown`
  - `DPadLeft`
  - `DPadRight`
  - `Start`
  - `Back`
  - `LeftThumb`
  - `RightThumb`
  - `LeftShoulder`
  - `RightShoulder`
  - `A`
  - `B`
  - `X`
  - `Y`
  - `LeftTrigger`
  - `RightTrigger`
  - `LeftStickUp`
  - `LeftStickDown`
  - `LeftStickLeft`
  - `LeftStickRight`
  - `Count`
- **Tsukino::Input::GamepadStick** — `Tsukino.Core/include/Tsukino/Core/Input/GamepadCodes.hpp` — スティックの種類の列挙型です。
  - `None = 0`
  - `Left`
  - `Right`
- **Tsukino::Input::InputDevice** — `Tsukino.Core/include/Tsukino/Core/Input/GamepadCodes.hpp` — 入力機器の種類の列挙型です。
  - `KeyboardMouse = 0`
  - `Gamepad`
- **Tsukino::Input::InputEventType** — `Tsukino.Core/include/Tsukino/Core/Input/InputEvent.hpp` — 入力の出来事の種類です。
  - `KeyDown`
  - `KeyUp`
  - `MouseMove`
  - `MouseWheel`
  - `TextInput`
  - `TextEditing`
  - `TouchDown`
  - `TouchMove`
  - `TouchUp`
- **Tsukino::Input::KeyCode** — `Tsukino.Core/include/Tsukino/Core/Input/KeyCodes.hpp` — キーコードの列挙型
  - `None = 0x00`
  - `LButton = 0x01`
  - `RButton = 0x02`
  - `MButton = 0x04`
  - `Backspace = 0x08`
  - `Tab = 0x09`
  - `Enter = 0x0D`
  - `Shift = 0x10`
  - `Control = 0x11`
  - `Alt = 0x12`
  - `Escape = 0x1B`
  - `Space = 0x20`
  - `PageUp = 0x21`
  - `PageDown = 0x22`
  - `End = 0x23`
  - `Home = 0x24`
  - `Left = 0x25`
  - `Up = 0x26`
  - `Right = 0x27`
  - `Down = 0x28`
  - `D0 = 0x30`
  - `D1`
  - `D2`
  - `D3`
  - `D4`
  - `D5`
  - `D6`
  - `D7`
  - `D8`
  - `D9`
  - `A = 0x41`
  - `B`
  - `C`
  - `D`
  - `E`
  - `F`
  - `G`
  - `H`
  - `I`
  - `J`
  - `K`
  - `L`
  - `M`
  - `N`
  - `O`
  - `P`
  - `Q`
  - `R`
  - `S`
  - `T`
  - `U`
  - `V`
  - `W`
  - `X`
  - `Y`
  - `Z`
  - `F1 = 0x70`
  - `F2`
  - `F3`
  - `F4`
  - `F5`
  - `F6`
  - `F7`
  - `F8`
  - `F9`
  - `F10`
  - `F11`
  - `F12`
- **Tsukino::Input::MouseButton** — `Tsukino.Core/include/Tsukino/Core/Input/KeyCodes.hpp` — マウスボタンの列挙型
  - `Left = 0`
  - `Right = 1`
  - `Middle = 2`
  - `Max = 3`

## 型の別名

- `using Tsukino::i16 = int16_t` — `Tsukino.Core/include/Tsukino/Core/typedef.hpp`
- `using Tsukino::i32 = int32_t` — `Tsukino.Core/include/Tsukino/Core/typedef.hpp`
- `using Tsukino::i64 = int64_t` — `Tsukino.Core/include/Tsukino/Core/typedef.hpp`
- `using Tsukino::i8 = int8_t` — `Tsukino.Core/include/Tsukino/Core/typedef.hpp`
- `using Tsukino::u16 = uint16_t` — `Tsukino.Core/include/Tsukino/Core/typedef.hpp`
- `using Tsukino::u32 = uint32_t` — `Tsukino.Core/include/Tsukino/Core/typedef.hpp`
- `using Tsukino::u64 = uint64_t` — `Tsukino.Core/include/Tsukino/Core/typedef.hpp`
- `using Tsukino::u8 = uint8_t` — `Tsukino.Core/include/Tsukino/Core/typedef.hpp`
- `using Tsukino::Core::Ref = std::shared_ptr<T>` — `Tsukino.Core/include/Tsukino/Core/Memory.hpp`
- `using Tsukino::ECS::Entity = entt::entity` — `Tsukino.Core/include/Tsukino/Core/ECS/Entity/Entity.hpp`

## 名前空間の関数

### Tsukino::Core::Math::Color — `Tsukino.Core/include/Tsukino/Core/Math/Color.hpp`

| シグネチャ | 説明 |
|---|---|
| `hlslpp::float4 FromSrgb8(int r, int g, int b, float alpha=1.0f)` |  |
| `float LinearToSrgb(float c)` |  |
| `hlslpp::float4 LinearToSrgb(const hlslpp::float4 &color)` |  |
| `float SrgbToLinear(float c)` |  |
| `hlslpp::float3 SrgbToLinear(const hlslpp::float3 &color)` |  |
| `hlslpp::float4 SrgbToLinear(const hlslpp::float4 &color)` |  |

### Tsukino::Core::Math::Easing — `Tsukino.Core/include/Tsukino/Core/Math/Easing.hpp`

| シグネチャ | 説明 |
|---|---|
| `float Clamp01(float t)` |  |
| `double Damp(double current, double target, float speed, float deltaTime)` |  |
| `float Damp(float current, float target, float speed, float deltaTime)` |  |
| `float Evaluate(EaseType type, float t)` |  |
| `float InCubic(float t)` |  |
| `float InOutCubic(float t)` |  |
| `float InOutQuad(float t)` |  |
| `float InOutSine(float t)` |  |
| `float InQuad(float t)` |  |
| `float Linear(float t)` |  |
| `float OutBack(float t, float overshoot=kDefaultOvershoot)` |  |
| `float OutBounce(float t)` |  |
| `float OutCubic(float t)` |  |
| `float OutElastic(float t)` |  |
| `float OutQuad(float t)` |  |
| `float Punch(float t)` |  |

### Tsukino::Core::Math::Spring — `Tsukino.Core/include/Tsukino/Core/Math/Spring.hpp`

| シグネチャ | 説明 |
|---|---|
| `template <typename T> void Step(T &position, T &velocity, const T &target, float frequency, float damping, float deltaTime)` |  |

### Tsukino::Core::Math — `Tsukino.Core/include/Tsukino/Core/Math/MathHelper.hpp`

| シグネチャ | 説明 |
|---|---|
| `bool Intersects(const hlslpp::float2 &min1, const hlslpp::float2 &size1, const hlslpp::float2 &min2, const hlslpp::float2 &size2)` | 2つの矩形（AABB）が重なっているかを判定する |
| `bool IsPointInRect(const hlslpp::float2 &point, const hlslpp::float2 &center, const hlslpp::float2 &size)` | 中心点とサイズから、点が矩形内にあるか判定する関数 |
| `hlslpp::quaternion SlerpShortestPath(const hlslpp::quaternion &q0, const hlslpp::quaternion &q1, float t)` | 最短経路を保証したslerp |
| `float ToDegrees(float radians)` | 弧度法を度数法に変換する関数 |
| `float ToRadians(float degrees)` | 度数法を弧度法に変換する関数 |

### Tsukino::Core::Utf8 — `Tsukino.Core/include/Tsukino/Core/Utf8.hpp`

| シグネチャ | 説明 |
|---|---|
| `void Append(std::string &text, char32_t codepoint)` | 1文字を UTF-8 にして末尾へ足します。 |
| `std::size_t CountCodepoints(std::string_view text) noexcept` | 文字数（コードポイントの数）を数えます。 |
| `char32_t DecodeNext(std::string_view text, std::size_t &index) noexcept` | 1文字を読み、位置を次の文字へ進めます。 |
| `void PopBack(std::string &text) noexcept` | 末尾の1文字を消します（文字の途中で切らない）。 |

### Tsukino::Core — `Tsukino.Core/include/Tsukino/Core/Memory.hpp`

| シグネチャ | 説明 |
|---|---|
| `template <typename T, typename... Args> Ref< T > CreateRef(Args &&... args)` | Ref<T> を作成する関数 |

### Tsukino::ECS — `Tsukino.Core/include/Tsukino/Core/ECS/EntityRef/EntityRef.hpp`

| シグネチャ | 説明 |
|---|---|
| `template <class Archive> void load_minimal(const Archive &, EntityRef &ref, const std::string &value)` | cereal用：読み込み処理（localNameのみを読み込む。entityの解決はEntityRefResolverArchiveが行う） |
| `template <class Archive> std::string save_minimal(const Archive &, const EntityRef &ref)` | cereal用：保存処理（localNameのみを書き出す） |

### Tsukino::IO::HotReload — `Tsukino.Core/include/Tsukino/Core/IO/HotReload.hpp`

| シグネチャ | 説明 |
|---|---|
| `void Poll()` |  |
| `void SetPaused(bool paused)` |  |
| `void Watch(const Tsukino::Core::Path &path, std::function< bool()> reload)` |  |

### Tsukino::IO — `Tsukino.Core/include/Tsukino/Core/IO/CerealOptional.hpp`

| シグネチャ | 説明 |
|---|---|
| `template <class Archive, class T> bool LoadOptional(Archive &archive, const char *name, T &value)` |  |

### Tsukino::IO — `Tsukino.Core/include/Tsukino/Core/IO/JsonFile.hpp`

| シグネチャ | 説明 |
|---|---|
| `template <class T> bool TryLoadJson(const std::string &path, const char *key, T &out)` |  |
| `template <class T> bool TrySaveJson(const std::string &path, const char *key, const T &value)` |  |

### Tsukino::Input — `Tsukino.Core/include/Tsukino/Core/Input/InputActionMap.hpp`

| シグネチャ | 説明 |
|---|---|
| `std::string GetBindingDisplayName(const InputBinding &binding, InputDevice device)` | 割り当てを画面に出すときの表記を得ます。 |
| `bool IsBindingDown(const InputSystem &input, const InputBinding &binding)` | 割り当てのキーまたはボタンが押されているかを返します（押しっぱなし）。 |
| `bool IsBindingPressed(const InputSystem &input, const InputBinding &binding)` | 割り当てのキーまたはボタンが押された瞬間かを返します。 |
| `bool IsBindingReleased(const InputSystem &input, const InputBinding &binding)` | 割り当てのキーまたはボタンが離された瞬間かを返します。 |
| `hlslpp::float2 ReadAxis(const InputSystem &input, const InputAxisBinding &binding)` | 2軸の入力を読みます。 |

### Tsukino::Input — `Tsukino.Core/include/Tsukino/Core/Input/InputNames.hpp`

| シグネチャ | 説明 |
|---|---|
| `GamepadButton GamepadButtonFromName(std::string_view name)` | 設定ファイルの綴りからゲームパッドのボタンを得ます。 |
| `const char * GetGamepadButtonDisplayName(GamepadButton button)` | ゲームパッドのボタンを画面に出すときの表記を得ます。 |
| `const char * GetKeyDisplayName(KeyCode code)` | キーを画面に出すときの表記を得ます。 |
| `KeyCode KeyCodeFromName(std::string_view name)` | 設定ファイルの綴りからキーコードを得ます。 |

## 名前空間の定数・変数

### Tsukino::Core::Math::Easing — `Tsukino.Core/include/Tsukino/Core/Math/Easing.hpp`

| シグネチャ | 説明 |
|---|---|
| `float kDefaultOvershoot = 1.70158f` | OutBack の既定の行き過ぎの強さです。 |

### Tsukino::Core::Math::Spring — `Tsukino.Core/include/Tsukino/Core/Math/Spring.hpp`

| シグネチャ | 説明 |
|---|---|
| `float kTwoPi = 6.28318531f` | 円周率の2倍（Hz から角周波数への変換に使う） |

### Tsukino::Core::Math — `Tsukino.Core/include/Tsukino/Core/Math/MathHelper.hpp`

| シグネチャ | 説明 |
|---|---|
| `float Deg2Rad = PI / 180.0f` |  |
| `float HALF_PI = PI * 0.5f` |  |
| `float PI = 3.1415926535f` |  |
| `float Rad2Deg = 180.0f / PI` |  |
| `float TWO_PI = PI * 2.0f` |  |

### Tsukino::Core::Utf8 — `Tsukino.Core/include/Tsukino/Core/Utf8.hpp`

| シグネチャ | 説明 |
|---|---|
| `char32_t kReplacementCharacter = 0xFFFD` | 読めなかったバイト列の代わりに返す文字（U+FFFD） |

### Tsukino::Input — `Tsukino.Core/include/Tsukino/Core/Input/GamepadCodes.hpp`

| シグネチャ | 説明 |
|---|---|
| `u32 kGamepadButtonCount = static_cast<u32>(GamepadButton::Count)` | ボタンの総数です。 |
| `u32 kMaxGamepads = 4` | 同時に扱えるゲームパッドの最大数です（XInput の上限）。 |
