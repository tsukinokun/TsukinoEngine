package jp.tsukino.engine;

import org.libsdl.app.SDLActivity;

/**
 * TsukinoEngine のアプリの Activity。
 * SDL はアプリの共有ライブラリ（libmain.so）へ静的にリンクしてあるので、読み込むのは main だけ。
 */
public class TsukinoActivity extends SDLActivity {
    @Override
    protected String[] getLibraries() {
        return new String[] { "main" };
    }
}
