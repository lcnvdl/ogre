@echo off
title Compilando OGRE para C# (Release)
setlocal

set "SCRIPT_DIR=%~dp0"
cd /d "%SCRIPT_DIR%"

echo ====================================================
echo 1. Compilando en modo RELEASE...
echo ====================================================
cmake --build build --config Release --parallel
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] La compilacion en modo RELEASE ha fallado.
    goto :error
)

echo ====================================================
echo 2. Instalando RELEASE...
echo ====================================================
cmake --install build --config Release --prefix "%SCRIPT_DIR%build/dist/Release"
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] La instalacion en modo RELEASE ha fallado.
    goto :error
)

echo ====================================================
echo Proceso completado exitosamente.
echo ====================================================
pause
exit /b 0

:error
echo ====================================================
echo [FALLO] El proceso se detuvo debido a un error.
echo ====================================================
pause
exit /b %ERRORLEVEL%
