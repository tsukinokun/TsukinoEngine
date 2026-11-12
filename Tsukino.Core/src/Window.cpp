//--------------------------------------------------------------
//! @file	Window.cpp
//! @brief  ウィンドウ管理クラスの実装
//! @author 山﨑愛
//--------------------------------------------------------------
#include "Tsukino/Core/Window.hpp"

#include <SDL3/SDL.h>

// SDL_SetMainReady() のためだけに取り込む（main を SDL_main に置き換えさせない）
#define SDL_MAIN_HANDLED
#include <SDL3/SDL_main.h>

#include <cmath>

#if defined(_WIN32)
#    include <Tsukino/Core/WindowsLean.hpp>
#    include <dwmapi.h>
#endif

using Tsukino::Input::InputEvent;
using Tsukino::Input::InputEventType;
using Tsukino::Input::KeyCode;

//--------------------------------------------------------------
// SDL のキーから KeyCode への変換
//--------------------------------------------------------------
namespace {
    //--------------------------------------------------------------
    //! SDL のキーを、エンジンの KeyCode（Windows の仮想キーコードと同じ値）へ変換します。
    //! @param  [in] key      キーボード配列に従ったキー（文字キーの判定に使う）
    //! @param  [in] scancode キーの物理的な位置（文字以外のキーの判定に使う）
    //! @return 対応する KeyCode。対応するものが無ければ KeyCode::None
    //! @note   文字と数字は配列に従ったキーで見る（Windows の仮想キーコードと同じく、
    //!         AZERTY 配列なら A の刻印のキーが A になる）。
    //--------------------------------------------------------------
    KeyCode ToKeyCode(SDL_Keycode key, SDL_Scancode scancode) {
        if(key >= SDLK_A && key <= SDLK_Z) {
            return static_cast<KeyCode>(0x41 + (key - SDLK_A));
        }
        if(key >= SDLK_0 && key <= SDLK_9) {
            return static_cast<KeyCode>(0x30 + (key - SDLK_0));
        }
        if(scancode >= SDL_SCANCODE_F1 && scancode <= SDL_SCANCODE_F12) {
            return static_cast<KeyCode>(0x70 + (scancode - SDL_SCANCODE_F1));
        }
        if(scancode >= SDL_SCANCODE_KP_1 && scancode <= SDL_SCANCODE_KP_9) {
            return static_cast<KeyCode>(0x61 + (scancode - SDL_SCANCODE_KP_1));    // VK_NUMPAD1 〜
        }

        switch(scancode) {
        case SDL_SCANCODE_RETURN:
        case SDL_SCANCODE_KP_ENTER: return KeyCode::Enter;
        case SDL_SCANCODE_ESCAPE: return KeyCode::Escape;
        case SDL_SCANCODE_BACKSPACE: return KeyCode::Backspace;
        case SDL_SCANCODE_TAB: return KeyCode::Tab;
        case SDL_SCANCODE_SPACE: return KeyCode::Space;
        case SDL_SCANCODE_LEFT: return KeyCode::Left;
        case SDL_SCANCODE_RIGHT: return KeyCode::Right;
        case SDL_SCANCODE_UP: return KeyCode::Up;
        case SDL_SCANCODE_DOWN: return KeyCode::Down;
        case SDL_SCANCODE_PAGEUP: return KeyCode::PageUp;
        case SDL_SCANCODE_PAGEDOWN: return KeyCode::PageDown;
        case SDL_SCANCODE_HOME: return KeyCode::Home;
        case SDL_SCANCODE_END: return KeyCode::End;
        case SDL_SCANCODE_LSHIFT:
        case SDL_SCANCODE_RSHIFT: return KeyCode::Shift;
        case SDL_SCANCODE_LCTRL:
        case SDL_SCANCODE_RCTRL: return KeyCode::Control;
        case SDL_SCANCODE_LALT:
        case SDL_SCANCODE_RALT: return KeyCode::Alt;
        case SDL_SCANCODE_INSERT: return static_cast<KeyCode>(0x2D);       // VK_INSERT
        case SDL_SCANCODE_DELETE: return static_cast<KeyCode>(0x2E);       // VK_DELETE
        case SDL_SCANCODE_CAPSLOCK: return static_cast<KeyCode>(0x14);     // VK_CAPITAL
        case SDL_SCANCODE_KP_0: return static_cast<KeyCode>(0x60);         // VK_NUMPAD0
        default: return KeyCode::None;
        }
    }

