# snes-ds

Un emulador de SNES para consolas de doble pantalla (Anbernic RG DS Plus / RG DS) que usa la segunda pantalla para
el mapa, el HUD o el menú de cada juego. Sigue el modelo de [gba-ds](https://github.com/kyannon/gba-ds).

- [INVESTIGACION.md](INVESTIGACION.md): viabilidad y análisis.
- [ROADMAP.md](ROADMAP.md): fases, estado y riesgos.

## Qué hace ahora (A Link to the Past, USA)

- **Pantalla de juego:** el juego sin el HUD.
- **Segunda pantalla:** el mapa en vivo con el HUD (corazones, objeto, magia, rupias) superpuesto: el del mundo en el
  exterior y el de la mazmorra dentro de ellas. Un núcleo oculto lo abre y lo captura en un hilo aparte cada
  ~1,5 s y al cambiar de zona, con los marcadores animados. El HUD sale de la pantalla de juego también durante los
  diálogos.
- Con el **menú de objetos** abierto, el inventario pasa a la segunda pantalla y el mundo sigue visible.
- En interiores sin mapa, el último mapa se muestra atenuado.

## Compilar (Windows, MSYS2 UCRT64)

```bash
export PATH=/c/msys64/ucrt64/bin:$PATH
cmake -S . -B build -G Ninja && cmake --build build
```

Hace falta `mingw-w64-ucrt-x86_64-gcc`, `cmake`, `ninja` y `SDL2`. En Windows los `.exe` se enlazan de forma
estática, así que funcionan desde cualquier terminal (PowerShell, cmd) sin las DLLs de MSYS2.

## Usar

```bash
./build/snes-ds.exe "Legend of Zelda, The - A Link to the Past (USA).sfc"
```

Teclas: flechas · X = A · Z = B · S = X · A = Y · Q/W = L/R · Enter = Start · Mayús dcha = Select.
F2 intercambia las pantallas · F3 activa o desactiva la separación del HUD · F4 cambia entre aspecto original y 4:3 ·
F5/F7 guardan y cargan estado ·
Tab avance rápido · F11 pantalla completa · Esc salir.
Opciones (`--aspect=4:3`, `--layout=side`, `--screen=1024x768`, `--save-dir`, `--no-map`…): ver la cabecera de
[src/main.c](src/main.c).

## En la consola (ROCKNIX: RG DS Plus / RG DS)

```bash
scripts/build-rocknix-docker.sh      # desde Windows, con Docker Desktop
scripts/build-rocknix.sh             # desde Linux x86_64 con aarch64-linux-gnu-gcc
```

Deja `dist/snes-ds-v<VERSION>-rocknix-aarch64.zip`, que se descomprime en `/storage/roms/ports/`. Las instrucciones
para la consola (controles, opciones, qué mirar del rendimiento) están en [dist/README.txt](dist/README.txt), que va
dentro del zip.

## Harness (investigar juegos sin la consola)

```bash
./build/harness.exe ROM out games/zelda_alttp/tests/new_game.txt
```

Ejecuta el juego frame a frame desde un script: pulsaciones, capturas por capa, `peek`/`watch`/`expect`
sobre la RAM, estados y captura del mapa. Los comandos están en la cabecera de [tools/harness.c](tools/harness.c).
Los tests de cada juego están en `games/<id>/tests/`. Los de Zelda (`new_game.txt` y `sanctuary.txt`, que usa los
estados del primero) hacen de tests de regresión: llegan desde el encendido hasta el mapa y el menú, y hasta una mazmorra, comprobando por el
camino los valores de RAM, la captura del mapa y la separación del HUD.

## Juegos: un módulo por juego

Todo lo específico de un juego vive en su directorio y en ningún otro sitio:

```
games/
  games.c                 la lista de juegos (una línea por juego)
  zelda_alttp/
    profile.c             su perfil: lo único que ve el motor
    README.md             su mapa de memoria, capas y hallazgos
    tests/*.txt           sus scripts del harness
```

El motor (`src/`) no sabe nada de ningún juego: lee el perfil (sondas de memoria, reglas de capas, cómo abrir el
mapa) y lo aplica. Encuentra el perfil por el CRC32 de la ROM.

**Añadir un juego:**
1. Investigarlo con el harness: `layershot` para ver qué capa lleva el HUD o el menú, y `watch`/`dumpram` para
   encontrar los valores de memoria que indican cada pantalla.
2. Crear `games/<id>/profile.c` con `const Profile game_<id> = { .id = "<id>", ... }`, junto con su `README.md` y sus
   `tests/`.
3. Añadir `extern const Profile game_<id>;` y `&game_<id>,` en `games/games.c`.

CMake recoge solo los `.c` de `games/*/`. Si algún día un juego necesita código propio (por ejemplo, el inventario
táctil de Zelda), irá en su directorio y el perfil lo enganchará, sin tocar el motor ni los demás juegos.

## Estructura

| Ruta | Qué es |
|---|---|
| `src/core.h/.c` | Interfaz del núcleo (LakeSnes detrás) |
| `src/profiles.h/.c` | El motor de perfiles (tipos, búsqueda por CRC32, evaluación de sondas) |
| `games/` | Un directorio por juego, con su perfil, documentación y tests |
| `src/mapcap.h/.c` | Captura del mapa con núcleo oculto, síncrona o en hilo |
| `src/screen2.h/.c` | Qué muestra la segunda pantalla |
| `src/main.c` | La app (SDL2) |
| `tools/harness.c` | Harness sin ventana |
| `third_party/lakesnes` | [LakeSnes](https://github.com/angelo-wf/LakeSnes) (MIT), parcheado ([PATCHES.md](third_party/lakesnes/PATCHES.md)) |
| `third_party/stb_image_write.h` | Escritura de PNG (dominio público) |
| `third_party/zip` | Lectura de `.zip` ([kuba--/zip](https://github.com/kuba--/zip) y miniz, dominio público) |

## Licencia

snes-ds es software libre: puedes redistribuirlo y modificarlo bajo los términos de la
[GNU General Public License](LICENSE), versión 3 o (a tu elección) cualquier versión posterior
(`GPL-3.0-or-later`). Se distribuye sin ninguna garantía.

El código de terceros en `third_party/` conserva su propia licencia (LakeSnes: MIT; zip y miniz: dominio
público; stb_image_write: dominio público), todas compatibles con la GPL-3.
