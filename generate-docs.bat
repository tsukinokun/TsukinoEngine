@echo off
rem ---------------------------------------------------------------------------
rem Docs/ の自動生成物を作り直す。
rem   generate-docs.bat              … 生成して最後にキー入力を待つ（手で叩く用）
rem   generate-docs.bat --no-pause   … 待たない（スクリプト／エージェント用）
rem ---------------------------------------------------------------------------
setlocal
cd /d "%~dp0"

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
