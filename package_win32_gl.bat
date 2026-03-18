@echo off
REM Simple packaging script for Minecraft Win32 GL build
REM 1) Build the solution in Visual Studio (Debug or Release)
REM 2) Run this .bat from the repo root to assemble a clean game folder

setlocal ENABLEDELAYEDEXPANSION

REM Adjust these if you use a different config / solution
set "CONFIG=Release"
set "SLN_DIR=\handheld\project\MinecraftWin32_GL_VS2008"
set "EXE_NAME=MinecraftWin32_GL.exe"

set "ROOT=%~dp0"
set "BUILD_DIR=%ROOT%%SLN_DIR%\%CONFIG%"
set "OUT_DIR=%ROOT%dist_win32_gl_%CONFIG%"

if not exist "%BUILD_DIR%\%EXE_NAME%" (
    echo Built exe not found: "%BUILD_DIR%\%EXE_NAME%"
    echo Make sure you have built configuration "%CONFIG%" for the Win32_GL solution.
    pause
    exit /b 1
)

echo Packaging to "%OUT_DIR%"...
if not exist "%OUT_DIR%" mkdir "%OUT_DIR%"

REM Copy main exe
copy /Y "%BUILD_DIR%\%EXE_NAME%" "%OUT_DIR%\" >nul

REM Copy required DLLs
if exist "%ROOT%handheld\lib\bin" (
    xcopy "%ROOT%handheld\lib\bin\*.dll" "%OUT_DIR%\" /Y >nul
) else (
    echo WARNING: DLL folder "handheld\lib\bin" not found.
)

REM Copy data folder with textures, sounds, etc.
if exist "%ROOT%handheld\data" (
    xcopy "%ROOT%handheld\data" "%OUT_DIR%\data\" /E /I /Y >nul
) else (
    echo WARNING: Data folder "handheld\data" not found.
)

echo Done.
echo Game folder: "%OUT_DIR%"
pause

