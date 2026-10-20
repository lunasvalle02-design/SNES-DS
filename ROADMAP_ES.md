# Hoja de ruta de SNES DS

[English](ROADMAP.md) · **Español**

El objetivo es un emulador de SNES para consolas de doble pantalla (Anbernic RG DS Plus, RG DS) que use la segunda
pantalla para el mapa, los menús o el HUD de cada juego, siguiendo el modelo de
[gba-ds](https://github.com/kyannon/gba-ds). El estudio de viabilidad con el que empezó está en
[INVESTIGACION_ES.md](INVESTIGACION_ES.md).

Leyenda: ✅ hecho · 🔶 en curso · ⬜ pendiente

## Estado de un vistazo

| Fase | | Estado |
|---|---|---|
| 0 | Investigación | ✅ |
| 1 | Núcleo y harness en PC | ✅ |
| 2 | App de escritorio con dos pantallas, lista de juegos y menú de pausa | ✅ |
| 3 | Segunda pantalla para A Link to the Past | 🔶 Falta la pantalla de reserva |
| 4 | La consola (ROCKNIX) | ✅ A velocidad completa en la RG DS Plus; entrada y táctil confirmados |
| 5 | Táctil y comodidad | ✅ Cerrada por ahora |
| 6 | Más juegos | 🔶 Añadidos Super Metroid y Super Castlevania IV |
| 7 | Integración (EmulationStation, RetroAchievements) | ⬜ |

## Versiones

| Versión | Qué trajo |
|---|---|
| 0.1.0-dev | La primera versión para ROCKNIX. Medida en la consola: 18-39 ms por fotograma emulado (25-50 fps), con cortes de audio |
| 0.2.0-dev | El núcleo unas 3,7 veces más rápido (idéntico bit a bit); captura del mapa más barata; L1 + Start + Select para salir |
| 0.3.0-dev | La interfaz táctil, en inglés y en español: lista de juegos, menú de pausa, cuatro ranuras de estado con captura, ajustes. El menú se abre también tocando la pantalla de abajo o con cualquier botón libre. Corregido: un estallido de ruido al arrancar un juego |
| 0.4.0-dev | Comprobada en la consola: 60 fps sin cortes de audio; el menú se abre con L3 o el botón R (inicio). Partidas, estados y ajustes escritos de forma segura (un apagado no puede corromperlos); el log de velocidad ya no cuenta el tiempo en el menú |
| 0.5.0 | Dos juegos más, cada uno en su rama: Super Metroid (el mapa de pausa, con el HUD encima) y Super Castlevania IV (el HUD). Un perfil puede recortar la pantalla de juego mientras su HUD está abajo, así la sala de Super Metroid llena la pantalla de arriba. Los dos comprobados en la consola |

## Camino a la 1.0

| Versión | Objetivo | Estado |
|---|---|---|
| 0.4 | Comprobada en la consola: velocidad, entrada, táctil; partidas que un apagado no puede corromper | ✅ |
| 0.5 | Un segundo y un tercer juego, en ramas `game/…` (Super Metroid, Super Castlevania IV), con los hooks del motor que necesiten; el formato del perfil se asienta | ✅ |
| 0.6 (candidata) | Suspender y reanudar, sesiones largas, CI en GitHub Actions, `CHANGELOG.md`, releases en GitHub, las instrucciones de la consola en inglés | ⬜ |
| 1.0.0 | La candidata usada en la consola durante unos días sin problemas | ⬜ |

Lo que promete la 1.0, una vez publicada (versionado semántico: romper algo de esto obliga a la 2.0): el formato de
`snes-ds.cfg`, que las partidas y los estados sigan cargando, las opciones de línea de comandos y el formato del
perfil en el que se escriben los módulos de juego.

## Decisiones

| Tema | Decisión | Motivo |
|---|---|---|
| Núcleo | **LakeSnes** (MIT, C, incluido en `third_party/lakesnes`) | Su licencia permite reutilizar código de gba-ds (GPL-3). Es pequeño y legible (~7700 líneas), así que se puede parchear para separar capas, y zelda3 lo usa para verificar A Link to the Past |
| Aislamiento del núcleo | Todo el código pasa por [`src/core.h`](src/core.h) | Si LakeSnes se queda corto en la consola, otro núcleo (bsnes-jg, Snes9x…) puede sustituirlo sin tocar nada más |
| Separación de capas | Un parche en la unidad de imagen: `hideMask` (ocultar capas por línea) y `splitMask` (dibujar capas en una segunda imagen por línea) | El equivalente al `emu_hud_split` de gba-ds, pero exacto por scanline y casi gratis (~3 %) |
| Identificación de juegos | El CRC32 de la ROM sin header de copiador | Los headers de SNES no llevan un código de juego fiable |
| Código por juego | Solo en `games/<id>/`; el motor no contiene datos de ningún juego | Los juegos nunca se mezclan; el formato, la disposición y la interfaz son funciones generales de la app |
| Compilación | CMake + Ninja; GCC de MSYS2 (UCRT64) en PC, toolchain aarch64 en Docker para la consola | |
| Licencia | GPL-3.0 o posterior | Permite reutilizar código de gba-ds |

## Fase 0: Investigación ✅

- ✅ Análisis de gba-ds: arquitectura, perfiles, captura del mapa, separación del HUD, despliegue en ROCKNIX.
- ✅ ROM de referencia comprobada: A Link to the Past USA v1.0, CRC32 `777AAC2F`, SHA-1 `6D4F10A8…`.
- ✅ Mapa de RAM de A Link to the Past, sacado de zelda3.

## Fase 1: Núcleo y harness en PC ✅

- ✅ LakeSnes incluido y parcheado ([PATCHES.md](third_party/lakesnes/PATCHES.md)).
- ✅ `src/core.h/.c`: ROM, fotogramas, mando, acceso a WRAM/VRAM/CGRAM/OAM/ARAM/DSP, peek/poke, estados en memoria,
  copia de estado entre núcleos, SRAM, ocultar y separar capas.
- ✅ `tools/harness.c`: `wait/hold/tap/until`, `shot/layershot/splitshot/splitbox/sheet`, `peek/poke/watch/expect`,
  `dumpram`, `save/load`, `savesram/loadsram`, `hide/split`, `map`, `bench`, `hash`, `audio`.
- ✅ Guiones que llegan al juego desde el encendido, en [`games/zelda_alttp/tests/`](games/zelda_alttp/tests/).
- ✅ **Comprobado en vivo con la ROM:**
  - `$7E0010/11`: `07/00` es control libre en interiores, `09/00` en exteriores, `0E/02` diálogo, `0E/01` el menú
    de objetos, `0E/07` el mapa del mundo, y `09/20-22` el cierre del mapa.
  - Mapa del mundo: X abre la vista ampliada, X otra vez el mundo entero y X de nuevo lo cierra. El marcador de Link
    parpadea en rojo y blanco.
  - Capas: BG1 es la lluvia o la capa superior, BG2 el terreno y la sala, y **BG3 lleva el HUD, el texto de los
    diálogos y el menú de objetos entero**.
  - El HUD es BG3, líneas 15-59 (se separan las líneas 0-64); separarlo cuesta un ~3 % de velocidad.
  - Captura del mapa en el núcleo oculto: el mundo entero con 8 fotogramas de animación, 86 fotogramas de juego y
    unos 220 ms en PC. En la casa de Link responde correctamente "aquí no hay mapa".
- ⬜ `tools/ramdiff.py`: comparar volcados de RAM, como en gba-ds.
- ✅ Tests de regresión: `new_game.txt` (del encendido al mapa y el menú) y `sanctuary.txt` (una mazmorra: su mapa y
  el cuadro de texto arriba y abajo). Usan `expect`, `map … ok` y `untiltap`, salen con 1 si algo cambia y no
  necesitan partidas guardadas.

## Fase 2: App de escritorio con dos pantallas ✅

- ✅ `src/main.c`: una ventana SDL2 con las dos pantallas, apiladas (arriba el juego, abajo la segunda pantalla) o lado
  a lado (`--layout=side --screen=1024x768`, como en la consola), a escala entera.
- ✅ Entrada por teclado y mando (SDL GameController, por posición de los botones). ⬜ Mapeo configurable.
- ✅ Audio a 48 kHz con cola de SDL, sincronizado por el audio (por el reloj si no hay audio).
- ✅ Partidas (`.srm` junto a la ROM o en `--save-dir`; se escriben al cambiar y al salir).
- ✅ Estado rápido (F5/F7), avance rápido (Tab) y cuatro ranuras de estado con captura (desde el menú).
- ✅ Formato: el original (256×224 a escala entera, por defecto) o 4:3, que llena los paneles de 1024×768. Para 4:3
  la imagen se escala primero por números enteros, nítida, y solo se suaviza el último tramo, así que todos los
  píxeles miden lo mismo.
- ✅ **Lista de juegos** al arrancar (táctil y botones), con caché de CRC (`library-cache.txt`) para arrancar rápido
  con muchas ROMs, y las carátulas de EmulationStation (`gamelist.xml`, `images/`).
- ✅ **Menú de pausa** en la pantalla táctil: continuar, guardar o cargar estado, ajustes, salir del juego o de la app.
- ✅ **Ajustes** (formato, intercambiar pantallas, separación del HUD, mapa, volumen, FPS, idioma) guardados en
  `snes-ds.cfg`.
- ✅ La interfaz en inglés (por defecto) y en español ([`src/i18n.h`](src/i18n.h)), a elegir en los ajustes.
- ✅ La interfaz ([`src/ui/`](src/ui)): maquetación independiente de la resolución, formas suavizadas, Nunito con
  stb_truetype, un tema naranja con los colores de los botones del SNES en las pistas de controles.

## Fase 3: Segunda pantalla para A Link to the Past 🔶

- ✅ Perfiles por juego basados en datos (sondas de memoria con máscara y valor, reglas para separar capas, cómo abrir
  el mapa). Un módulo por juego en `games/<id>/` (perfil, `README.md` con su mapa de memoria, tests), registrado en
  `games/games.c`; el motor (`src/profiles.c`) no contiene datos de ningún juego.
- ✅ **HUD abajo:** en los módulos `07`/`09` (incluido el paso entre salas), las líneas 0-64 de BG3 van a la pantalla
  de abajo; también en los diálogos (`0E/02`) y al cruzar puertas (`0F`/`10`). El cuadro de texto empieza en la fila
  9 de BG3 (línea 72) si está arriba y en la 18 si está abajo (`$7E1CD2`), así que nunca llega al HUD.
- ✅ **Menú de objetos abajo:** con `0E/01`, todo BG3 baja y el mundo sigue a la vista arriba.
- ⬜ **Diálogos abajo** (opcional): llevar también el cuadro de texto a la segunda pantalla. Una opción del perfil.
- ✅ **Mapa en vivo con núcleo oculto** (`src/mapcap.c`, en un hilo): copiar el estado, pulsar X (y X otra vez para
  el mundo entero), esperar a que se estabilice (`$10/$11 = 0E/07` o `0E/03`), grabar 8 fotogramas para la animación
  del marcador, validar y publicar. Cada ~1,5 s y cada vez que cambian `$A0`/`$8A`.
- ✅ El mapa de mazmorra (`0E/03`). No hace falta el objeto mapa (muestra las salas visitadas; el objeto revela el
  resto). El mapa aparece tras unos fotogramas en negro, que la captura ya no da por buenos.
- ✅ Composición: el HUD sobre el mapa, en sus propias líneas; las dos pantallas del mismo tamaño. El mapa se atenúa
  donde no hay mapa. ⬜ Disposición configurable (vista ampliada o mundo entero…).
- ✅ El siseo del encendido (un efecto de sonido que el juego pide antes de mostrar nada), silenciado desde el perfil.
- ⬜ Una pantalla de reserva cuando no hay nada que mostrar: el logo o el título del juego.

## Fase 4: La consola (RG DS Plus / RG DS, ROCKNIX) 🔶

- ✅ Compilación cruzada aarch64: `scripts/build-rocknix.sh` (en cualquier Linux x86-64) o
  `scripts/build-rocknix-docker.sh` (Debian 12 en Docker, desde Windows). El binario solo necesita la `libSDL2` de
  ROCKNIX y glibc ≥ 2.34.
- ✅ Probado en aarch64 con QEMU (Docker `--platform linux/arm64`): pasan los dos tests de Zelda, y la app funciona con
  SDL2, lado a lado sobre 2048×768, con capturas del mapa y la interfaz. La velocidad bajo QEMU no significa nada.
- ✅ La app, preparada para la consola:
  - Pantalla completa, y `swaymsg '[pid=…] fullscreen enable global'` para ocupar los dos paneles.
  - La disposición sigue el tamaño real de la salida (lado a lado si es ancha).
  - Cierre limpio con SIGTERM (se escribe la partida).
  - El log apunta los dispositivos de entrada, el mapeo del mando, las primeras pulsaciones y toques, y la velocidad.
- ✅ `dist/launch.sh`, `dist/screens.sh` (encender el segundo panel y colocarlo a la derecha; al salir, dejarlo como
  estaba), `dist/SNES DS.sh` (la entrada de Ports) y `dist/bench.sh`. Escritos para este proyecto, no copiados de
  gba-ds.
- ✅ Un zip para `/storage/roms/ports/`: `dist/snes-ds-v<VERSION>-rocknix-aarch64.zip`.
- ✅ **Primera medida en la consola (v0.1.0):** 18-39 ms por fotograma emulado, 25-50 fps, con cortes de audio.
  Insuficiente.
- ✅ **LakeSnes optimizado** ([PATCHES.md](third_party/lakesnes/PATCHES.md)) sin cambiar lo que emula: los hashes de
  imagen y WRAM de `games/zelda_alttp/tests/core_check.txt` son idénticos antes y después, con el hilo de dibujo y
  sin él, en x86-64 y en aarch64. Fotogramas emulados por segundo en PC, en el exterior con lluvia:

  | Paso | fps | × |
  |---|---|---|
  | Original | 245 | 1,0 |
  | Unidad de imagen por líneas: capas decodificadas una vez, ventanas y brillo precalculados | 293 | 1,2 |
  | Reloj por tramos entre eventos | 321 | 1,3 |
  | Composición por capas en vez de por píxel | 439 | 1,8 |
  | Paso completo solo tras cambiar la configuración de la IRQ (`irqDirty`) | 460 | 1,9 |
  | `-O3` + LTO | 486 | 2,0 |
  | Saltarse las parejas de capa y prioridad sin píxeles en la línea | 504 | 2,1 |
  | Fondos decodificados tile a tile | 589 | 2,4 |
  | **Dibujo en un hilo propio** (otro núcleo de la CPU) | **~900** | **~3,7** |

- ✅ Un núcleo oculto del mapa más barato: no dibuja mientras abre el mapa, va con prioridad baja en Linux, y la espera
  entre capturas cuenta desde que termina la anterior. Una captura del mundo pasó de 271 a 108 ms en PC.
- ✅ **Segunda medida en la consola (v0.3.0, A Link to the Past, de la casa de Link al castillo):** 60 fps en todo
  momento, **9,3-10,2 ms por fotograma emulado** (la estimación era ~10), picos de 15-19 ms mientras se captura el
  mapa, que absorbe la cola de audio: **ningún corte de audio**. Dibujar cuesta ~1,7 ms. Las capturas del mapa tardan
  ~290 ms en el exterior y hasta ~1,1 s en el castillo, en segundo plano. Para este juego no hace falta optimizar
  más; los juegos más pesados (chips de mejora, Mode 7 todo el rato) se medirán según lleguen.
- ✅ El táctil en la consola: un panel Goodix, asignado por sway a todo el escritorio; como la ventana de SNES DS ocupa
  los dos paneles, los toques caen donde deben (el panel de abajo es la mitad derecha).
- ✅ El botón de menú. El log de la consola mostraba que guide (botón 10) y L3 (11) llegaban a la app, pero el menú no
  se abría: el bucle principal descartaba la acción de menú del mando (solo pasaban Esc y los toques). Corregido en
  la 0.4, con un test que maneja la app con un mando virtual (`tools/app_tests/menu.txt`, lo pasa
  `scripts/check.sh`). Confirmado en la consola: **L3 y el botón R** (el de inicio, guide en SDL) abren el menú.
- ✅ Las partidas, los estados, sus capturas, los ajustes y la caché de la lista se escriben en un fichero temporal,
  se fuerzan al disco y se renombran sobre el anterior ([`src/fileutil.c`](src/fileutil.c)): un apagado deja el
  fichero viejo o el nuevo.
- ✅ El log de velocidad y el contador de FPS no cuentan el tiempo en la lista de juegos ni en el menú (antes una
  pausa aparecía como 28 fps).
- ⬜ BananaOS (KMSDRM, una ventana por panel).

## Fase 5: Táctil y comodidad ✅

Cerrada por ahora. El táctil maneja las pantallas propias de SNES DS; los juegos no lo usan, por decisión (comodidad
y estabilidad), así que el inventario táctil de A Link to the Past queda descartado.

- ✅ La lista de juegos y el menú por táctil; tocar la pantalla de abajo pausa el juego y abre el menú.
- ✅ Intercambiar pantallas (ajustes).
- ✅ Carátulas en la lista de juegos.
- ✅ La interfaz en inglés y en español.
- ❌ Inventario táctil en A Link to the Past (descartado).
- Más adelante: ocultar la segunda pantalla, ajustes por juego, mostrar los juegos sin perfil (jugables a una
  pantalla).

## Fase 6: Más juegos 🔶

**Paso 0, hecho ✅: la base para añadir juegos en ramas separadas**, ya sean agentes o personas:

- ✅ [AGENTS.md](AGENTS.md) ([en español](AGENTS_ES.md)): las reglas básicas, el esquema de ramas (`game/<id>`,
  `engine/<tema>`, `fix/<tema>`, `docs/<tema>`), el flujo investigación → perfil → tests → documentación, y cómo llegan
  al motor el código y las optimizaciones propias de un juego (hooks genéricos y opcionales en ramas `engine/`).
- ✅ [`scripts/check.sh`](scripts/check.sh): los tests de todos los juegos con un solo comando, cada ROM encontrada por
  su perfil (`harness --find`), con los hashes de referencia del núcleo guardados en el repositorio
  (`core_check.expected`).
- ✅ [`docs/game-template/`](docs/game-template): un perfil, los README y un primer test de los que partir.

Después, el mismo camino para cada juego: harness → `layershot` para las capas → `watch` para los indicadores →
perfil → tests → prueba en la consola. Una rama cada uno:

1. ✅ **Super Metroid** (`game/super_metroid`): el mapa de pausa y el minimapa del HUD.
2. ⬜ Super Mario World (`game/super_mario_world`): el mapa del mundo y el HUD.
3. ⬜ Secret of Mana, Chrono Trigger y Final Fantasy VI (`game/secret_of_mana`, `game/chrono_trigger`,
   `game/final_fantasy_vi`): menús y el mapa del mundo en Mode 7.
4. 🔶 Super Castlevania IV ✅ y Dracula X ⬜ (`game/super_castlevania_iv`, `game/castlevania_dracula_x`): el HUD.

## Fase 7: Integración ⬜

- ⬜ EmulationStation y BananaOS, como el `esinteg.c` de gba-ds.
- ⬜ RetroAchievements con rcheevos (modo casual).
- ⬜ Android (GammaOS), más adelante.

## Riesgos

| Riesgo | Impacto | Mitigación |
|---|---|---|
| LakeSnes no llega a 60 fps en el Cortex-A55 | Era alto, ahora bajo | Optimizado unas 3,7 veces (fase 4): A Link to the Past va a ~9,5 ms/fotograma en la RG DS Plus. Faltan por medir juegos más pesados; `core.h` permite cambiar de núcleo |
| Fallos de precisión de LakeSnes (su `bugs.md`: bloqueos en subidas al SPC en algunos juegos) | Medio | A Link to the Past está verificado por zelda3; probar cada juego nuevo |
| Un juego cambia los registros de BG3 a mitad de fotograma | Bajo | La separación se hace en la propia unidad de imagen, scanline a scanline, así que ya está cubierto |
| Licencias (Snes9x no es compatible con la GPL) | Medio | Seguir con LakeSnes o bsnes |
| El botón de menú o los paneles táctiles de la consola llegan a la app de otra forma | Medio | Varias formas de abrir el menú (un toque, cualquier botón libre, Select + Start); el log apunta los dispositivos de entrada |
