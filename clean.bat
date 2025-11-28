@echo off
setlocal enabledelayedexpansion

echo Cleaning project...
echo.

:: Files and folders to keep
set "keep=utils clean.bat generate.bat premake5.lua"

:: Delete files (except keep list)
for %%F in (*) do (
    set "skip="
    for %%K in (%keep%) do (
        if /i "%%F"=="%%K" set "skip=1"
    )
    if not defined skip (
        echo Deleting file: %%F
        del /f /q "%%F" 2>nul
    )
)

:: Delete directories (except utils, src, Bin, vendor)
for /d %%D in (*) do (
    if /i not "%%D"=="utils" if /i not "%%D"=="src" if /i not "%%D"=="Bin" if /i not "%%D"=="vendor" (
        echo Deleting folder: %%D
        rmdir /s /q "%%D" 2>nul
    )
)

:: Delete hidden .vs folder
if exist ".vs" (
    echo Deleting folder: .vs
    rmdir /s /q ".vs" 2>nul
)

echo.
echo Done!
pause
