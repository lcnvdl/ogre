# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Qué es

OGRE (Object-Oriented Graphics Rendering Engine): motor de render C++ modular (MIT). Este checkout se usa también para generar los bindings **C#** (`Components/Csharp`, vía SWIG), por eso los `.bat` de la raíz.

## Build

CMake es el único sistema de build (guía completa: `BuildingOgre.md`). El directorio `build/` ya está configurado con el generador **Visual Studio 17 2022** (D3D11 + GL3+ activos, Vulkan off, `OGRE_BUILD_SAMPLES=ON`, `OGRE_BUILD_COMPONENT_CSHARP=ON`, `OGRE_BUILD_TESTS=OFF`).

```
cmake --build build --config Release --parallel
cmake --build build --config Debug --parallel
cmake --install build --config Release --prefix ./build/dist/Release
```

- `build-release.bat`: Release + install a `build/dist/Release`, deteniéndose ante errores.
- `build-2.bat`: Release + Debug, cada uno con su install a `build/dist/{Release,Debug}`.
- `build.bat`: versión antigua sin comprobación de errores (no usarla como referencia).
- Los `.bat` están sin trackear en git (son locales del usuario).
- Opciones relevantes: `OGRE_BUILD_RENDERSYSTEM_*`, `OGRE_BUILD_PLUGIN_*`, `OGRE_BUILD_COMPONENT_*` (ver `CMakeLists.txt` ~l.317-333), `OGRE_BUILD_TESTS`, `OGRE_BUILD_SAMPLES`, `OGRE_STATIC`.
- Los componentes Python/Java/C# están deshabilitados con `OGRE_STATIC`; requieren SWIG.

## Tests

Hay que activar `-DOGRE_BUILD_TESTS=ON` (está OFF en `build/`) y tener GTest disponible.

- Unit tests (GTest): ejecutable `Test_Ogre` (fuentes en `Tests/OgreMain/src`, `Tests/Components/*`, `Tests/RenderSystems`; entrada `Tests/src/main.cpp`). Un solo test: `build/bin/Test_Ogre --gtest_filter=Suite.Nombre`.
- Visual tests: `TestContext` (`Tests/VisualTests`), p. ej. `./TestContext -rs "OpenGL 3+ Rendering Subsystem"`.
- Fuzzers en `Tests/fuzz` (OSS-Fuzz).
- CI (`.github/workflows/ci-build.yml`, config en `ci-build.cmake`): compila con `-Werror`, valida que Doxygen no emita warnings (`build/doxygen_warnings.log` vacío), corre unit y visual tests, y `git diff --check` (sin espacios sobrantes al final de línea).

## Arquitectura

- `OgreMain/`: núcleo (scene graph, recursos, materiales, meshes, animación, `Root`, `RenderSystem` abstracto). Headers públicos en `include/` con prefijo `Ogre` (`OgreFoo.h`); headers privados en `src/`.
- `RenderSystems/`: backends intercambiables (D3D11, D3D9, GL, GL3Plus, GLES2, Metal, Vulkan, Tiny) sobre `GLSupport` común. Se cargan como plugins.
- `PlugIns/`: scene managers (Octree, PCZ, BSP), codecs de imagen (STBI, FreeImage, EXR), ParticleFX, DotScene, Assimp, GLSLang, etc. Cargados dinámicamente vía `plugins.cfg` (o estáticamente con `OGRE_BITES_STATIC_PLUGINS`).
- `Components/`: librerías opcionales — `Bites` (framework de apps/ventanas/input, depende de `Overlay`), `Overlay` (UI + ImGui), `RTShaderSystem` (generación de shaders), `Terrain`, `Paging`, `Volume`, `MeshLodGenerator`, `Property`, `Bullet`, y bindings `Python`/`Java`/`Csharp` (SWIG).
- `Tools/` (XMLConverter, MeshUpgrader…), `Samples/`, `Docs/`, `CMake/` (módulos, toolchains, instalación).
- Ojo: `OgreMain` y los componentes tienen interfaces que alimentan los bindings SWIG; cambios de firmas/headers públicos pueden romper la generación de C#/Python/Java.

## Convenciones

(`Docs/CodingStandards.md`)

- Cada fuente empieza con el encabezado: `// This file is part of the OGRE project.` / `// It is subject to the license terms in the LICENSE file found in the top-level directory` / `// of this distribution and at https://www.ogre3d.org/licensing.` / `// SPDX-License-Identifier: MIT`.
- Todo dentro del namespace `Ogre`; preferir STL a contenedores propios, evitar C-strings/`void*`/varargs; `const`-correctness; destructores `virtual` si hay vtable; `private` sobre `protected`; PImpl para reducir dependencias.
- Compatibilidad C++ conservadora (el estándar mínimo lo fija el CMake; el código es multiplataforma, sin headers de plataforma salvo ventanas/render).
- Formato: `_clang-format` en la raíz. Mensajes de commit estilo `Componente: descripción` (p. ej. `Main: Bone - update ...`).
- Cambios relevantes de API se documentan en `Docs/14-Notes.md`.
