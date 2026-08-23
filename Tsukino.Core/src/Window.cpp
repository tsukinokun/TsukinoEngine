//--------------------------------------------------------------
//! @file	Window.cpp
//! @brief  ウィンドウ管理クラスの実装
//! @author 山﨑愛
//--------------------------------------------------------------
#include "Tsukino/Core/Window.hpp"
#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")

//--------------------------------------------------------------
// フックの定義
//--------------------------------------------------------------
namespace Tsukino::Core {
    // Window.cpp 内の追加コード
    static HHOOK   g_mouseHook = nullptr;
    static HHOOK   g_kbHook    = nullptr;
    static Window* g_instance  = nullptr;

    // 共通の入力転送ロジック
    static void DispatchInput(UINT msg, WPARAM wp, LPARAM lp) {
        if(g_instance) {
            g_instance->InvokeCallback(msg, wp, lp);
        }
    }

    LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
        if(nCode == HC_ACTION) {
            KBDLLHOOKSTRUCT* pKbd = reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);
            UINT             msg  = (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) ? WM_KEYDOWN : WM_KEYUP;
            DispatchInput(msg, pKbd->vkCode, 0);
        }
        return CallNextHookEx(g_kbHook, nCode, wParam, lParam);
    }

    LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam) {
        if(nCode == HC_ACTION) {
            MSLLHOOKSTRUCT* pMouse = reinterpret_cast<MSLLHOOKSTRUCT*>(lParam);
            // マウス座標をLPARAMにパック
            LPARAM lp = MAKELPARAM(pMouse->pt.x, pMouse->pt.y);
            DispatchInput(static_cast<UINT>(wParam), 0, lp);
        }
        return CallNextHookEx(g_mouseHook, nCode, wParam, lParam);
    }

    //--------------------------------------------------------------
    // 実際にフックを仕掛ける関数
    //--------------------------------------------------------------
    void InstallHooks() {
        g_mouseHook = SetWindowsHookEx(WH_MOUSE_LL, MouseProc, GetModuleHandle(nullptr), 0);
        g_kbHook    = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, GetModuleHandle(nullptr), 0);
    }

    //--------------------------------------------------------------
    // 実際にフックを外す関数
    //--------------------------------------------------------------
    void UninstallHooks() {
        if(g_mouseHook) {
            UnhookWindowsHookEx(g_mouseHook);
            g_mouseHook = nullptr;
            OutputDebugString(L"[Debug] Mouse Hook Uninstalled\n");   
        }
        if(g_kbHook) {
            UnhookWindowsHookEx(g_kbHook);
            g_kbHook = nullptr;
            OutputDebugString(L"[Debug] KB Hook Uninstalled\n");    
        }
    }
}    // namespace Tsukino::Core

// 名前空間 : Tsukino::Core
namespace Tsukino::Core {
    //--------------------------------------------------------------
    //! @brief コンストラクタ
    //--------------------------------------------------------------
    Window::Window()
        : m_hWnd(nullptr)    // ウィンドウハンドルを初期化
        , m_width(0)         // ウィンドウの幅を初期化
        , m_height(0) {      // ウィンドウの高さを初期化
    }

