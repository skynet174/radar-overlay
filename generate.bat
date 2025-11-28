@echo off
echo ========================================
echo    Generating Visual Studio Solution
echo ========================================
echo.

:: Generate solution using vs2022 base with v145 toolset (VS2026)
utils\premake5.exe vs2022

if %errorlevel% neq 0 (
    echo.
    echo [ERROR] Failed to generate solution!
    pause
    exit /b 1
)

echo.
echo ========================================
echo    Solution generated successfully!
echo ========================================
echo.
echo Open radar.sln in Visual Studio
echo.
if %0 == "%~0" pause
