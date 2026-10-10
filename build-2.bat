@echo off
title Compilando OGRE para C#
setlocal enabledelayedexpansion

:: Guardar la ruta actual del script
set "SCRIPT_DIR=%~dp0"
cd /d "%SCRIPT_DIR%"

echo ====================================================
echo 1. Compilando en modo RELEASE...
echo ====================================================
cmake --build build --config Release --parallel
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] La compilación en modo RELEASE ha fallado.
    goto :error
)

echo ====================================================
echo 2. Instalando RELEASE...
echo ====================================================
cmake --install build --config Release --prefix "%SCRIPT_DIR%build/dist/Release"
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] La instalación en modo RELEASE ha fallado.
    goto :error
)

echo ====================================================
echo 3. Compilando en modo DEBUG...
echo ====================================================
cmake --build build --config Debug --parallel
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] La compilación en modo DEBUG ha fallado.
    goto :error
)

echo ====================================================
echo 4. Instalando DEBUG...
echo ====================================================
cmake --install build --config Debug --prefix "%SCRIPT_DIR%build/dist/Debug"
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] La instalación en modo DEBUG ha fallado.
    goto :error
)

echo ====================================================
echo ¡Proceso Completado Exitosamente!
echo ====================================================
pause
exit /b 0

:error
echo ====================================================
echo [FALLO] El proceso se detuvo debido a un error.
echo ====================================================
pause
exit /b %ERRORLEVEL%