    //--------------------------------------------------------------
    //! @brief デストラクタ
    //--------------------------------------------------------------
    Window::~Window() {
        UninstallHooks();    // 確実にアンフック

        //--------------------------------------------------------------
        // 【重要】DestroyWindow() へ入る前に通知先を必ず切る。
        //
        // DestroyWindow() は同期的に WM_ACTIVATEAPP / WM_KILLFOCUS / WM_SIZE /
        // WM_DESTROY を WindowProc へ送る。つまりデストラクタの途中で
        // WindowProc が再入する。
        // 通知先（m_callback / m_resizeCallback / m_focusLostCallback）は
        // 一般に Window より先に破棄されるオブジェクトを掴んでいるため、
        // ここを残したまま破棄すると解放済みオブジェクトを呼び出すことになる。
        //
        // 実際、終了時に 3 回に 1 回ほど
        // STATUS_FATAL_USER_CALLBACK_EXCEPTION (0xC000041D) で落ちていたのはこれが原因。
        // ウィンドウがアクティブなまま終了したときだけ WM_ACTIVATEAPP /
        // WM_KILLFOCUS が飛ぶため、再現が非決定的になっていた。
        //--------------------------------------------------------------
        m_callback          = nullptr;
        m_resizeCallback    = nullptr;
        m_focusLostCallback = nullptr;

        // ウィンドウが存在する場合は破棄
        if(m_hWnd) {
            // WindowProc 側から this へ辿れないようにしてから破棄する
            ::SetWindowLongPtr(m_hWnd, GWLP_USERDATA, 0);

            DestroyWindow(m_hWnd);    // ウィンドウを破棄
            m_hWnd = nullptr;         // ハンドルをリセット
        }
    }

    //--------------------------------------------------------------
    //! @brief ウィンドウ生成
    //--------------------------------------------------------------
    bool Window::Create(const std::string& title, int width, int height, WindowStyle style) {
        m_width  = width;     // ウィンドウの幅を保存
        m_height = height;    // ウィンドウの高さを保存
        m_style  = style;     // ウィンドウのスタイルを保存

        //--------------------------------------------------------------
        // ウィンドウクラスの登録
        //--------------------------------------------------------------
        WNDCLASSEX wc{};                                      // ウィンドウクラス構造体を初期化
        wc.cbSize        = sizeof(WNDCLASSEX);                // 構造体のサイズを設定
        wc.lpfnWndProc   = WindowProc;                        // ウィンドウプロシージャを設定
        wc.hInstance     = GetModuleHandle(nullptr);          // アプリケーションインスタンスを取得して設定
        wc.lpszClassName = TEXT("TsukinoWindowClass");        // ウィンドウクラス名を設定
        wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);    // デフォルトの矢印カーソルを設定

        //--------------------------------------------------------------
        // ウィンドウクラスの登録に失敗した場合はエラーメッセージを表示して終了
        //--------------------------------------------------------------
        if(!RegisterClassEx(&wc)) {
            MessageBox(nullptr, TEXT("ウィンドウクラスの登録に失敗しました"), TEXT("Error"), MB_OK);
            return false;
        }

        //--------------------------------------------------------------
        // マルチバイト → ワイド文字変換
        //--------------------------------------------------------------
        int          len = MultiByteToWideChar(CP_UTF8, 0, title.c_str(), -1, nullptr, 0);
        std::wstring wTitle(len, L'\0');
        MultiByteToWideChar(CP_UTF8, 0, title.c_str(), -1, &wTitle[0], len);

        //--------------------------------------------------------------
        // ウィンドウ作成
        //--------------------------------------------------------------
        // 使用するウィンドウのスタイルを定義
        DWORD dwStyle   = WS_OVERLAPPEDWINDOW;
        DWORD dwExStyle = 0;

        // スタイルに応じてフラグを切り替える
        if(style == WindowStyle::Popup) {
            dwStyle = WS_POPUP;
        } else if(style == WindowStyle::ClickThrough) {
            dwStyle   = WS_POPUP;
            dwExStyle = WS_EX_LAYERED | WS_EX_TRANSPARENT;
        }

        // 希望する描画領域（クライアント領域）のサイズを設定
        RECT rc = {0, 0, width, height};

        // 枠線やタイトルバーを含めた、ウィンドウ全体の正しいサイズを計算
        AdjustWindowRect(&rc, dwStyle, FALSE);

        // 計算された全体の幅と高さを算出
        int winWidth  = rc.right - rc.left;
        int winHeight = rc.bottom - rc.top;

        // m_width と m_height には、枠線を含まない「純粋なゲーム画面の解像度」を保存し直す
        m_width  = width;
        m_height = height;

