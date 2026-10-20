# SNES DS: ¿se puede hacer un "gba-ds" para SNES?

[English](RESEARCH.md) · **Español**

> [!NOTE]
> Este es el estudio de viabilidad escrito antes de empezar el desarrollo. Lo que se decidió después (LakeSnes como
> núcleo, GPL-3.0 o posterior como licencia) y cómo ha ido está en la [hoja de ruta](ROADMAP_ES.md).

**Respuesta corta: sí.** La idea de [kyannon/gba-ds](https://github.com/kyannon/gba-ds) se puede llevar a SNES casi
tal cual, y en algunos aspectos SNES lo pone más fácil. Hay una decisión importante antes de escribir código: la
**licencia del núcleo de emulación** (ver [§4](#4-la-decisión-importante-qué-núcleo-usar-y-su-licencia)).

## Contenido

1. [Cómo funciona gba-ds](#1-cómo-funciona-gba-ds-lo-que-hay-que-trasladar)
2. [La ROM de referencia](#2-la-rom-de-referencia)
3. [Qué puede mostrar la segunda pantalla en A Link to the Past](#3-qué-puede-mostrar-la-segunda-pantalla-en-a-link-to-the-past)
4. [La decisión importante: qué núcleo usar y su licencia](#4-la-decisión-importante-qué-núcleo-usar-y-su-licencia)
5. [Una alternativa solo para Zelda: la decompilación zelda3](#5-una-alternativa-solo-para-zelda-la-decompilación-zelda3)
6. [Los siguientes juegos con sentido](#6-los-siguientes-juegos-con-sentido)
7. [Plan propuesto](#7-plan-propuesto)

## 1. Cómo funciona gba-ds (lo que hay que trasladar)

Según su README, su `CLAUDE.md`, `src/profiles.c`/`profiles.h` y la página de la wiki *Adding second-screen
compatibility for a game*:

| Pieza | Qué hace | Equivalente en SNES |
|---|---|---|
| **libmgba enlazado estáticamente** | La emulación de GBA | Un núcleo de SNES usado como librería (§4) |
| **Una sola ventana sobre las dos pantallas** (sway "fullscreen global" en ROCKNIX, `dist/screens.sh`; KMSDRM con una ventana por panel en BananaOS) | Muestra las dos pantallas | Igual; el enfoque se traslada sin cambios |
| **Captura del mapa en un segundo núcleo oculto** (`mapcap.c`) | Cada ~1,5 s, y tras cada cambio de sala, copia el estado del juego a una instancia invisible, pulsa el botón del mapa, espera a que se estabilice y graba unos fotogramas | Igual: `retro_serialize`/`unserialize`, o `S9xFreezeGame` en memoria |
| **Validación por RAM y píxeles** (`mem`, `live`, `menu`, `room`, `probes`) | Solo muestra el mapa si un byte de RAM confirma que está abierto | Igual, leyendo la WRAM de 128 KB ($7E0000-$7FFFFF) |
| **HUD separado** (`emu_hud_split`) | Un segundo renderizador recibe las mismas escrituras de VRAM/OAM y dibuja solo algunas capas BG y sprites | Viable (§3.2) |
| **Un `MapProfile` por juego**, identificado por el código del header | Todo lo específico de cada juego | Identificar por CRC32/SHA-1: los headers de SNES no llevan un código único fiable |
| **Harness sin ventana** (`peek`, `dumpram`, `layershot`, `ramdiff.py`) | Encontrar direcciones de RAM | Imprescindible desde el principio |

Un detalle útil: **gba-ds ya es compatible con A Link to the Past, en su versión de GBA**. Su perfil usa el *mismo
byte de modo* que el original de SNES (el comentario dice "as on the SNES"). En GBA ese byte está en `0x03002BDC`; en
SNES, en `$7E0010`.

## 2. La ROM de referencia

```
Archivo:  Legend of Zelda, The - A Link to the Past (USA).sfc   1 048 576 bytes, sin header de copiador
Header:   "THE LEGEND OF ZELDA" @ $7FC0, mapa $20 = LoROM, ROM $0A = 1 MB, SRAM $03 = 8 KB, EE. UU., v1.0
Checksum: $50F2 / complemento $AF0D (válido)
SHA-1:    6D4F10A8B10E10DBE624CB23CF03B88BB8252973
MD5:      608C22B8FF930C62DC2DE54BCD6EBA72
```

Es el volcado limpio de EE. UU., el mismo que pide la decompilación zelda3, así que todas las direcciones de abajo
encajan con él. No tiene chip de mejora (ni SA-1 ni Super FX), lo que lo convierte en un juego barato de emular.

### Direcciones de RAM clave (WRAM, banco $7E)

Comprobadas en `src/variables.h` y `src/hud.c` de [snesrev/zelda3](https://github.com/snesrev/zelda3):

| Dirección | Significado | Uso en la pantalla 2 |
|---|---|---|
| `$7E0010` | Módulo principal: `$07` mazmorra/interior, `$09` mundo exterior, `$0E` interfaz | `live` / `mem` |
| `$7E0011` | Submódulo. Con el módulo `$0E`: `$01` menú de objetos, `$03` mapa de mazmorra, `$07` mapa del mundo (como en el perfil de GBA) | Validar la captura del mapa |
| `$7E001B` | Interior (1) o exterior (0) | Elegir qué mapa mostrar |
| `$7E0020` / `$7E0022` | Coordenadas Y/X de Link (16 bits) | Un marcador propio sobre el mapa |
| `$7E008A`, `$7E040A` | Área del mundo exterior | Detectar cambios de sala (`room`) |
| `$7E00A0` | Sala de mazmorra (16 bits) | Cambios de sala, mapa de mazmorra |
| `$7E040C` | Mazmorra actual ×2 | Qué mapa de mazmorra toca |
| `$7E0016` | Indicador de "actualizar el HUD en el próximo NMI" | Forzar que se redibuje el HUD al cambiar de objeto |
| `$7E0202` | Objeto Y equipado | Inventario táctil (§3.3) |
| `$7E00EA` | Copia de BG3VOFS (desliza el menú de objetos) | Detectar el menú |
| `$7EC700` | Buffer del tilemap del HUD (se copia a VRAM en el NMI) | Fuente directa del HUD |
| `$7EF340…` | Inventario guardado (`$7EF340` arco, `$7EF359` espada…) | Inventario en la pantalla 2 |
| `$7EF360` / `$7EF362` | Rupias (objetivo / mostradas) | Un HUD propio |
| `$7EF36C` / `$7EF36D` / `$7EF36E` | Vida máxima / vida actual / magia | Un HUD propio |

En SNES el botón del mapa es **X** (en GBA era L). El mapa del mundo de SNES se dibuja en **Mode 7**, y capturarlo
funciona igual que con cualquier otra imagen.

## 3. Qué puede mostrar la segunda pantalla en A Link to the Past

### 3.1 Un mapa en vivo (como en gba-ds)

La vía probada: una instancia oculta pulsa X, la captura se valida con `$10 == $0E && $11 ∈ {$03, $07}` y se muestra
recortada. Las capturas se hacen periódicamente y cada vez que cambian `$A0`/`$8A`.

**Una variante más barata que gba-ds no usa:** capturar el mapa del mundo una vez y dibujar encima un marcador propio
en tiempo real con `$20/$22` y `$040A`. Así, fuera de las mazmorras el núcleo oculto no necesita ejecutarse cada
1,5 s, y el mapa del exterior se actualiza a 60 fps en lugar de a ~0,7 fps.

### 3.2 El HUD en la segunda pantalla

Esto es **más fácil en SNES que en GBA**. El HUD de Zelda es solo **BG3** (tiles de 2bpp en Mode 1): el código de
zelda3 lo construye en `$7EC700` y desliza el menú con BG3VOFS.

- **Dibujar el HUD en la pantalla 2** no necesita una segunda unidad de imagen: leer de VRAM el tilemap y el CHR de
  BG3, más la CGRAM, y pintar una capa de 2bpp son unas 150 líneas de C, sin depender de las interioridades de ningún
  núcleo.
- **Quitar el HUD de la pantalla principal** sí necesita un núcleo que permita desactivar capas. Snes9x lo permite
  (`S9xToggleBG`, o las opciones de núcleo "Show layer N"), y bsnes también. **Hipótesis por comprobar con
  `layershot`:** en los módulos `$07`/`$09`, BG3 solo se usa para el HUD. Si la lluvia o la niebla también cayeran en
  BG3, la separación tendría que ir por scanlines, como hace `hud.bgPrioBelow` en gba-ds.
- Los cuadros de texto probablemente también van en BG3. Merece la pena comprobarlo: permitiría un modo "diálogos
  abajo".

> El desarrollo confirmó la hipótesis: BG3 lleva el HUD, el texto de los diálogos y el menú de objetos entero, y la
> separación se hace por scanline dentro de la unidad de imagen parcheada.

### 3.3 Inventario táctil (como hace el port de Android de samyost1)

Tocar un objeto en la pantalla de abajo, escribirlo en `$7E0202` y poner `$7E0016` a 1 para que el juego redibuje el
icono. No deben poder elegirse los objetos que el jugador no tiene (`$7EF340…`), y la escritura solo debe hacerse
cuando `$10 ∈ {$07, $09}` y `$11 == 0`. Es una escritura en memoria, como el `poke` de cámara que gba-ds ya usa para
Super Mario Advance 2.

## 4. La decisión importante: qué núcleo usar (y su licencia)

gba-ds es **GPL-3**. Reutilizar su código (`screens.sh`, `mapcap.c`, `ui.c`, el harness…) obliga a que el proyecto
nuevo también sea GPL-3, y por tanto a que el núcleo sea compatible con la GPL.

| Núcleo | Licencia | ¿Compatible con reutilizar gba-ds? | Velocidad en ARM (A55) | Comentario |
|---|---|---|---|---|
| **Snes9x** | Propia, solo no comercial | ❌ No es compatible con la GPL | Excelente: dos instancias con margen | Técnicamente el más práctico, pero obligaría a escribir el código host desde cero, sin gba-ds |
| **bsnes / bsnes-jg** (PPU rápido) | GPL-3 | ✅ | Justo con una instancia; dudoso con dos | Muy preciso, más pesado |
| **ares** (núcleo de SNES derivado de bsnes) | ISC | ✅ | Pesado | C++, muchas dependencias |
| **Mesen2** | GPL-3 | ✅ | Pesado | Herramientas de depuración excelentes, útiles para investigar en PC |
| **LakeSnes** | MIT | ✅ | Desconocida, probablemente media | C simple y fácil de modificar (zelda3 lo incluye para verificar). Hay que medirlo |

**Recomendación:**

1. Medir primero: ejecutar bsnes-jg (PPU rápido) y LakeSnes con esta ROM en la consola a velocidad libre. Si
   cualquiera de los dos supera holgadamente los ~120 fps en un núcleo de la CPU, cabe el núcleo oculto y el proyecto
   puede seguir en GPL-3 reutilizando gba-ds.
2. Si ninguno llega, usar Snes9x y escribir el host desde cero, tomando de gba-ds solo las ideas, no el código. Otra
   opción es pedirle a kyannon una excepción de licencia.

Sobre el hardware: el `CLAUDE.md` de gba-ds dice que la RG DS Plus tiene dos paneles de 1024×768 (el de abajo
táctil) y la RG DS dos de 640×480. Se dice que el chip de la Plus es un RK3568 (4× Cortex-A55), aunque no está
confirmado oficialmente. Una imagen de SNES (256×224) cabe a **escala entera 3×** en 1024×768 (768×672) y a **2×** en
640×480 (512×448).

## 5. Una alternativa solo para Zelda: la decompilación zelda3

Si el objetivo fuera **solo** A Link to the Past, ya existen ports de doble pantalla basados en
[snesrev/zelda3](https://github.com/snesrev/zelda3), el juego reescrito en C, que usa esta misma ROM para extraer sus
recursos:

- [samyost1/zelda3-android](https://github.com/samyost1/zelda3-android) (AYN Thor): mapa del mundo en vivo, mapa de
  mazmorra con las salas visitadas e inventario táctil.
- Un port para 3DS y otro para Switch con modo de doble pantalla.

Llevar uno de ellos a ROCKNIX en la RG DS sería la forma más rápida de tener Zelda, pero no serviría para ningún otro
juego. **El emulador es el enfoque que escala**: un perfil por juego, como en gba-ds.

## 6. Los siguientes juegos con sentido

Con el mismo criterio de metroidvanias que gba-ds:

- **Super Metroid**: el mapa de pausa y el minimapa del HUD. El candidato número uno.
- A Link to the Past (este), Secret of Mana y Chrono Trigger (menús, el mapa del mundo en Mode 7), Super Mario World
  (el mapa del mundo), Castlevania: Dracula X y Super Castlevania IV (el HUD) y Final Fantasy VI (menús, el mapa del
  mundo).
- El HUD de cada juego sigue la misma receta: encontrar su capa con `layershot` y dibujarla aparte.

## 7. Plan propuesto

1. **Un harness en PC primero** (como `tools/harness.c`): el núcleo más `wait`, `hold`, `shot`, `peek`, `dumpram`,
   `layershot`, `save`/`load`. Con esta ROM, comprobar en vivo las direcciones del §2 y la hipótesis de BG3 del §3.2.
2. **Medir el núcleo** en la consola (§4) y decidir la licencia.
3. **Perfil de Zelda v0**: captura del mapa con una instancia oculta (el botón X, `mem`/`live`/`menu`/`room`).
4. **El HUD en BG3**, con la opción de quitarlo de la pantalla principal.
5. **Inventario táctil** (`$0202` + `$0016`).
6. Un paquete para ROCKNIX, siguiendo el enfoque de `screens.sh` / `launch.sh`.

## Fuentes

- [gba-ds](https://github.com/kyannon/gba-ds), su wiki y su `CLAUDE.md`
- [snesrev/zelda3](https://github.com/snesrev/zelda3)
- [zelda3-android con segunda pantalla](https://retrohandhelds.gg/zelda-3-now-has-a-second-screen-with-this-port/)
- [El port de doble pantalla para 3DS](https://www.timeextension.com/news/2026/07/zelda-link-to-the-past-gets-a-dual-screen-port-on-3ds-with-help-from-ai)
- [Especificaciones de la RG DS Plus](https://retrohandhelds.gg/anbernic-rg-ds-plus-officially-revealed-equipped-with-larger-screens-and-built-in-stylus-for-under-100/)
- [Anbernic: primer vistazo a la RG DS Plus](https://anbernic.com/blogs/news/anbernic-rg-ds-plus-first-look)
