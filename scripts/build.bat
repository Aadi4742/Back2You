@echo off
REM ==============================================================
REM Back2You - Automated Build Script for Windows
REM ==============================================================

echo [Back2You] Starting CMake build process...

if not exist build (
    mkdir build
)

cd build

cmake ..
if %ERRORLEVEL% NEQ 0 (
    echo [Back2You Error] CMake configuration failed.
    exit /b %ERRORLEVEL%
)

cmake --build . --config Release
if %ERRORLEVEL% NEQ 0 (
    echo [Back2You Error] Compilation failed.
    exit /b %ERRORLEVEL%
)

echo.
echo [Back2You] Build succeeded! Executable located in build\bin\Release\back2you_server.exe
cd ..