        // 5. 計算した winWidth, winHeight を使ってウィンドウを生成
        m_hWnd = CreateWindowEx(dwExStyle,
                                wc.lpszClassName,
                                wTitle.c_str(),
                                dwStyle,    // 定義したスタイルを渡す
                                CW_USEDEFAULT,
                                CW_USEDEFAULT,
                                winWidth,     // 計算した「全体の幅」
                                winHeight,    // 計算した「全体の高さ」
                                nullptr,
                                nullptr,
                                wc.hInstance,
                                this);

        //--------------------------------------------------------------
        // ウィンドウの作成に失敗した場合はエラーメッセージを表示して終了
        //--------------------------------------------------------------
        if(!m_hWnd) {
            MessageBox(nullptr, TEXT("ウィンドウの作成に失敗しました"), TEXT("Error"), MB_OK);
            return false;
        }

        //--------------------------------------------------------------
        // ClickThrough スタイル時の DWM 透過設定
        //--------------------------------------------------------------
        if(style == WindowStyle::ClickThrough) {
            // レイヤードウィンドウとしてアルファを使用可能にする（透明度255 = 不透明で初期化）
            SetLayeredWindowAttributes(m_hWnd, 0, 255, LWA_ALPHA);

            // DWM に対して、クライアント領域全体を「透明なフレーム」として拡張するよう指示
            MARGINS margins = {-1, -1, -1, -1};    // 全体に適用する特殊な値
            DwmExtendFrameIntoClientArea(m_hWnd, &margins);
        }

        ShowWindow(m_hWnd, SW_SHOW);    // ウィンドウを表示
        UpdateWindow(m_hWnd);           // ウィンドウを更新して描画を開始

        // ClickThroughなら、はじめから最前面表示にする
        if(style == WindowStyle::ClickThrough) {
            SetTopmost(true);
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
        MSG msg{};    // メッセージ構造体を初期化

        //--------------------------------------------------------------
        // メッセージキューにメッセージがある限り処理を続ける
        //--------------------------------------------------------------

        while(PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            // WM_QUIT メッセージが来たらアプリケーションを終了する

            if(msg.message == WM_QUIT)

                return false;

            TranslateMessage(&msg);    // キーボードメッセージを文字メッセージに変換

            DispatchMessage(&msg);    // メッセージをウィンドウプロシージャに送る
        }
        // メッセージの処理が完了したら true を返す（アプリケーションは継続）
        return true;
    }

    //--------------------------------------------------------------
    //! @brief Win32 ウィンドウプロシージャ
    //--------------------------------------------------------------
    LRESULT CALLBACK Window::WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        Window* pWindow = nullptr;    // ウィンドウインスタンスへのポインタを格納する変数

