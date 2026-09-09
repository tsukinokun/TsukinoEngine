@echo off
rem ---------------------------------------------------------------------------
rem Docs/ の自動生成物を作り直す。
rem   generate-docs.bat              … 生成して最後にキー入力を待つ（手で叩く用）
rem   generate-docs.bat --no-pause   … 待たない（スクリプト／エージェント用）
rem ---------------------------------------------------------------------------
setlocal
cd /d "%~dp0"

rem ---------------------------------------------------------------------------
rem doxygen は出力先を上書きするだけで、消えたヘッダに対応する古い出力を
rem 掃除しない。そのままだと gen-api-digest がその残骸を読み、
rem 削除・改名した型が Docs/ に残り続ける（実例: HighlightComponent を
rem RimGlowComponent へ改名しても、xml/_highlight_component_8hpp.xml が
rem 居座って api-index.md に旧名が載り続けた）。
rem 生成のたびに作り直す前提なので、先に丸ごと消す。
rem ---------------------------------------------------------------------------
echo [0/3] Cleaning stale Doxygen output...
if exist xml  rmdir /s /q xml
if exist html rmdir /s /q html

echo [1/3] Generating Doxygen documentation (HTML + XML)...
doxygen Doxyfile
if errorlevel 1 goto :failed

echo [2/3] Generating component manifest...
vendor\premake5.exe gen-manifest
if errorlevel 1 goto :failed

echo [3/3] Generating API index...
vendor\premake5.exe gen-api-digest
if errorlevel 1 goto :failed

echo.
echo Done.
echo   Docs\components.md
echo   Docs\api-index.md
echo   Docs\api\*.md
echo   Docs\agent-manifest.json
echo   html\index.html
if not "%~1"=="--no-pause" pause
exit /b 0

:failed
echo.
echo Generation failed.
if not "%~1"=="--no-pause" pause
exit /b 1
