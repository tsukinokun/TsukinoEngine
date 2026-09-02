@echo off
setlocal
cd /d "%~dp0"

echo [1/3] Generating Doxygen documentation (HTML + XML)...
doxygen Doxyfile
if errorlevel 1 goto :failed

echo [2/3] Generating component manifest...
vendor\premake5.exe gen-manifest
if errorlevel 1 goto :failed

echo [3/3] Generating API digest...
vendor\premake5.exe gen-api-digest
if errorlevel 1 goto :failed

echo.
echo Done.
echo   Docs\components.md
echo   Docs\api-digest.md
echo   Docs\agent-manifest.json
echo   html\index.html
pause
exit /b 0

:failed
echo.
echo Generation failed.
pause
exit /b 1