        //--------------------------------------------------------------
        // インスタンスを覚えさせる
        //--------------------------------------------------------------
        if(msg == WM_NCCREATE) {
            // 作成時に渡した 'this' を取り出して、HWNDのメモリに保存する
            CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
            pWindow               = reinterpret_cast<Window*>(pCreate->lpCreateParams);
            SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pWindow));
        } else {
            // 保存しておいた 'this' を取り出す
            pWindow = reinterpret_cast<Window*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
        }

        //--------------------------------------------------------------
        // ウィンドウのアクティブ状態に応じてフックの有効/無効を切り替える
        //--------------------------------------------------------------
        if(pWindow && pWindow->m_updateMode == UpdateMode::ActiveOnly) {
            if(msg == WM_ACTIVATEAPP) {
                pWindow->UpdateHookState(wParam != 0);    // アクティブならインストール、そうでなければアンインストール
            }
        }

        //--------------------------------------------------------------
        // もし通知先 (m_callback) が登録されていれば、メッセージを投げる
        //--------------------------------------------------------------
        if(pWindow && pWindow->m_callback) {
            // 入力に関係するメッセージだけをフィルタリングして通知
            if((msg >= WM_KEYFIRST && msg <= WM_KEYLAST) || (msg >= WM_MOUSEFIRST && msg <= WM_MOUSELAST)) {
                pWindow->m_callback(msg, wParam, lParam);
            }
        }

        //--------------------------------------------------------------
        // クライアント領域のサイズ変更をレンダラーへ伝える
        //
        // これを行わないとスワップチェインのバックバッファが元のサイズのまま
        // 残り、映像が引き伸ばされてアスペクト比が崩れる。
        //--------------------------------------------------------------
        if(pWindow && msg == WM_SIZE) {
            const int newWidth  = static_cast<int>(LOWORD(lParam));
            const int newHeight = static_cast<int>(HIWORD(lParam));

            // 最小化時は 0x0 が来る。0 サイズのバッファは作れないので無視する
            if(wParam != SIZE_MINIMIZED && newWidth > 0 && newHeight > 0) {
                pWindow->m_width  = newWidth;
                pWindow->m_height = newHeight;

                if(pWindow->m_resizeCallback) {
                    pWindow->m_resizeCallback(newWidth, newHeight);
                }
            }
        }

        //--------------------------------------------------------------
        // フォーカスを失ったら入力状態をクリアさせる
        //
        // キーを押したまま Alt+Tab すると WM_KEYUP は移動先のウィンドウへ
        // 届くため、こちらには「離した」通知が来ない。
        // クリアしないとそのキーが押されっぱなしとして残り続ける。
        //--------------------------------------------------------------
        if(pWindow && pWindow->m_focusLostCallback) {
            const bool lostFocus = (msg == WM_KILLFOCUS) || (msg == WM_ACTIVATEAPP && wParam == FALSE);
            if(lostFocus) {
                pWindow->m_focusLostCallback();
            }
        }

        //--------------------------------------------------------------
        // ウィンドウが破棄されたときの処理
        //--------------------------------------------------------------
        if(msg == WM_DESTROY) {
            PostQuitMessage(0);
            return 0;
        }

        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    //--------------------------------------------------------------
    //! @brief 動的に最前面表示を切り替える関数
    //--------------------------------------------------------------
    void Window::SetTopmost(bool enable) {
        m_isTopmost = enable;

        if(!m_hWnd)
            return;

        HWND hWndInsertAfter = enable ? HWND_TOPMOST : HWND_NOTOPMOST;

        // 位置やサイズは変えず、Zオーダーだけを更新（アクティブ化させない）
        ::SetWindowPos(m_hWnd, hWndInsertAfter, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    }

    //------------------------------------------------------------------
    //! @brief フックの管理をウィンドウ側に寄せるためのロジック変更
    //------------------------------------------------------------------
    void Window::SetUpdateMode(UpdateMode mode) {
        m_updateMode = mode;

        //--------------------------------------------------------------
        // AlwaysResident なら常にフックを掛ける。
        // ActiveOnly なら自分が前面にあるときだけ掛ける。
        //
        // 以前は `m_updateMode != UpdateMode::ActiveOnly || true` と書かれており、
        // 右辺の true によって式全体が常に true になっていた（モード指定が無効だった）。
        //--------------------------------------------------------------
        const bool shouldInstall = (m_updateMode == UpdateMode::AlwaysResident) || (GetForegroundWindow() == m_hWnd);

        UpdateHookState(shouldInstall);
    }

    //--------------------------------------------------------------
    //! @brief フックからコールバックを呼び出すための公開メソッド   
    //--------------------------------------------------------------
    void Window::InvokeCallback(UINT msg, WPARAM wParam, LPARAM lParam) {
        if(m_callback) {
            m_callback(msg, wParam, lParam);
        }
    }

    //--------------------------------------------------------------
    //! @brief ライフサイクル管理
    //--------------------------------------------------------------
    void Window::EnableHooksIfClickThrough() {
        if(m_style == WindowStyle::ClickThrough) {
            g_instance = this;
            InstallHooks();
        }
    }

    //------------------------------------------------------------------
    //! @brief フック管理関数
    //------------------------------------------------------------------
    void Window::UpdateHookState(bool shouldInstall) {
        if(m_style != WindowStyle::ClickThrough)
            return;

        if(shouldInstall) {
            if(!g_mouseHook && !g_kbHook) {    // フックが未登録の場合のみ
                g_instance = this;
                InstallHooks();
            }
        } else {
            if(g_mouseHook || g_kbHook) {    // フックが登録されている場合のみ
                UninstallHooks();
            }
        }
    }

    //--------------------------------------------------------------
    //! @brief 全画面表示の切り替え
    //--------------------------------------------------------------
    void Window::SetFullscreen(bool enable) {
        if(!m_hWnd)
            return;
        if(m_isFullscreen == enable)
            return;

        m_isFullscreen = enable;

        if(m_style == WindowStyle::ClickThrough) {
            // ClickThroughの場合は「デスクトップ全体を覆う」挙動にする
            if(enable) {
                int screenW = GetSystemMetrics(SM_CXSCREEN);
                int screenH = GetSystemMetrics(SM_CYSCREEN);
                SetWindowPos(m_hWnd, HWND_TOPMOST, 0, 0, screenW, screenH, SWP_FRAMECHANGED | SWP_NOACTIVATE);
            } else {
                // 元のサイズ（または任意のデスクトップ固定サイズ）に戻す処理
                // 必要に応じて適切な位置・サイズを指定してください
                SetWindowPos(m_hWnd, HWND_TOPMOST, 0, 0, m_width, m_height, SWP_FRAMECHANGED | SWP_NOACTIVATE);
            }
        } else {
            // 通常のウィンドウモードの切り替え
            if(enable) {
                // 現在の位置とサイズを保存
                GetWindowRect(m_hWnd, &m_preFullscreenRect);

                // 枠なしにしてフルスクリーン化
                SetWindowLong(m_hWnd, GWL_STYLE, WS_POPUP | WS_VISIBLE);
                int screenW = GetSystemMetrics(SM_CXSCREEN);
                int screenH = GetSystemMetrics(SM_CYSCREEN);
                SetWindowPos(m_hWnd, HWND_TOP, 0, 0, screenW, screenH, SWP_FRAMECHANGED);
            } else {
                // 枠ありに戻す
                SetWindowLong(m_hWnd, GWL_STYLE, WS_OVERLAPPEDWINDOW | WS_VISIBLE);
                SetWindowPos(m_hWnd,
                             HWND_TOP,
                             m_preFullscreenRect.left,
                             m_preFullscreenRect.top,
                             m_preFullscreenRect.right - m_preFullscreenRect.left,
                             m_preFullscreenRect.bottom - m_preFullscreenRect.top,
                             SWP_FRAMECHANGED);
            }
        }
    }

    //--------------------------------------------------------------
    //! @brief OSカーソルの表示/非表示を切り替える関数
    //--------------------------------------------------------------
    void Window::SetCursorVisible(bool visible) {
        if(m_cursorVisible == visible)
            return;    // 状態が変わらないならShowCursorの参照カウントを崩さないよう何もしない

        m_cursorVisible = visible;
        ::ShowCursor(visible ? TRUE : FALSE);
    }

    //--------------------------------------------------------------
    //! @brief ウィンドウがフォアグラウンドかを取得する関数
    //--------------------------------------------------------------
    bool Window::IsFocused() const {
        return m_hWnd && GetForegroundWindow() == m_hWnd;
    }

    //--------------------------------------------------------------
    //! @brief OSカーソルをクライアント領域の中央へ移動する関数
    //--------------------------------------------------------------
    void Window::CenterCursor() {
        if(!m_hWnd)
            return;

        RECT rc{};
        GetClientRect(m_hWnd, &rc);

        POINT center{(rc.right - rc.left) / 2, (rc.bottom - rc.top) / 2};
        ClientToScreen(m_hWnd, &center);

        ::SetCursorPos(center.x, center.y);
    }
}    // namespace Tsukino::Core
