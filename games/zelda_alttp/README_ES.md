# The Legend of Zelda: A Link to the Past

[English](README.md) · **Español**

| | |
|---|---|
| Módulo | `zelda_alttp` ([profile.c](profile.c)) |
| ROM | EE. UU. v1.0, LoROM, 1 MB, SRAM de 8 KB |
| CRC32 | `777AAC2F` |
| SHA-1 | `6D4F10A8B10E10DBE624CB23CF03B88BB8252973` |
| Fuente de las direcciones | [snesrev/zelda3](https://github.com/snesrev/zelda3) (`src/variables.h`, `hud.c`, `messaging.c`, `dungeon.c`) |

## Segunda pantalla

- **HUD:** BG3, líneas 15-59. Sale de la pantalla de juego en los módulos `07`/`09`, durante el texto (`0E/02`) y al
  cruzar puertas (`0F`/`10`), y se dibuja sobre el mapa.
- **Menú de objetos** (`0E/01`): todo BG3 pasa a la segunda pantalla y el mundo sigue a la vista en la de juego.
- **Mapa:** el núcleo oculto pulsa X.
  - En el exterior pulsa X otra vez para ver el mundo entero (`0E/07`).
  - En una mazmorra aparece el mapa de la mazmorra (`0E/03`). No hace falta el objeto mapa: muestra las salas
    visitadas y el objeto revela el resto. Antes hay unos 30 fotogramas en negro, tras el fundido.
  - En casas y cuevas X no hace nada, y el último mapa se atenúa.

## Memoria (WRAM, comprobada con el harness)

| Dirección | Qué es | Valores vistos |
|---|---|---|
| `$7E0010` | Módulo principal | `00` intro, `01` selección de archivo, `04` nombre, `05` carga, `07` interior, `08`/`0F`/`10` transiciones, `09` exterior, `0E` menús, `17` guardar y salir, `1B` menú de inicio |
| `$7E0011` | Submódulo | Con `0E`: `01` objetos, `02` texto, `03` mapa de mazmorra, `07` mapa del mundo. Con `09`: `20-22` el cierre del mapa |
| `$7E001B` | Interior | `01` en una casa, `00` fuera |
| `$7E0020` / `$7E0022` | Y / X de Link (16 bits) | |
| `$7E008A` | Área del mundo exterior | `2C` delante de la casa de Link |
| `$7E00A0` | Sala de mazmorra (16 bits) | `12` el Santuario |
| `$7E040C` | Mazmorra ×2 | `FF` fuera de las mazmorras, `00` Castillo de Hyrule (incluido el Santuario) |
| `$7E1CD2` | Esquina del cuadro de texto (VRAM) | `$6125` arriba (fila 9 de BG3, línea 72), `$6244` abajo (fila 18) |
| `$7EF3C5` | Progreso | `02` = Zelda rescatada; permite empezar en el Santuario |
| `$7E0202` | Objeto Y equipado | Aún sin usar (inventario táctil) |
| `$7E0016` | Actualizar el HUD en el próximo NMI | Aún sin usar |

En el modo 1, BG1 es la lluvia o la capa superior, BG2 el terreno o la sala, y BG3 el HUD, el texto y el menú.

## Sonido al encender

En el fotograma 82 tras el encendido, justo después de subir su driver de sonido a la ARAM, el juego escribe `$0A`
en `$2143` (efectos de sonido): el driver toca la muestra 3 en la voz 7 a un tono muy alto (`$3D0A`), unos 0,3 s que
suenan como un siseo con la pantalla en negro. No es un artefacto de la emulación: la muestra es BRR válida y la
pide el propio código del juego (harness: `audio 200` imprime los puertos y el DSP). Nada más suena antes del
fotograma 110, así que el perfil silencia esos fotogramas (`bootMuteFrames`).

## Tests

Hace falta tu propia ROM; las partidas no, los guiones las crean.

```bash
./build/harness ROM out games/zelda_alttp/tests/new_game.txt     # del encendido al mapa y el menú
./build/harness ROM out games/zelda_alttp/tests/sanctuary.txt    # una mazmorra (usa los estados del anterior)
./build/harness ROM out games/zelda_alttp/tests/core_check.txt   # hashes del núcleo, frente a una ejecución anterior a un cambio
```

## Pendiente

- Inventario táctil (`$7E0202` + `$7E0016`).
- Otras regiones (Europa, Japón): sus CRC32, y si sus direcciones cambian.
