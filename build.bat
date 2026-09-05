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
rem MSBuild の場所を解決する。msbuild は PATH に無く、置き場は Visual Studio の
rem 版とエディションで変わるため、三段構えで探す。
rem
rem   1) VS2022 (17.x)  premake が vs2022 形式を吐くので、あればこれを使う
rem   2) -latest        VS2022 が無い環境（VS2026 のみ等）はこちらで拾う
rem   3) 決め打ちパス    vswhere 自体が無い古い環境の保険
rem
rem 1) だけに絞っていた時期があり、VS2026 しか入っていない環境で
rem 「MSBuild not found」になっていた。2) を必ず残すこと。
rem 逆に 1) を消して -latest だけにするのも避ける。premake の生成物は
rem v143 前提なので、VS2022 があるならそちらを優先したい。
rem ---------------------------------------------------------------------------
set "MSBUILD="
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if exist "%VSWHERE%" call :find_vs2022
if not defined MSBUILD if exist "%VSWHERE%" call :find_latest
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
rem vswhere で MSBuild を探すサブルーチン。バージョン範囲を引数で渡す形にはしないこと。
rem "[17.0,18.0)" を変数経由で展開すると、閉じ括弧が for の括弧を先に閉じてしまい
rem 「-products was unexpected at this time」で落ちる。範囲ごとにラベルを分けてある。
rem for /f の中で実行ファイルのパスを引用符で囲むと cmd の解析が壊れるため、
rem コマンド全体を ^" で包んである。この形を崩さないこと
rem （崩すと 'C:\Program' is not recognized で黙って fallback に落ちる）。
rem ---------------------------------------------------------------------------
:find_vs2022
for /f "usebackq tokens=*" %%i in (`^""%VSWHERE%" -version "[17.0,18.0)" -products * -requires Microsoft.Component.MSBuild -property installationPath^"`) do (
    if exist "%%i\MSBuild\Current\Bin\MSBuild.exe" set "MSBUILD=%%i\MSBuild\Current\Bin\MSBuild.exe"
)
goto :eof

:find_latest
for /f "usebackq tokens=*" %%i in (`^""%VSWHERE%" -latest -products * -requires Microsoft.Component.MSBuild -property installationPath^"`) do (
    if exist "%%i\MSBuild\Current\Bin\MSBuild.exe" set "MSBUILD=%%i\MSBuild\Current\Bin\MSBuild.exe"
)
goto :eof

