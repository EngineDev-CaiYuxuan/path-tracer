@echo off
REM ==========================================
REM Path Tracer - Auto Build & Run Script
REM Target: Visual Studio 2026 (v18)
REM ==========================================

REM Switch to the directory where this .bat file is located
cd /d "%~dp0"

echo [1/3] Configuring CMake (VS 2026)...
REM Note: Do NOT add -A x64 here, as it conflicts with the existing cache.
cmake -S . -B build -G "Visual Studio 18 2026" -DCMAKE_BUILD_TYPE=Debug
if %errorlevel% neq 0 (
    echo [ERROR] CMake configuration failed!
    echo Tip: If you recently changed the generator or platform, delete the 'build' folder and try again.
    pause
    exit /b 1
)

echo.
echo [2/3] Building project...
cmake --build build --config Debug
if %errorlevel% neq 0 (
    echo [ERROR] Build failed!
    pause
    exit /b 1
)

echo.
echo [3/3] Running path_tracer.exe...
echo ==========================================
.\build\bin\Debug\path_tracer.exe

REM Keep the window open if the program crashed
pause