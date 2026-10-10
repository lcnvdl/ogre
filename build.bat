@echo off
title Compilando OGRE para C#

echo ====================================================
echo 1. Compilando en modo RELEASE...
echo ====================================================
cmake --build build --config Release --parallel

echo ====================================================
echo 2. Compilando en modo DEBUG (Opcional pero recomendado)...
echo ====================================================
cmake --build build --config Debug --parallel

echo ====================================================
echo 3. Instalando y organizando librerias...
echo ====================================================
cmake --install build --config Release --prefix "./build/dist/Release"
cmake --install build --config Debug --prefix "./build/dist/Debug"

echo ====================================================
echo ¡Proceso Completado!
echo ====================================================
pause