    //--------------------------------------------------------------
    //! SDL のマウスボタンを、エンジンの KeyCode へ変換します。
    //! @param  [in] button SDL のマウスボタン（SDL_BUTTON_LEFT など）
    //! @return 対応する KeyCode。対応するものが無ければ KeyCode::None
    //--------------------------------------------------------------
    KeyCode ToMouseKeyCode(Uint8 button) {
        switch(button) {
        case SDL_BUTTON_LEFT: return KeyCode::LButton;
        case SDL_BUTTON_RIGHT: return KeyCode::RButton;
        case SDL_BUTTON_MIDDLE: return KeyCode::MButton;
        default: return KeyCode::None;
        }
    }
}    // namespace

#if defined(_WIN32)
//--------------------------------------------------------------
// フックの定義（ClickThrough スタイル用。Windows のみ）
//--------------------------------------------------------------
namespace Tsukino::Core {
    static HHOOK   g_mouseHook = nullptr;
    static HHOOK   g_kbHook    = nullptr;
    static Window* g_instance  = nullptr;

    // exe に埋め込むアイコンのリソース名（Tools/premake/tsukino.lua の tsukino_icon() と揃える）
    static constexpr const TCHAR* kAppIconResourceName = TEXT("TSUKINO_APP_ICON");

    //--------------------------------------------------------------
    //! フックで受け取った入力をウィンドウのキューへ積みます。
    //! @param  [in] event 積む入力
    //! @note   フック内では通知先を直接呼ばず、キューへ積むだけにする。
    //!         直接呼ぶと、その先（InputSystemやゲームロジック）にブレークポイントを
    //!         置いた瞬間、同期呼び出しであるWH_MOUSE_LL/WH_KEYBOARD_LLの性質上、
    //!         OS全体のマウス/キーボード入力が止まってしまうため。
    //--------------------------------------------------------------
    static void DispatchInput(const InputEvent& event) {
        if(g_instance) {
            g_instance->EnqueueInput(event);
        }
    }

    //--------------------------------------------------------------
    //! キーボードの低レベルフックです。
    //! @param  [in] nCode  フックの処理コード
    //! @param  [in] wParam キーのメッセージの種類
    //! @param  [in] lParam KBDLLHOOKSTRUCT へのポインタ
    //! @return 次のフックの戻り値
    //--------------------------------------------------------------
    static LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
        if(nCode == HC_ACTION) {
            const KBDLLHOOKSTRUCT* pKbd = reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);

            InputEvent event;
            event.type = (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) ? InputEventType::KeyDown : InputEventType::KeyUp;
            event.key  = static_cast<KeyCode>(pKbd->vkCode);    // KeyCode は仮想キーコードと同じ値
            DispatchInput(event);
        }
        return CallNextHookEx(g_kbHook, nCode, wParam, lParam);
    }

    //--------------------------------------------------------------
    //! マウスの低レベルフックです。
    //! @param  [in] nCode  フックの処理コード
    //! @param  [in] wParam マウスのメッセージの種類
    //! @param  [in] lParam MSLLHOOKSTRUCT へのポインタ
    //! @return 次のフックの戻り値
    //--------------------------------------------------------------
    static LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam) {
        if(nCode == HC_ACTION) {
            const MSLLHOOKSTRUCT* pMouse = reinterpret_cast<MSLLHOOKSTRUCT*>(lParam);

            // 座標はスクリーン座標のまま渡す（ClickThrough はデスクトップ全体を覆う前提）
            InputEvent event;
            event.x = pMouse->pt.x;
            event.y = pMouse->pt.y;

            switch(wParam) {
            case WM_MOUSEMOVE: event.type = InputEventType::MouseMove; break;
            case WM_LBUTTONDOWN: event.type = InputEventType::KeyDown; event.key = KeyCode::LButton; break;
            case WM_LBUTTONUP: event.type = InputEventType::KeyUp; event.key = KeyCode::LButton; break;
            case WM_RBUTTONDOWN: event.type = InputEventType::KeyDown; event.key = KeyCode::RButton; break;
            case WM_RBUTTONUP: event.type = InputEventType::KeyUp; event.key = KeyCode::RButton; break;
            case WM_MBUTTONDOWN: event.type = InputEventType::KeyDown; event.key = KeyCode::MButton; break;
            case WM_MBUTTONUP: event.type = InputEventType::KeyUp; event.key = KeyCode::MButton; break;
            case WM_MOUSEWHEEL:
                event.type  = InputEventType::MouseWheel;
                event.wheel = static_cast<float>(static_cast<short>(HIWORD(pMouse->mouseData))) / static_cast<float>(WHEEL_DELTA);
                break;
            default: return CallNextHookEx(g_mouseHook, nCode, wParam, lParam);
            }
            DispatchInput(event);
        }
        return CallNextHookEx(g_mouseHook, nCode, wParam, lParam);
    }

    //--------------------------------------------------------------
    //! 低レベルフックを仕掛けます。
    //--------------------------------------------------------------
    static void InstallHooks() {
        g_mouseHook = SetWindowsHookEx(WH_MOUSE_LL, MouseProc, GetModuleHandle(nullptr), 0);
        g_kbHook    = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, GetModuleHandle(nullptr), 0);
    }

    //--------------------------------------------------------------
    //! 低レベルフックを外します。
    //--------------------------------------------------------------
    static void UninstallHooks() {
        if(g_mouseHook) {
            UnhookWindowsHookEx(g_mouseHook);
            g_mouseHook = nullptr;
        }
        if(g_kbHook) {
            UnhookWindowsHookEx(g_kbHook);
            g_kbHook = nullptr;
        }
    }
}    // namespace Tsukino::Core
#endif

