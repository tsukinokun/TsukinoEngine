@echo off
vendor\premake5.exe vs2022

if %errorlevel% neq 0 (
    echo Premake generation failed.
    pause
    exit /b %errorlevel%
)

start "" ".build/TsukinoEngine.sln"
