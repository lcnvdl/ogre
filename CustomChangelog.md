# CustomChangelog

Cambios propios de este fork (`origin` = `lcnvdl/ogre`) respecto de Ogre oficial (`upstream` = `OGRECave/ogre`). Sirve para revisar qué hay que conservar o adaptar al traer cambios de `upstream`.

- Base de `upstream`: `0420e6ba5` (2026-10-09, "Main: TangentSpaceCalc - catch zero-length tangents from degenerate-UV tris (#3748)").
- Quien usa el fork: ArgonGameEngine (C#, bindings SWIG de `Components/Csharp`). Un cambio de API pública obliga a recompilar todo, regenerar los bindings (`build/csharp`) y copiarlos a Argon.

## Al actualizar desde upstream

1. `git fetch upstream` y `git rebase upstream/master` (o merge), resolviendo conflictos con esta lista a mano.
2. Revisar que cada punto de abajo siga presente y compile; si upstream resolvió lo mismo, borrar el punto.
3. Compilar (`build-release.bat`), regenerar los bindings y desplegar en Argon (`update-ogre-binaries.bat` y los `.cs` de `build/csharp` a `OgreBindings/`, reaplicando el ajuste `LPUTF8Str` de `BitesPINVOKE.cs` que documenta el `AGENTS.md` de Argon). `update-ogre-binaries.bat` no copia los `*.cfg`, porque Argon usa los suyos.

## Cambios

### Main/Bites: archivo de configuración elegible (`17c749590`)
- `Root::getConfigFileName()` y `Root::deleteConfig()`.
- `ApplicationContextBase::setConfigFileName()`, `getConfigFileName()` y `getConfigFilePath()`; `createRoot()` los usa en lugar de un `ogre.cfg` fijo.

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
- GLX y EGL (`GLXWindow`, `EGLWindow`, que cubre X11/Wayland/Android/Emscripten): sin bordes = estado fullscreen del gestor de ventanas sin `switchMode`. Sin compilar (no hay build de Linux).
- macOS (`CocoaWindow`): su pantalla completa ya es una ventana sin bordes; `setFullscreenMode` informa `FSM_BORDERLESS`. Sin compilar.

### Herramientas locales (`51c7ecab9`)
- `CLAUDE.md` y scripts `build*.bat` (sin trackear salvo `CLAUDE.md`). Este `CustomChangelog.md`, referenciado desde `CLAUDE.md`.
