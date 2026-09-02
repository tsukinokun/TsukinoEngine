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
set "MSBUILD=C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe"
set "SLN=%ROOT%.build\TsukinoEngine.sln"

rem msbuild は PATH に無い。vswhere -latest はこの環境では VS18 を返すため使わない
if not exist "%MSBUILD%" (
    echo build.bat: MSBuild not found: "%MSBUILD%"
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
