@echo off
setlocal enabledelayedexpansion

set "SCRIPT_DIR=%~dp0"
for %%I in ("%SCRIPT_DIR%..") do set "REPO_DIR=%%~fI"

set "INPUT_HTML=%~1"
if "%INPUT_HTML%"=="" (
  if exist "%SCRIPT_DIR%SaintAltarLight_Standalone.html" (
    set "INPUT_HTML=%SCRIPT_DIR%SaintAltarLight_Standalone.html"
  ) else (
    set "INPUT_HTML=%REPO_DIR%\SaintAltarLight_Standalone.html"
  )
)

set "OUTPUT_HTML=%~2"
if "%OUTPUT_HTML%"=="" (
  for %%P in ("%INPUT_HTML%") do set "OUTPUT_HTML=%%~dpPout.html"
)

set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (
  echo [ERROR] vswhere not found: "%VSWHERE%"
  echo Please install Visual Studio Community 2026 with "Desktop development with C++".
  exit /b 1
)

set "VSINSTALL="
for /f "usebackq tokens=* delims=" %%P in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSINSTALL=%%P"

if "%VSINSTALL%"=="" (
  echo [ERROR] No Visual Studio C++ toolchain found.
  echo Please install "Desktop development with C++" workload.
  exit /b 1
)

set "VSDEVCMD=%VSINSTALL%\Common7\Tools\VsDevCmd.bat"
if not exist "%VSDEVCMD%" (
  echo [ERROR] VsDevCmd.bat not found: "%VSDEVCMD%"
  exit /b 1
)

call "%VSDEVCMD%" -arch=x64 >nul
if errorlevel 1 (
  echo [ERROR] Failed to initialize VS developer environment.
  exit /b 1
)

cd /d "%SCRIPT_DIR%"

echo Building html_gen.exe ...
cl /nologo /std:c++17 /EHsc /O2 main.cpp /Fe:html_gen.exe /link shell32.lib
if errorlevel 1 (
  echo [ERROR] Build failed.
  echo.
  pause
  exit /b 1
)

if not exist "%INPUT_HTML%" (
  echo [ERROR] Input HTML not found: "%INPUT_HTML%"
  echo Searched default locations:
  echo   "%SCRIPT_DIR%SaintAltarLight_Standalone.html"
  echo   "%REPO_DIR%\SaintAltarLight_Standalone.html"
  echo.
  pause
  exit /b 2
)

echo Generating: "%OUTPUT_HTML%" ...
"%SCRIPT_DIR%html_gen.exe" "%INPUT_HTML%" "%OUTPUT_HTML%"
if errorlevel 1 (
  echo [ERROR] Generate failed.
  echo.
  pause
  exit /b 3
)

echo.
echo [OK] Done.
echo Open: "%OUTPUT_HTML%"
echo.
pause
exit /b 0
