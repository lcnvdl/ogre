# CustomChangelog

Cambios propios de este fork (`origin` = `lcnvdl/ogre`) respecto de Ogre oficial (`upstream` = `OGRECave/ogre`). Sirve para revisar qué hay que conservar o adaptar al traer cambios de `upstream`.

- Base de `upstream`: `68782dcd4` (2026-10-12, "Main: Root - simplify extra iOS checks in restoreConfig"), traída con el merge `031959d42` (2026-10-13). Base anterior: `0420e6ba5`.
- Las actualizaciones se hacen con merge, no con rebase: los commits del fork ya están en `origin` y un rebase obligaría a hacer force push.
- Quien usa el fork: ArgonGameEngine (C#, bindings SWIG de `Components/Csharp`). Un cambio de API pública obliga a recompilar todo, regenerar los bindings (`build/csharp`) y copiarlos a Argon.

## Al actualizar desde upstream

1. `git fetch upstream` y `git merge --no-ff upstream/master`, resolviendo conflictos con esta lista a mano. Actualizar la base de arriba.
2. Revisar que cada punto de abajo siga presente y compile; si upstream resolvió lo mismo, borrar el punto.
3. Compilar (`build-release.bat`), regenerar los bindings y desplegar en Argon (`update-ogre-binaries.bat` y los `.cs` de `build/csharp` a `OgreBindings/`, reaplicando el ajuste `LPUTF8Str` de `BitesPINVOKE.cs` que documenta el `AGENTS.md` de Argon). `update-ogre-binaries.bat` no copia los `*.cfg`, porque Argon usa los suyos.

## Cambios

### Main/Bites: archivo de configuración elegible (`17c749590`)
- `Root::getConfigFileName()` y `Root::deleteConfig()`.
- `ApplicationContextBase::setConfigFileName()`, `getConfigFileName()` y `getConfigFilePath()`; `createRoot()` los usa en lugar de un `ogre.cfg` fijo.
- `OgreRoot.cpp` es el archivo con más riesgo de conflicto: upstream lo tocó en `d576e79c4` y `68782dcd4` (`saveConfig` con `_openFileStream`, `restoreConfig` en iOS), y el merge entró sin conflictos.

### Bites: `ApplicationContextBase::showConfigDialog` (`323d1ca54`)
- Permite volver a mostrar el diálogo de configuración de Ogre.

### D3D11: `setFullscreen` síncrono (`fab0c1b8a`)
- `D3D11RenderWindowHwnd::setFullscreen` aplica estilo, `SetFullscreenState`, `ResizeTarget` y `ResizeBuffers` en orden en el momento, en vez de dejar el cambio a medias para `_beginUpdate`.
- (`43679be3c` intentó conservar la frecuencia del escritorio al entrar en pantalla completa y se revirtió en `ff3db57d8`.)

### D3D11: sin Alt+Enter de DXGI (`551f9e49f`)
- `MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER)` al crear la ventana y en `notifyDeviceRestored` (antes `0`). DXGI ponía la swap chain en pantalla completa exclusiva por su cuenta: con GPU híbridas en el monitor de la otra GPU y sin redimensionar los buffers. Cada aplicación decide qué hace Alt+Enter.

### D3D11: restaurar la ventana al salir de pantalla completa (`551f9e49f`)
- `_saveWindowedState()` / `_restoreWindowedState()` guardan estilo, estilo extendido y rectángulo de la ventana antes de entrar y los restauran al salir (también en ventanas externas, como la de SDL, que antes quedaban sin marco y centradas en otro monitor).
- `adjustWindow()` usa el estilo real de la ventana (`GetWindowLong`) cuando existe; `mWindowedWinStyle` solo se rellena para ventanas creadas por Ogre.

### Main + render systems: `RenderWindow::setFullscreenMode` (modos de pantalla completa estilo Unity) (`551f9e49f`)
- `RenderWindow::FullscreenMode { FSM_WINDOWED, FSM_EXCLUSIVE, FSM_BORDERLESS }`, `virtual setFullscreenMode(mode, width, height)` y `getFullscreenMode()`; nuevo miembro protegido `mIsBorderlessFullScreen`. Cambia el ABI de `RenderWindow`: hay que recompilar todo y regenerar los bindings.
- Por defecto `setFullscreenMode` llama a `setFullscreen(mode != FSM_WINDOWED, ...)`, así que los render systems sin implementación propia tratan el modo sin bordes como pantalla completa normal: D3D9 (no se compila en este build, su pantalla completa depende de resetear el dispositivo), Vulkan (no implementa `setFullscreen`), Metal, Tiny.
- D3D11 (`D3D11RenderWindowHwnd`): sin bordes = ventana popup del tamaño del monitor de la ventana, sin `SetFullscreenState` (la composición del escritorio presenta en cualquier GPU). `_beginUpdate`, `_finishSwitchingFullscreen`, `setActive`, `_destroySwapChain`, `_createSwapChain`, `getVBlankMissCount` y `_resizeSwapChainBuffers` solo sincronizan DXGI en modo exclusivo. `setFullscreen` delega en `setFullscreenMode`. Compilado y probado en Argon (Pong, 2026-10-13): los tres modos, ida y vuelta, perder el foco en sin bordes y Alt+Enter ignorado.
- GL/GL3+ en Windows (`GLSupport` `Win32Window`): sin bordes con `enterBorderlessFullscreen()` / `leaveBorderlessFullscreen()`, sin `ChangeDisplaySettingsEx`; `setActive` y `destroy` solo restauran el modo de video si era exclusivo. Compilado.
- GLX y EGL (`GLXWindow`, `EGLWindow`, que cubre X11/Wayland/Android/Emscripten): sin bordes = estado fullscreen del gestor de ventanas sin `switchMode`. Compilado en Linux (X11, 2026-10-17), sin probar.
- macOS (`CocoaWindow`): su pantalla completa ya es una ventana sin bordes; `setFullscreenMode` informa `FSM_BORDERLESS`. Sin compilar.

### D3D11: cambiar el tamaño de la ventana sin moverla de lugar (`022384037`)
- `D3D11RenderWindowHwnd::_restoreWindowedState` mantiene el centro del rectángulo guardado en vez de su esquina superior izquierda, y limita la ventana al área de trabajo de su monitor. Antes, un cambio de tamaño en modo ventana (por ejemplo, de 800×600 a 1280×720 con `setFullscreenMode(FSM_WINDOWED, w, h)`) hacía crecer la ventana hacia abajo a la derecha. Al salir de pantalla completa el tamaño no cambia, así que la ventana vuelve exactamente a donde estaba.
- Solo cambia `OgreD3D11RenderWindow.cpp` (sin cambios de API): basta con recompilar `RenderSystem_Direct3D11` y copiar la DLL. GL (`Win32Window`) ya centraba la ventana en ese caso. Probado en Argon (StarDelivery, 2026-10-14).

### Bites: `ApplicationContextBase::raiseWindow` (`6f48bda36`)
- Con SDL < 2.0.22 (Ubuntu 22.04 trae 2.0.20) `SDL_HINT_FORCE_RAISEWINDOW` no existe: `#ifndef` y solo `SDL_RaiseWindow` (el hint solo importa en Windows).
- `virtual raiseWindow(NativeWindowType*)` (vacío en la base) y la sobrecarga `raiseWindow()` sobre `mWindows[0]`, como `setWindowGrab`. Trae la ventana al frente y le da el foco.
- `ApplicationContextSDL` lo implementa con `SDL_RaiseWindow` activando `SDL_HINT_FORCE_RAISEWINDOW` solo durante la llamada (y restaurando el valor anterior): sin el hint, Windows solo hace parpadear el botón de la barra de tareas si otro proceso tiene el foco. Qt no lo implementa.
- Argon lo llama al terminar el primer frame si la ventana está en modo ventana, porque a veces arrancaba detrás de otras ventanas.
- SWIG no regenera los bindings de Bites al cambiar solo el header: hubo que tocar `Components/Bites/include/OgreBites.i` antes de compilar.

### Main: archivo `Callback` para leer recursos a través del host (`OGRE_ARGON_VFS`, apagado por defecto) (`657ed2305`)
- Opción CMake `OGRE_ARGON_VFS` (por defecto `FALSE`): define `USE_ARGON_VFS` en `OgreBuildSettings.h` (`#cmakedefine01`), que SWIG también lee, así que `Ogre.i` usa `#if USE_ARGON_VFS` (y los bindings exponen la constante `USE_ARGON_VFS`). SWIG no se vuelve a ejecutar al cambiar solo la opción: hay que tocar `Ogre.i` y borrar el `CallbackArchiveFactory.cs` que quede en `build/csharp`. Sin la opción, la clase no existe ni en la DLL ni en los bindings.
- `OgreCallbackArchive.h`/`.cpp` (nuevos, todo dentro de `#if USE_ARGON_VFS`): `CallbackArchiveFactory` (tipo `"Callback"`) y, privados en el `.cpp`, `CallbackArchive` y `CallbackDataStream`. Los archivos los entrega la aplicación con una tabla `CallbackArchiveFunctions` de punteros a función (`open`, `read`, `seek`, `tell`, `getLength`, `close`, `exists`, `list`); el `DataStream` pide los bytes a medida que Ogre los lee. Es genérico: no sabe nada de Argon ni de cifrado.
- El nombre del archivo (`addResourceLocation(nombre, "Callback", grupo)`) es una carpeta relativa a la raíz del host (`"."` es la raíz); a las callbacks llegan rutas UTF-8 con `/`. `find`/`list` siguen las reglas de `FileSystemArchive` (carpeta opcional en el patrón, máscara sobre el nombre base, sin distinguir mayúsculas) y se recalculan en cada llamada, sin caché. Las carpetas se deducen de las rutas de los archivos. `getModifiedTime` devuelve 0.
- `CallbackArchiveFactory::registerFunctions(void*)` (en C#, `IntPtr`) copia la tabla y registra el factory en `ArchiveManager`; hay que llamarlo después de crear `Root`.
- Lo usa Argon (fase 2 de "recursos a demanda") para que Ogre lea los recursos del juego por el VFS del motor.

### Csharp: bindings sin compilador de C# (Linux)
- `Components/Csharp/CMakeLists.txt`: fuera de MSVC, si no encuentra `mcs` (Mono) ya no apaga el componente; genera igual los `.cs` y `libOgre.so` y solo omite `Ogre.dll` (y su install). Argon compila los `.cs` con .NET 8, así que no necesita Mono.

### Bites: fondo del diálogo de configuración GLX en instalaciones reubicables (Linux)
- `OgreGLXConfigDialog.cpp`: `CreateBackdrop` busca primero `GLX_backdrop.png` junto al ejecutable (`/proc/self/exe`) y después en `OGRE_MEDIA_DIR "/../"`, una ruta absoluta fija al compilar (`/usr/local/share/OGRE-14.6`) que no existe si Ogre no se instaló ahí. Argon copia la imagen a `OgreBinariesLinux/`.
- Si no encuentra la imagen, el formulario usa un fondo blanco (`XtNbackground`) en vez de un pixmap `None`, que dejaba la ventana sin pintar (se veía lo que había detrás). Compilado en Linux (X11, 2026-10-17).

### Deps: Assimp estático también en Linux (`CMake/Dependencies.cmake`)
- Ogre compilaba Assimp 6.0.5 estático solo con `MSVC OR MINGW OR SKBUILD`; en Linux `Codec_Assimp.so` enlazaba el `libassimp.so.5` del sistema (5.2 en Ubuntu 22.04), que el juego distribuido no lleva. Ahora lo compila en todas las plataformas salvo Android, Emscripten e iOS (zlib sigue solo en Windows).
- `-DASSIMP_BUILD_ZLIB=ON`: Assimp usa su zlib y su minizip; los `.a` del sistema no tienen `-fPIC` y no se pueden enlazar en una `.so` ("relocation R_X86_64_PC32 ... recompile with -fPIC"). En Windows ya era el valor por defecto.
- El bloque de dependencias solo corre si `build/Dependencies` no existe: en un build existente hay que compilar Assimp a mano con los mismos parámetros y reconfigurar con `-Dassimp_DIR=build/Dependencies/lib/cmake/assimp-6.0` (ver `Docs/Forja Games/instrucciones-linux.txt`). `Codec_Assimp.so` pasa de 160 KB a 12 MB y ya no tiene dependencias fuera de Ogre. Probado con Starship en Argon (2026-10-17).

### Herramientas locales (`51c7ecab9`)
- `CLAUDE.md` y scripts `build*.bat` (sin trackear salvo `CLAUDE.md`). Este `CustomChangelog.md`, referenciado desde `CLAUDE.md`.
