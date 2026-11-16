# 各 OS でのビルド

TsukinoEngine は CMake でビルドします（ルートの `CMakeLists.txt`）。
Windows のゲームは premake（`premake5.lua` と `Tools/premake/tsukino.lua`）でもビルドできます。

| OS | 描画 API | シェーダーの形式（フォルダ名） | 作り方 |
|---|---|---|---|
| Windows | Direct3D 11 | `dxbc` | Visual Studio 2022 以降 |
| macOS | Metal | `metal` | Xcode |
| iOS | Metal | `metal` | Xcode（`CMAKE_SYSTEM_NAME=iOS`） |
| Android | Vulkan | `spirv` | Android Studio / Gradle（`Platforms/Android`） |

描画 API は OS ごとに1つに決めてあり（`GraphicsContext.cpp`）、シェーダーもその形式だけを作ります
（`ShaderLoader::GetBinaryDirectoryName`）。形式を足すときは `TSUKINO_SHADER_PROFILES` を使います。

## Windows

```bat
cmake -S . -B .build/cmake -A x64
cmake --build .build/cmake --config Debug --target Tsukino.Sandbox
```

Debug は作業フォルダ（Sandbox はエンジンのフォルダ）のアセットをそのまま読みます。
Release はビルドの後に、組み込みアセットと `ASSET_DIRS` を exe の隣へ写します。

モデルの取り込み（`.fbx` / `.obj` → `.tsm`）には Assimp が要ります。CMake のパッケージ（vcpkg など）があればそれを、
無ければ premake が NuGet から復元した `.build/packages/AssimpCpp.*` を使います。どちらも無いときは取り込みを外してビルドします。

## macOS

```sh
cmake -S . -B build-mac -G Xcode
cmake --build build-mac --config Debug --target Tsukino.Sandbox
```

`shaderc` もこのビルドで作られ、組み込みシェーダーを Metal 用へ変換します。
Assimp は Homebrew（`brew install assimp`）のものを `find_package` で見つけます。

## iOS

iOS 用のビルドでは `shaderc` を作れないので、先に macOS 用のビルドで作った `shaderc` を渡します。

```sh
cmake -S . -B build-ios -G Xcode -DCMAKE_SYSTEM_NAME=iOS -DCMAKE_OSX_DEPLOYMENT_TARGET=15.0 \
      -DTSUKINO_SHADERC=$PWD/build-mac/External/bgfx/cmake/bgfx/Debug/shaderc
```

署名のチームは Xcode で設定します。アセットはビルドの後に `.app` の中へ写されます（Release）。
iOS では `.fbx` / `.obj` を取り込めないので、開発機で作った `Cache/`（`.tsm`・`.ktx`）も同梱します。

## Android

`Platforms/Android` が Gradle のプロジェクトです。SDL の Java の層（`External/SDL/android-project`）をそのまま使い、
アプリの `CMakeLists.txt` を NDK でビルドして `libmain.so` にします。

1. Windows か macOS で CMake のビルドをして `shaderc` を作る
2. APK に入れるアセットを1つのフォルダへ並べる（デスクトップの作業フォルダと同じ相対パス。
   `Tsukino.BuiltIn/Assets`・ゲームの `Assets`・開発機で作った `Cache/`）。シェーダーは `spirv` の形式が要る
3. `Platforms/Android/gradle.properties` の `tsukino.*` を設定してビルドする

```sh
cd Platforms/Android
../../External/SDL/android-project/gradlew assembleDebug -Ptsukino.shaderc=/path/to/shaderc
```

Android ではアセットを APK の中から読みます（`FileSystem` は SDL を通すので、同じ相対パスで読めます）。
セーブなどの書き込みは `FileSystem::GetUserDataPath()` の場所へ行ってください。

## スマホでの注意

- アプリが裏へ回ると、エンジンは音を止めて描画を飛ばします（`Window::AppLifecycle`）。表へ戻ると描画先を付け直します。
- UI は `Window::GetSafeArea()` の四角の中に置くと、画面の切り欠きやホームバーに隠れません。
- BC 圧縮のテクスチャを読めない GPU では、読み込むときに RGBA8 へ展開します（メモリは増えます）。
