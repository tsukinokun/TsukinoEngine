//--------------------------------------------------------------
//! @file	Window.cpp
//! @brief  ウィンドウ管理クラスの実装
//! @author 山﨑愛
//--------------------------------------------------------------
#include "Tsukino/Core/Window.h"
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
        // ウィンドウが存在する場合は破棄
        if(m_hWnd) {
            DestroyWindow(m_hWnd);    // ウィンドウを破棄
            m_hWnd = nullptr;         // ハンドルをリセット
        }
    }

    //--------------------------------------------------------------
    //! @brief ウィンドウ生成
    //--------------------------------------------------------------
    bool Window::Create(const std::string& title, int width, int height) {
        m_width  = width;     // ウィンドウの幅を保存
        m_height = height;    // ウィンドウの高さを保存

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
        m_hWnd = CreateWindowEx(
            0, wc.lpszClassName, wTitle.c_str(), WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, width, height, nullptr, nullptr, wc.hInstance, nullptr);

        //--------------------------------------------------------------
        // ウィンドウの作成に失敗した場合はエラーメッセージを表示して終了
        //--------------------------------------------------------------
        if(!m_hWnd) {
            MessageBox(nullptr, TEXT("ウィンドウの作成に失敗しました"), TEXT("Error"), MB_OK);
            return false;
        }

        ShowWindow(m_hWnd, SW_SHOW);    // ウィンドウを表示
        UpdateWindow(m_hWnd);           // ウィンドウを更新して描画を開始

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
            DispatchMessage(&msg);     // メッセージをウィンドウプロシージャに送る
        }
        // メッセージの処理が完了したら true を返す（アプリケーションは継続）
        return true;
    }

    //--------------------------------------------------------------
    //! @brief Win32 ウィンドウプロシージャ
    //--------------------------------------------------------------
    LRESULT CALLBACK Window::WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        // メッセージコードに応じて処理を分岐
        switch(msg) {
            // ウィンドウが閉じられたときの処理
        case WM_DESTROY:
            // ウィンドウが破棄されたときにアプリケーションを終了する
            PostQuitMessage(0);
            return 0;

        default:
            // デフォルトのメッセージ処理を行う
            return DefWindowProc(hwnd, msg, wParam, lParam);
        }
    }
}    // namespace Tsukino::Core