// 名前空間 : Tsukino::Core
namespace Tsukino::Core {
    //--------------------------------------------------------------
    //! @brief コンストラクタ
    //--------------------------------------------------------------
    Window::Window() = default;

    //--------------------------------------------------------------
    //! @brief デストラクタ
    //--------------------------------------------------------------
    Window::~Window() {
#if defined(_WIN32)
        UninstallHooks();    // 確実にアンフック
        if(g_instance == this) {
            g_instance = nullptr;
        }
#endif

        //--------------------------------------------------------------
        // ウィンドウの破棄より前に通知先を必ず切る。
        // 通知先は一般に Window より先に破棄されるオブジェクトを掴んでいるため、
        // 破棄の途中で届いた通知から解放済みオブジェクトを呼び出さないようにする。
        //--------------------------------------------------------------
        m_inputCallback     = nullptr;
        m_resizeCallback    = nullptr;
        m_focusLostCallback = nullptr;
        m_hitTestCallback   = nullptr;

        if(m_window) {
            SDL_DestroyWindow(m_window);
            m_window = nullptr;
            SDL_QuitSubSystem(SDL_INIT_VIDEO);
        }

#if defined(_WIN32)
        // LoadImage で読んだアイコンは共有されないので自分で解放する（ウィンドウ破棄の後）
        if(m_icon) {
            DestroyIcon(static_cast<HICON>(m_icon));
            m_icon = nullptr;
        }
        if(m_iconSmall) {
            DestroyIcon(static_cast<HICON>(m_iconSmall));
            m_iconSmall = nullptr;
        }
#endif
    }

    //--------------------------------------------------------------
    //! @brief ウィンドウ生成
    //--------------------------------------------------------------
    bool Window::Create(const std::string& title, int width, int height, WindowStyle style) {
        m_style = style;

        //--------------------------------------------------------------
        // SDL の初期化
        //
        // main は自前（WinMain など）なので、SDL_main を使わないことを先に伝える。
        // IME の変換中の文字列はゲーム側で描く（OS 標準の変換窓は出さない。候補の窓は OS に任せる）
        //--------------------------------------------------------------
        SDL_SetMainReady();
        SDL_SetHint(SDL_HINT_IME_IMPLEMENTED_UI, "composition");
        if(!SDL_InitSubSystem(SDL_INIT_VIDEO)) {
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", SDL_GetError(), nullptr);
            return false;
        }

        //--------------------------------------------------------------
        // スタイルに応じたフラグ
        //--------------------------------------------------------------
        SDL_WindowFlags flags = SDL_WINDOW_HIGH_PIXEL_DENSITY;
        switch(style) {
        case WindowStyle::Default: flags |= SDL_WINDOW_RESIZABLE; break;
        case WindowStyle::Popup: flags |= SDL_WINDOW_BORDERLESS; break;
        case WindowStyle::ClickThrough: flags |= SDL_WINDOW_BORDERLESS | SDL_WINDOW_ALWAYS_ON_TOP; break;
        }

        //--------------------------------------------------------------
        // ウィンドウ作成
        //--------------------------------------------------------------
        m_window = SDL_CreateWindow(title.c_str(), width, height, flags);
        if(!m_window) {
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Failed to create the window.", nullptr);
            SDL_QuitSubSystem(SDL_INIT_VIDEO);
            return false;
        }

        // 描画に使う大きさは、画面の拡大率を掛けたピクセル数
        SDL_GetWindowSizeInPixels(m_window, &m_width, &m_height);

        ApplyEmbeddedIcon();

        // ClickThroughなら、はじめから最前面表示にする
        if(style == WindowStyle::ClickThrough) {
            ApplyClickThroughStyle();
            m_isTopmost = true;
        } else {
            m_isTopmost = false;
        }

        EnableHooksIfClickThrough();

        // ウィンドウの作成に成功した場合は true を返す
        return true;
    }

