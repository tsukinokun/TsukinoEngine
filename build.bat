@echo off
rem ---------------------------------------------------------------------------
rem TsukinoEngine 単体（Sandbox 込み）のビルド。
rem   build.bat            … Debug をビルド
rem   build.bat Release    … Release をビルド
rem
rem 成功時の出力は 0 行、失敗時はエラー行だけになる。
rem MSBuild を直接叩かないこと（静音フラグを忘れると数千行出る）。
rem ゲーム側リポジトリから使う場合は、そちら側の build.bat を使うこと。
rem ---------------------------------------------------------------------------
setlocal

set "CONFIG=%~1"
if "%CONFIG%"=="" set "CONFIG=Debug"

set "ROOT=%~dp0"
set "SLN=%ROOT%.build\TsukinoEngine.sln"

rem ---------------------------------------------------------------------------
rem MSBuild の場所を解決する。msbuild は PATH に無い。
rem Community 決め打ちだと Professional / Enterprise / BuildTools で動かないため、
rem まず vswhere で引き、見つからないときだけ従来のパスへ落とす。
rem vswhere に -latest を渡すとこの環境では VS18 を返してしまうので、
rem -version "[17.0,18.0)" で VS2022 に絞る。ここを -latest に戻さないこと。
rem ---------------------------------------------------------------------------
set "MSBUILD="
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if exist "%VSWHERE%" call :find_msbuild
if not defined MSBUILD set "MSBUILD=C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe"

if not exist "%MSBUILD%" (
    echo build.bat: MSBuild not found. Install Visual Studio 2022 with the "Desktop development with C++" workload.
    exit /b 1
)

rem プロジェクトファイルが無ければ premake で生成する
rem （ソースファイルを新規追加したときも premake5 vs2022 での再生成が要る）
if not exist "%SLN%" (
    "%ROOT%vendor\premake5.exe" vs2022 >nul
    if errorlevel 1 exit /b 1
)

rem NuGet の復元。premake が吐くのは旧形式の packages.config なので
rem -t:restore だけでは復元されず -p:RestorePackagesConfig=true が要る
if not exist "%ROOT%packages" (
    "%MSBUILD%" "%SLN%" -t:restore -p:RestorePackagesConfig=true -p:Configuration=%CONFIG% -p:Platform=x64 -nologo -v:q -clp:"ErrorsOnly;NoSummary"
    if errorlevel 1 exit /b 1
)

"%MSBUILD%" "%SLN%" /p:Configuration=%CONFIG% /p:Platform=x64 /m /nologo /v:q /clp:"ErrorsOnly;NoSummary"
exit /b %ERRORLEVEL%

rem ---------------------------------------------------------------------------
rem vswhere で VS2022 の MSBuild を探すサブルーチン。
rem for /f の中で実行ファイルのパスを引用符で囲むと cmd の解析が壊れるため、
rem コマンド全体を ^" で包んである。この形を崩さないこと
rem （崩すと 'C:\Program' is not recognized で黙って fallback に落ちる）。
rem ---------------------------------------------------------------------------
:find_msbuild
for /f "usebackq tokens=*" %%i in (`^""%VSWHERE%" -version "[17.0,18.0)" -products * -requires Microsoft.Component.MSBuild -property installationPath^"`) do (
    if exist "%%i\MSBuild\Current\Bin\MSBuild.exe" set "MSBUILD=%%i\MSBuild\Current\Bin\MSBuild.exe"
)
goto :eof

