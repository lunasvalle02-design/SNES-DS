# SNES DS: ¿se puede hacer un "gba-ds" para SNES?

**Respuesta corta: sí.** La idea de [kyannon/gba-ds](https://github.com/kyannon/gba-ds) se puede llevar a SNES casi
tal cual, y en algunas cosas SNES lo pone más fácil. Hay que tomar una decisión importante antes de escribir
código, que es la **licencia del núcleo de emulación** (ver §4).

---

## 1. Cómo funciona gba-ds (lo que hay que copiar)

Lo saqué del README, de `CLAUDE.md`, de `src/profiles.c`/`profiles.h` y de la wiki
*Adding second-screen compatibility for a game*.

| Pieza | Qué hace | Equivalente en SNES |
|---|---|---|
| **libmgba enlazado estáticamente** | Emulación de GBA | Un núcleo de SNES usado como librería (§4) |
| **Una sola ventana que ocupa las dos pantallas** (sway "fullscreen global" en ROCKNIX, `dist/screens.sh`; KMSDRM con una ventana por panel en BananaOS) | Muestra las dos pantallas | Igual, se puede reutilizar sin cambios |
| **Captura del mapa en un segundo núcleo oculto** (`mapcap.c`) | Cada ~1,5 s, y tras cada cambio de sala, copia el estado del juego a una instancia invisible, pulsa el botón del mapa, espera a que se estabilice y graba unos frames | Igual: `retro_serialize`/`unserialize`, o `S9xFreezeGame` en memoria |
| **Validación por RAM y píxeles** (`mem`, `live`, `menu`, `room`, `probes`) | Solo muestra el mapa si un byte de RAM confirma que está abierto | Igual, leyendo la WRAM de 128 KB ($7E0000-$7FFFFF) |
| **HUD separado** (`emu_hud_split`) | Un segundo renderizador recibe las mismas escrituras de VRAM/OAM y dibuja solo algunas capas BG y sprites | Viable (§3.2) |
| **Un `MapProfile` por juego**, identificado por el código del header | Todo lo específico de cada juego | Identificar por CRC32/SHA-1, porque los headers de SNES no tienen un código único fiable |
| **Harness headless** (`peek`, `dumpram`, `layershot`, `ramdiff.py`) | Descubrir direcciones de RAM | Imprescindible copiarlo desde el principio |

Un detalle útil: **gba-ds ya soporta A Link to the Past, pero la versión de GBA**. Su perfil usa el *mismo byte de
modo* que el original de SNES (el comentario dice "as on the SNES"). En GBA ese byte está en `0x03002BDC`; en SNES
está en `$7E0010`.

## 2. La ROM que me pasaste

```
Archivo:  Legend of Zelda, The - A Link to the Past (USA).sfc   1 048 576 bytes, sin header de copiador
Header:   "THE LEGEND OF ZELDA" @ $7FC0, mapa $20 = LoROM, ROM $0A = 1 MB, SRAM $03 = 8 KB, región USA, v1.0
Checksum: $50F2 / complemento $AF0D (válido)
SHA-1:    6D4F10A8B10E10DBE624CB23CF03B88BB8252973
MD5:      608C22B8FF930C62DC2DE54BCD6EBA72
```

Es el dump limpio de USA. También es el que pide la decompilación zelda3, así que todas las direcciones de abajo
encajan con él. No tiene chip especial (ni SA-1 ni SuperFX), así que es un juego barato de emular.

### Direcciones de RAM clave (WRAM, banco $7E)

Las comprobé en `src/variables.h` y `src/hud.c` de [snesrev/zelda3](https://github.com/snesrev/zelda3):

| Dirección | Significado | Uso en pantalla 2 |
|---|---|---|
| `$7E0010` | Módulo principal: `$07` mazmorra/interior, `$09` mundo exterior, `$0E` interfaz | `live` / `mem` |
| `$7E0011` | Submódulo. Con módulo `$0E`: `$01` menú de objetos, `$03` mapa de mazmorra, `$07` mapa del mundo (como en el perfil de GBA) | Validar la captura del mapa |
| `$7E001B` | Interior (1) o exterior (0) | Elegir qué mapa mostrar |
| `$7E0020` / `$7E0022` | Coordenadas Y/X de Link (16 bits) | Marcador propio sobre el mapa |
| `$7E008A`, `$7E040A` | Área del mundo exterior | Detectar cambios de sala (`room`) |
| `$7E00A0` | Sala de mazmorra (16 bits) | Detectar cambios de sala, mapa de mazmorra |
| `$7E040C` | Mazmorra actual ×2 | Saber qué mapa de mazmorra toca |
| `$7E0016` | Indicador de "actualiza el HUD en el próximo NMI" | Forzar que se redibuje el HUD al cambiar de objeto |
| `$7E0202` | Objeto Y equipado | Inventario táctil (§3.3) |
| `$7E00EA` | Copia de BG3VOFS (desliza el menú de objetos) | Detectar el menú |
| `$7EC700` | Buffer del tilemap del HUD (se copia a VRAM en el NMI) | Fuente directa del HUD |
| `$7EF340…` | Inventario guardado (`$7EF340` arco, `$7EF359` espada…) | Inventario en pantalla 2 |
| `$7EF360` / `$7EF362` | Rupias (objetivo / mostradas) | HUD propio |
| `$7EF36C` / `$7EF36D` / `$7EF36E` | Vida máxima / vida actual / magia | HUD propio |

En SNES el botón del mapa es **X**; en GBA era L. El mapa del mundo de SNES se dibuja en **Mode 7**, igual que la
captura del juego, sin ningún problema añadido.

## 3. Qué se puede mostrar en la segunda pantalla con Zelda

### 3.1 Mapa en vivo (igual que gba-ds)
Es la vía probada: una instancia oculta pulsa X, se valida con `$10 == $0E && $11 ∈ {$03, $07}` y se muestra la
captura recortada. La captura se dispara periódicamente y también cuando cambian `$A0`/`$8A`.

**Variante más barata, que gba-ds no usa:** capturar el mapa del mundo una sola vez y, después, dibujar encima un
marcador propio en tiempo real con `$20/$22` y `$040A`. Así no hace falta ejecutar el núcleo oculto cada 1,5 s fuera
de las mazmorras, y en el exterior el mapa se actualiza a 60 fps en lugar de a ~0,7 fps.

### 3.2 HUD en la segunda pantalla
En SNES esto sale **más fácil que en GBA**. El HUD de Zelda es solo **BG3** (tiles de 2bpp en Mode 1): el código de
zelda3 lo construye en `$7EC700` y desliza el menú con BG3VOFS.

- **Dibujar el HUD en la pantalla 2:** no necesita un segundo PPU. Basta con leer VRAM (tilemap y CHR de BG3) y
  CGRAM del núcleo y pintar una capa 2bpp, unas 150 líneas de C. Con eso no se depende de los detalles internos de
  ningún núcleo.
- **Quitar el HUD de la pantalla principal:** sí requiere que el núcleo deje desactivar capas. Snes9x lo permite
  (`S9xToggleBG` o las opciones de núcleo "Show layer N") y bsnes también. **Hipótesis por verificar con `layershot`:**
  durante los módulos `$07`/`$09`, BG3 solo se usa para el HUD. Si las capas de lluvia o niebla también caen en BG3,
  habrá que separarlo por scanlines, como hace `hud.bgPrioBelow` en gba-ds.
- Los cuadros de texto probablemente también van en BG3. Hay que comprobarlo; sería una buena opción para un modo
  "diálogos abajo".

### 3.3 Inventario táctil (lo que hace el port de Android de samyost1)
Consiste en tocar un objeto en la pantalla inferior, escribirlo en `$7E0202` y poner `$7E0016` a 1 para que el juego
redibuje el icono. Hay que impedir que se elijan objetos que el jugador no tiene (`$7EF340…`) y hacerlo solo cuando
`$10 ∈ {$07, $09}` y `$11 == 0`. Es una escritura en memoria, como el `poke` de cámara que gba-ds ya usa para Super
Mario Advance 2.

## 4. La decisión importante: qué núcleo usar (y su licencia)

gba-ds es **GPL-3**. Si quieres reutilizar su código (`screens.sh`, `mapcap.c`, `ui.c`, el harness…), el proyecto
nuevo también tiene que ser GPL-3, y por tanto el núcleo tiene que ser compatible con la GPL.

| Núcleo | Licencia | ¿Compatible con reutilizar gba-ds? | Rendimiento en ARM (A55) | Comentario |
|---|---|---|---|---|
| **Snes9x** | Propia, solo no comercial | ❌ No es compatible con la GPL | Excelente: dos instancias van sobradas | El más práctico técnicamente, pero obligaría a escribir el código host desde cero, sin copiar gba-ds |
| **bsnes / bsnes-jg** (PPU rápido) | GPL-3 | ✅ | Justo con una instancia; dudoso con dos | Mucha precisión, más pesado |
| **ares** (núcleo SNES derivado de bsnes) | ISC | ✅ | Pesado | C++, muchas dependencias |
| **Mesen2** | GPL-3 | ✅ | Pesado | Muy buenas herramientas de depuración, útiles para la fase de investigación en PC |
| **LakeSnes** | MIT | ✅ | Desconocido, probablemente medio | C simple y fácil de modificar (zelda3 lo incluye para verificar). Hay que medirlo |

**Recomendación:**
1. Primero haz una medición: ejecuta bsnes-jg (PPU rápido) y LakeSnes con esta ROM en el dispositivo a velocidad
   libre. Si cualquiera de los dos supera holgadamente ~120 fps en un núcleo de CPU, cabe el núcleo oculto y se puede
   seguir con GPL-3 reutilizando gba-ds.
2. Si ninguno llega, usa Snes9x y escribe el host desde cero, tomando de gba-ds solo las ideas y no el código.
   Otra opción es pedirle a kyannon una excepción de licencia.

Sobre el hardware: CLAUDE.md de gba-ds dice que el RG DS Plus tiene dos paneles de 1024×768 (el inferior táctil) y
el RG DS dos de 640×480. Se ha dicho que el chip del Plus es un RK3568 (4× Cortex-A55), pero no está confirmado
oficialmente. Una imagen de SNES (256×224) cabe a **escala entera 3×** en 1024×768 (768×672) y a **2×** en
640×480 (512×448).

## 5. Alternativa solo para Zelda: la decompilación zelda3

Si el objetivo fuera **solo** A Link to the Past, ya existen ports de doble pantalla basados en
[snesrev/zelda3](https://github.com/snesrev/zelda3), que es el juego reescrito en C y usa esta misma ROM para
extraer los recursos:
- [samyost1/zelda3-android](https://github.com/samyost1/zelda3-android) (AYN Thor): mapa del mundo en vivo, mapa de
  mazmorra con salas visitadas e inventario táctil.
- Un port para 3DS y otro para Switch con modo de doble pantalla.

Llevar uno de ellos a ROCKNIX/RG DS sería lo más rápido para Zelda, pero no sirve para ningún otro juego. **El
emulador es la opción que escala**: un perfil por juego, como gba-ds.

## 6. Siguientes juegos con sentido (mismo enfoque de metroidvania que gba-ds)

- **Super Metroid**: mapa de pausa y minimapa en el HUD. Es el candidato número uno.
- Zelda ALttP (este), Secret of Mana / Chrono Trigger (menús, mapa del mundo en Mode 7), Super Mario World (mapa del
  mundo), Castlevania: Dracula X / Super Castlevania IV (HUD) y FF6 (menú, mapa del mundo).
- Para el HUD de cada juego sirve la misma receta: averiguar con `layershot` qué capa usa y renderizarla aparte.

## 7. Plan de arranque propuesto

1. **Harness en PC primero** (como `tools/harness.c`): núcleo + `wait`, `hold`, `shot`, `peek`, `dumpram`,
   `layershot`, `save`/`load`. Con esta ROM, comprobar en vivo las direcciones de §2 y la hipótesis de BG3 de §3.2.
2. **Medir el núcleo** en el dispositivo (§4) y elegir licencia.
3. **Perfil de Zelda v0**: captura del mapa con instancia oculta (botón X, `mem`/`live`/`menu`/`room`).
4. **HUD en BG3** dibujado desde VRAM, con opción de quitarlo de la pantalla principal.
5. **Inventario táctil** (`$0202` + `$0016`).
6. Empaquetado para ROCKNIX reutilizando el enfoque de `screens.sh` / `launch.sh`.

Fuentes: [gba-ds](https://github.com/kyannon/gba-ds), su wiki y su `CLAUDE.md`; [zelda3](https://github.com/snesrev/zelda3);
[zelda3-android dual screen](https://retrohandhelds.gg/zelda-3-now-has-a-second-screen-with-this-port/);
[port de 3DS](https://www.timeextension.com/news/2026/07/zelda-link-to-the-past-gets-a-dual-screen-port-on-3ds-with-help-from-ai);
[especificaciones del RG DS Plus](https://retrohandhelds.gg/anbernic-rg-ds-plus-officially-revealed-equipped-with-larger-screens-and-built-in-stylus-for-under-100/);
[Anbernic](https://anbernic.com/blogs/news/anbernic-rg-ds-plus-first-look).