    //--------------------------------------------------------------
    //! @brief メッセージ処理
    //--------------------------------------------------------------
    bool Window::ProcessMessages() {
        // ウィンドウ座標（OS の拡大率で縮んだ座標）から描画のピクセルへの倍率
        const float density = m_window ? SDL_GetWindowPixelDensity(m_window) : 1.0f;
        const auto  toPixel = [density](float value) { return static_cast<i32>(std::lround(value * density)); };

        SDL_Event sdlEvent;
        while(SDL_PollEvent(&sdlEvent)) {
            InputEvent event;

            switch(sdlEvent.type) {
            //--------------------------------------------------------------
            // 終了
            //--------------------------------------------------------------
            case SDL_EVENT_QUIT:
            case SDL_EVENT_WINDOW_CLOSE_REQUESTED: return false;

            //--------------------------------------------------------------
            // キーボード
            //--------------------------------------------------------------
            case SDL_EVENT_KEY_DOWN:
            case SDL_EVENT_KEY_UP:
                event.type   = (sdlEvent.type == SDL_EVENT_KEY_DOWN) ? InputEventType::KeyDown : InputEventType::KeyUp;
                event.key    = ToKeyCode(sdlEvent.key.key, sdlEvent.key.scancode);
                event.repeat = sdlEvent.key.repeat;
                if(event.key != KeyCode::None) {
                    NotifyInput(event);
                }
                break;

            //--------------------------------------------------------------
            // マウス（タッチから SDL が作ったものも含む）
            //--------------------------------------------------------------
            case SDL_EVENT_MOUSE_MOTION:
                event.type      = InputEventType::MouseMove;
                event.x         = toPixel(sdlEvent.motion.x);
                event.y         = toPixel(sdlEvent.motion.y);
                event.fromTouch = (sdlEvent.motion.which == SDL_TOUCH_MOUSEID);
                NotifyInput(event);
                break;

            case SDL_EVENT_MOUSE_BUTTON_DOWN:
            case SDL_EVENT_MOUSE_BUTTON_UP:
                event.type      = (sdlEvent.type == SDL_EVENT_MOUSE_BUTTON_DOWN) ? InputEventType::KeyDown : InputEventType::KeyUp;
                event.key       = ToMouseKeyCode(sdlEvent.button.button);
                event.x         = toPixel(sdlEvent.button.x);
                event.y         = toPixel(sdlEvent.button.y);
                event.fromTouch = (sdlEvent.button.which == SDL_TOUCH_MOUSEID);
                if(event.key != KeyCode::None) {
                    NotifyInput(event);
                }
                break;

            case SDL_EVENT_MOUSE_WHEEL:
                event.type  = InputEventType::MouseWheel;
                event.wheel = sdlEvent.wheel.y;
                // OS の設定で向きが逆になっているときは、奥へ回すと正にそろえる
                if(sdlEvent.wheel.direction == SDL_MOUSEWHEEL_FLIPPED) {
                    event.wheel = -event.wheel;
                }
                NotifyInput(event);
                break;

            //--------------------------------------------------------------
            // 文字入力と IME
            //--------------------------------------------------------------
            case SDL_EVENT_TEXT_INPUT:
                event.type = InputEventType::TextInput;
                event.text = sdlEvent.text.text ? sdlEvent.text.text : "";
                NotifyInput(event);
                break;

            case SDL_EVENT_TEXT_EDITING:
                event.type = InputEventType::TextEditing;
                event.text = sdlEvent.edit.text ? sdlEvent.edit.text : "";
                NotifyInput(event);
                break;

            //--------------------------------------------------------------
            // タッチ（座標は 0〜1 で届くので、クライアント領域のピクセルへ直す）
            //--------------------------------------------------------------
            case SDL_EVENT_FINGER_DOWN:
            case SDL_EVENT_FINGER_MOTION:
            case SDL_EVENT_FINGER_UP:
            case SDL_EVENT_FINGER_CANCELED:
                event.type = (sdlEvent.type == SDL_EVENT_FINGER_DOWN)     ? InputEventType::TouchDown
                             : (sdlEvent.type == SDL_EVENT_FINGER_MOTION) ? InputEventType::TouchMove
                                                                          : InputEventType::TouchUp;
                event.fingerId = static_cast<u64>(sdlEvent.tfinger.fingerID);
                event.x        = static_cast<i32>(std::lround(sdlEvent.tfinger.x * static_cast<float>(m_width)));
                event.y        = static_cast<i32>(std::lround(sdlEvent.tfinger.y * static_cast<float>(m_height)));
                NotifyInput(event);
                break;

            //--------------------------------------------------------------
            // クライアント領域のサイズ変更をレンダラーへ伝える
            //
            // これを行わないとスワップチェインのバックバッファが元のサイズのまま
            // 残り、映像が引き伸ばされてアスペクト比が崩れる。
            // 最小化時の 0 サイズは作れないので無視する
            //--------------------------------------------------------------
            case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
                if(sdlEvent.window.data1 > 0 && sdlEvent.window.data2 > 0) {
                    m_width  = sdlEvent.window.data1;
                    m_height = sdlEvent.window.data2;
                    if(m_resizeCallback) {
                        m_resizeCallback(m_width, m_height);
                    }
                }
                break;

            //--------------------------------------------------------------
            // フォーカス
            //
            // キーを押したまま Alt+Tab するとキーを離した通知は移動先のウィンドウへ
            // 届くため、こちらには来ない。クリアしないと押されっぱなしとして残り続ける
            //--------------------------------------------------------------
            case SDL_EVENT_WINDOW_FOCUS_LOST:
                if(m_updateMode == UpdateMode::ActiveOnly) {
                    UpdateHookState(false);
                }
                if(m_focusLostCallback) {
                    m_focusLostCallback();
                }
                break;

            case SDL_EVENT_WINDOW_FOCUS_GAINED:
                if(m_updateMode == UpdateMode::ActiveOnly) {
                    UpdateHookState(true);
                }
                break;

            default: break;
            }
        }

        // 低レベルフックが積んだ入力をこのタイミングでまとめて処理する
        DispatchQueuedInput();

        // メッセージの処理が完了したら true を返す（アプリケーションは継続）
        return true;
    }

