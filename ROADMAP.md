# snes-ds: roadmap

Objetivo: un emulador de SNES para consolas de doble pantalla (Anbernic RG DS Plus, RG DS) que use la segunda
pantalla para el mapa, el menú o la interfaz de cada juego, siguiendo el modelo de
[gba-ds](https://github.com/kyannon/gba-ds). La investigación previa está en [INVESTIGACION.md](INVESTIGACION.md).

Leyenda: ✅ hecho · 🔶 en curso · ⬜ pendiente · ❓ decisión abierta

---

## Decisiones tomadas

| Tema | Decisión | Motivo |
|---|---|---|
| Núcleo inicial | **LakeSnes** (MIT, C, vendorizado en `third_party/lakesnes`) | Su licencia permite reutilizar código de gba-ds (GPL-3). Es pequeño y legible (~7700 líneas), así que se puede parchear para separar capas, y zelda3 lo usa para verificar ALttP |
| Aislamiento del núcleo | Todo el código pasa por [`src/core.h`](src/core.h) | Si LakeSnes resulta lento en el dispositivo, se cambia por otro núcleo (bsnes-jg, Snes9x…) sin tocar nada más |
| Separación de capas | Parche en el PPU: `hideMask` (ocultar capas por línea) y `splitMask` (dibujar capas en una segunda imagen por línea) | Es el equivalente al `emu_hud_split` de gba-ds, pero exacto por scanline y casi gratis (~3 %) |
| Identificación de juegos | CRC32 de la ROM sin header | Los headers de SNES no tienen un código de juego fiable |
| Compilación | CMake + Ninja; GCC de MSYS2 (UCRT64) en PC, toolchain aarch64 para el dispositivo | |
| Licencia del proyecto | ❓ GPL-3 recomendada (permite reutilizar gba-ds) | Pendiente de que lo confirmes |

## Fase 0: Investigación ✅
- ✅ Análisis de gba-ds (arquitectura, perfiles, captura de mapa, separación del HUD, despliegue en ROCKNIX).
- ✅ Verificación de la ROM: ALttP USA v1.0, CRC32 `777AAC2F`, SHA-1 `6D4F10A8…`.
- ✅ Mapa de RAM de ALttP sacado de zelda3.

## Fase 1: Núcleo y harness en PC ✅
- ✅ LakeSnes vendorizado y parcheado ([PATCHES.md](third_party/lakesnes/PATCHES.md)).
- ✅ `src/core.h/.c`: ROM, frame, mando, WRAM/VRAM/CGRAM/OAM, peek/poke, estados en memoria, copia de estado
  entre núcleos, SRAM, ocultar y separar capas.
- ✅ `tools/harness.c`: `wait/hold/tap/until`, `shot/layershot/splitshot/splitbox/sheet`, `peek/poke/watch/expect`,
  `dumpram`, `save/load`, `savesram/loadsram`, `hide/split`, `map`, `bench`.
- ✅ Scripts para llegar al juego desde cero en [`games/zelda_alttp/tests/`](games/zelda_alttp/tests/).
- ✅ **Comprobado en vivo con tu ROM:**
  - `$7E0010/11`: `07/00` es control libre en interior, `09/00` en exterior, `0E/02` diálogo, `0E/01` menú de
    objetos, `0E/07` mapa del mundo, y `09/20-22` el cierre del mapa.
  - Mapa del mundo: X abre la vista ampliada, X otra vez el mundo entero y X otra vez lo cierra. El marcador de Link
    parpadea en rojo y blanco.
  - Capas: BG1 es la lluvia o la capa superior, BG2 el terreno y la sala, y **BG3 lleva el HUD, el texto de diálogo y
    el menú de objetos completo**.
  - El HUD ocupa BG3, líneas 15-59 (se separan las líneas 0-64), y separarlo cuesta un ~3 % de rendimiento.
  - Captura del mapa en el núcleo oculto: el mundo entero con 8 frames de animación, 86 frames de juego y unos
    220 ms en PC. En la casa de Link responde correctamente "no hay mapa aquí".
  - Velocidad de LakeSnes en PC: unos 240 fps (4× tiempo real).
- ⬜ `tools/ramdiff.py`: comparar volcados de RAM, como en gba-ds.
- ✅ Tests de regresión: `games/zelda_alttp/tests/new_game.txt` (del encendido al mapa y el menú) y
  `sanctuary.txt` (mazmorra: mapa de mazmorra y cuadro de texto arriba y abajo). Usan `expect`,
  `map … ok` y `untiltap`, salen con 1 si algo cambia y no necesitan partidas guardadas.

## Fase 2: App de escritorio con dos pantallas 🔶 (falta el menú de pausa)
- ✅ `src/main.c`: ventana SDL2 con las dos pantallas apiladas (arriba el juego, abajo la segunda pantalla), escala
  entera, modo lado a lado (`--layout=side --screen=1024x768`, el del dispositivo), F2 para intercambiarlas y
  `--frames/--shot` para probar sin ventana.
- ✅ Entrada por teclado y mando (SDL GameController, por posición). ⬜ Mapeo configurable (el RG DS da los botones
  por etiqueta, ver gba-ds).
- ✅ Audio a 48 kHz con cola SDL y sincronización por audio (por reloj si no hay audio).
- ✅ SRAM persistente (`.srm` junto a la ROM o en `--save-dir`; se escribe al cambiar y al salir).
- ✅ Estado rápido (F5/F7) y avance rápido (Tab). ⬜ Varias ranuras.
- ✅ Relación de aspecto: la original (256×224 a escala entera, por defecto) o 4:3 (`--aspect=4:3`, F4), que llena
  los paneles de 1024×768. Para 4:3 se escala primero a enteros con píxeles exactos y luego se suaviza solo el último
  tramo, así que todos los píxeles miden lo mismo.
- ⬜ Menú de pausa básico (continuar, guardar o cargar estado, cambiar de pantalla, salir).

## Fase 3: Segunda pantalla para Zelda 🔶 (falta la pantalla de reserva)
- ✅ Perfiles por juego basados en datos (sondas de memoria con máscara y valor, reglas de separación de capas, cómo
  abrir el mapa). Hay un módulo por juego en `games/<id>/` (perfil, `README.md` con su mapa de memoria, tests)
  registrado en `games/games.c`; el motor (`src/profiles.c`) no contiene datos de ningún juego.
- ✅ **HUD abajo:** en los módulos `07`/`09` (incluido el paso entre salas), BG3 de las líneas 0-64 va a la
  pantalla inferior. También durante los diálogos (`0E/02`) y al cruzar puertas (`0F`/`10`): el cuadro de texto
  empieza en la fila 9 de BG3 (línea 72) si está arriba y en la 18 si está abajo (`$7E1CD2`), así que nunca toca el HUD.
- ✅ **Menú de objetos abajo:** con `0E/01`, todo BG3 va abajo y el mundo sigue visible arriba.
- ⬜ **Diálogos abajo** (opcional): llevar también el cuadro de texto a la segunda pantalla. Opción del perfil.
- ✅ **Mapa en vivo con núcleo oculto** (`src/mapcap.c`, en un hilo): copiar el estado, pulsar X (y X otra vez para
  el mundo entero), esperar a que se estabilice (`$10/$11 = 0E/07` o `0E/03`), capturar 8 frames para la animación
  del marcador, validar y publicar. Se dispara cada ~1,5 s y al cambiar `$A0`/`$8A`.
- ✅ Mapa de mazmorra (`0E/03`). No hace falta tener el objeto mapa (muestra las salas visitadas; el objeto revela el
  resto). El mapa aparece tras unos frames en negro, que la captura ya no da por buenos.
- ✅ Composición: el HUD superpuesto sobre el mapa, en sus mismas líneas; las dos pantallas tienen el mismo tamaño.
  El mapa se atenúa donde no hay mapa. ⬜ Diseño configurable (mapa ampliado o mundo entero…).
- ⬜ Pantalla de reserva cuando no hay nada que mostrar: logo o título del juego.

## Fase 4: Dispositivo (RG DS Plus / RG DS, ROCKNIX) ⬜
- ⬜ Compilación cruzada aarch64 (toolchain como `scripts/build-rocknix.sh` de gba-ds, o WSL).
- ⬜ **Medir LakeSnes en el dispositivo.** Hacen falta ≥60 fps para el juego más una reserva para el núcleo oculto. Si
  no llega:
  1. Optimizar el PPU de LakeSnes (ahora calcula píxel a píxel; pasarlo a renderizado por línea y por capa).
  2. Cambiar de núcleo detrás de `core.h`: bsnes-jg (GPL-3) o Snes9x (licencia no comercial, ver INVESTIGACION §4).
- ⬜ `dist/launch.sh` y `dist/screens.sh` adaptados de gba-ds (encender DSI-1, colocarlo a la derecha, el táctil
  sobre todo el layout).
- ⬜ Ventana global sobre las dos pantallas (sway "fullscreen global"); KMSDRM en BananaOS.
- ⬜ Zip de release para `/storage/roms/ports/snes-ds/`.

## Fase 5: Táctil y comodidad ⬜
- ⬜ **Inventario táctil en Zelda:** tocar un objeto escribe `$7E0202` y pone `$7E0016` a 1, solo en `07|09/00` y solo
  con objetos que se tienen (`$7EF340…`).
- ⬜ Intercambiar pantallas, ocultar la segunda, ajustes por juego.
- ⬜ Lista de juegos con logos y una marca para los que tienen perfil.

## Fase 6: Más juegos ⬜
Mismo flujo para cada uno: harness → `layershot` para ver qué capa usa → `watch`/`ramdiff` para los indicadores →
perfil → prueba en el dispositivo.
1. **Super Metroid**: mapa de pausa y minimapa del HUD.
2. Super Mario World: mapa del mundo y HUD.
3. Secret of Mana, Chrono Trigger y FF6: menús y mapa del mundo en Mode 7.
4. Super Castlevania IV y Dracula X: HUD.

## Fase 7: Integración ⬜
- ⬜ EmulationStation y BananaOS, como `esinteg.c` de gba-ds.
- ⬜ RetroAchievements con rcheevos (modo casual).
- ⬜ Android (GammaOS), más adelante.

---

## Riesgos

| Riesgo | Impacto | Mitigación |
|---|---|---|
| LakeSnes no llega a 60 fps en el Cortex-A55 | Alto | Medirlo pronto (fase 4); el PPU por línea es el cuello de botella conocido; `core.h` permite cambiar de núcleo |
| Fallos de precisión de LakeSnes (`bugs.md`: bloqueos en subidas al SPC en algunos juegos) | Medio | ALttP está verificado por zelda3; probar cada juego nuevo |
| Un juego cambia los registros de BG3 a mitad de frame | Bajo | La separación se hace en el propio PPU, scanline a scanline, así que ya lo contempla |
| Licencia (Snes9x no es compatible con la GPL) | Medio | Seguir con LakeSnes o bsnes; decidirlo antes de copiar código de gba-ds |

## Cómo compilar y probar (PC, Windows)

```bash
export PATH=/c/msys64/ucrt64/bin:$PATH
cmake -S . -B build -G Ninja && cmake --build build
./build/harness.exe "ROM.sfc" out games/zelda_alttp/tests/new_game.txt
./build/snes-ds.exe "ROM.sfc"
```