    //--------------------------------------------------------------
    //! @brief OS のウィンドウハンドルを取得する関数
    //--------------------------------------------------------------
    void* Window::GetNativeHandle() const {
#if defined(_WIN32)
        if(m_window) {
            return SDL_GetPointerProperty(SDL_GetWindowProperties(m_window), SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
        }
#endif
        return nullptr;
    }

    //--------------------------------------------------------------
    //! @brief 入力を通知先へ渡す関数
    //--------------------------------------------------------------
    void Window::NotifyInput(const InputEvent& event) {
        if(m_inputCallback) {
            m_inputCallback(event);
        }
    }

    //--------------------------------------------------------------
    //! @brief 低レベルフックから呼ばれる、入力をキューへ積むだけの関数
    //--------------------------------------------------------------
    void Window::EnqueueInput(const InputEvent& event) {
        m_inputQueue.push_back(event);
    }

    //--------------------------------------------------------------
    //! @brief キューに溜まった入力をまとめて処理する関数
    //--------------------------------------------------------------
    void Window::DispatchQueuedInput() {
        if(m_inputQueue.empty())
            return;

        // 処理中（通知先のゲームロジック等）に EnqueueInput が再入されても、
        // 走査中のベクタを壊さないようにローカルへ退避してから処理する
        std::vector<InputEvent> events;
        events.swap(m_inputQueue);

        for(const InputEvent& event : events) {
            NotifyInput(event);

            if(event.type == InputEventType::MouseMove) {
                UpdateClickThroughFromHitTest(event.x, event.y);
            }
        }
    }

    //--------------------------------------------------------------
    //! @brief ClickThrough スタイルの Windows 固有の設定を行う関数
    //--------------------------------------------------------------
    void Window::ApplyClickThroughStyle() {
#if defined(_WIN32)
        HWND hwnd = static_cast<HWND>(GetNativeHandle());
        if(!hwnd)
            return;

        // クリックを背面へ通すレイヤードウィンドウにする
        LONG_PTR exStyle = ::GetWindowLongPtr(hwnd, GWL_EXSTYLE);
        ::SetWindowLongPtr(hwnd, GWL_EXSTYLE, exStyle | WS_EX_LAYERED | WS_EX_TRANSPARENT);

        // レイヤードウィンドウとしてアルファを使用可能にする（透明度255 = 不透明で初期化）
        SetLayeredWindowAttributes(hwnd, 0, 255, LWA_ALPHA);

        // DWM に対して、クライアント領域全体を「透明なフレーム」として拡張するよう指示
        MARGINS margins = {-1, -1, -1, -1};    // 全体に適用する特殊な値
        DwmExtendFrameIntoClientArea(hwnd, &margins);
#endif
    }

    //--------------------------------------------------------------
    //! @brief exe に埋め込まれたアイコンをウィンドウへ設定する関数
    //--------------------------------------------------------------
    void Window::ApplyEmbeddedIcon() {
#if defined(_WIN32)
        HWND hwnd = static_cast<HWND>(GetNativeHandle());
        if(!hwnd)
            return;

        //--------------------------------------------------------------
        // exe に埋め込まれたアイコンの読み込み（tsukino_icon() で埋め込んだもの）
        //
        // タイトルバー・Alt+Tab 用の大きいアイコンと、タスクバー・左上用の
        // 小さいアイコンを、それぞれ表示サイズに合わせて .ico から選ばせる。
        // 埋め込まれていなければ何もせず、SDL の既定のアイコンのままにする
        //--------------------------------------------------------------
        HINSTANCE instance = GetModuleHandle(nullptr);
        HICON     icon     = static_cast<HICON>(LoadImage(instance, kAppIconResourceName, IMAGE_ICON, GetSystemMetrics(SM_CXICON),
                                                          GetSystemMetrics(SM_CYICON), LR_DEFAULTCOLOR));
        HICON iconSmall = static_cast<HICON>(LoadImage(instance, kAppIconResourceName, IMAGE_ICON, GetSystemMetrics(SM_CXSMICON),
                                                       GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR));
        if(icon) {
            SendMessage(hwnd, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(icon));
        }
        if(iconSmall) {
            SendMessage(hwnd, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(iconSmall));
        }
        m_icon      = icon;
        m_iconSmall = iconSmall;
#endif
    }

    //--------------------------------------------------------------
    //! @brief クリック透過（WS_EX_TRANSPARENT）を動的に切り替える関数
    //--------------------------------------------------------------
    void Window::SetClickThroughEnabled(bool enabled) {
        if(m_style != WindowStyle::ClickThrough)
            return;
        if(!m_window || m_clickThroughEnabled == enabled)
            return;

        m_clickThroughEnabled = enabled;

#if defined(_WIN32)
        HWND     hwnd    = static_cast<HWND>(GetNativeHandle());
        LONG_PTR exStyle = ::GetWindowLongPtr(hwnd, GWL_EXSTYLE);
        if(enabled) {
            exStyle |= WS_EX_TRANSPARENT;
        } else {
            exStyle &= ~WS_EX_TRANSPARENT;
        }
        ::SetWindowLongPtr(hwnd, GWL_EXSTYLE, exStyle);
#endif
    }

    //--------------------------------------------------------------
    //! @brief 登録済みの HitTestCallback を使って
    //!        クリック透過の有効/無効を更新する関数
    //--------------------------------------------------------------
    void Window::UpdateClickThroughFromHitTest(int screenX, int screenY) {
        if(!m_hitTestCallback)
            return;

        bool wantsInput = m_hitTestCallback(screenX, screenY);
        SetClickThroughEnabled(!wantsInput);
    }

    //--------------------------------------------------------------
    //! @brief 動的に最前面表示を切り替える関数
    //--------------------------------------------------------------
    void Window::SetTopmost(bool enable) {
        m_isTopmost = enable;

        if(!m_window)
            return;

        SDL_SetWindowAlwaysOnTop(m_window, enable);
    }

    //------------------------------------------------------------------
    //! @brief フックの管理をウィンドウ側に寄せるためのロジック変更
    //------------------------------------------------------------------
    void Window::SetUpdateMode(UpdateMode mode) {
        m_updateMode = mode;

        //--------------------------------------------------------------
        // AlwaysResident なら常にフックを掛ける。
        // ActiveOnly なら自分が前面にあるときだけ掛ける。
        //--------------------------------------------------------------
        const bool shouldInstall = (m_updateMode == UpdateMode::AlwaysResident) || IsFocused();

        UpdateHookState(shouldInstall);
    }

    //--------------------------------------------------------------
    //! @brief クリックスルーの場合に、フックを有効化する関数
    //--------------------------------------------------------------
    void Window::EnableHooksIfClickThrough() {
#if defined(_WIN32)
        if(m_style == WindowStyle::ClickThrough) {
            g_instance = this;
            InstallHooks();
        }
#endif
    }

    //------------------------------------------------------------------
    //! @brief フックの状態を更新する関数
    //------------------------------------------------------------------
    void Window::UpdateHookState(bool shouldInstall) {
        if(m_style != WindowStyle::ClickThrough)
            return;

#if defined(_WIN32)
        if(shouldInstall) {
            if(!g_mouseHook && !g_kbHook) {    // フックが未登録の場合のみ
                g_instance = this;
                InstallHooks();
            }
        } else {
            if(g_mouseHook || g_kbHook) {    // フックが登録されている場合のみ
                UninstallHooks();
            }
            // フックが外れている間はヒットテストが走らないため、
            // クリック透過を「有効」に戻しておく（奪いっぱなしを防ぐ）
            SetClickThroughEnabled(true);
        }
#else
        (void)shouldInstall;
#endif
    }

    //--------------------------------------------------------------
    //! @brief 全画面表示の切り替え
    //--------------------------------------------------------------
    void Window::SetFullscreen(bool enable) {
        if(!m_window)
            return;
        if(m_isFullscreen == enable)
            return;

        m_isFullscreen = enable;

        if(m_style == WindowStyle::ClickThrough) {
            // ClickThroughの場合は「デスクトップ全体を覆う」挙動にする（最前面のまま）
            if(enable) {
                SDL_GetWindowPosition(m_window, &m_preFullscreenX, &m_preFullscreenY);
                SDL_GetWindowSize(m_window, &m_preFullscreenWidth, &m_preFullscreenHeight);

                SDL_Rect bounds{};
                if(SDL_GetDisplayBounds(SDL_GetDisplayForWindow(m_window), &bounds)) {
                    SDL_SetWindowPosition(m_window, bounds.x, bounds.y);
                    SDL_SetWindowSize(m_window, bounds.w, bounds.h);
                }
            } else {
                SDL_SetWindowPosition(m_window, m_preFullscreenX, m_preFullscreenY);
                SDL_SetWindowSize(m_window, m_preFullscreenWidth, m_preFullscreenHeight);
            }
        } else {
            // 通常のウィンドウは、デスクトップの解像度のまま枠なしで全画面にする
            SDL_SetWindowFullscreen(m_window, enable);
        }
    }

    //--------------------------------------------------------------
    //! @brief OSカーソルの表示/非表示を切り替える関数
    //--------------------------------------------------------------
    void Window::SetCursorVisible(bool visible) {
        if(m_cursorVisible == visible)
            return;

        m_cursorVisible = visible;
        if(visible) {
            SDL_ShowCursor();
        } else {
            SDL_HideCursor();
        }
    }

    //--------------------------------------------------------------
    //! @brief ウィンドウがフォアグラウンドかを取得する関数
    //--------------------------------------------------------------
    bool Window::IsFocused() const {
        return m_window && (SDL_GetWindowFlags(m_window) & SDL_WINDOW_INPUT_FOCUS) != 0;
    }

    //--------------------------------------------------------------
    //! @brief OSカーソルをクライアント領域の中央へ移動する関数
    //--------------------------------------------------------------
    void Window::CenterCursor() {
        if(!m_window)
            return;

        // SDL_WarpMouseInWindow はウィンドウ座標（拡大率で縮んだ座標）で受け取る
        int windowWidth = 0, windowHeight = 0;
        SDL_GetWindowSize(m_window, &windowWidth, &windowHeight);
        SDL_WarpMouseInWindow(m_window, static_cast<float>(windowWidth) * 0.5f, static_cast<float>(windowHeight) * 0.5f);
    }
}    // namespace Tsukino::Core
