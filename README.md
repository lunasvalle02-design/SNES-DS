# snes-ds

Un emulador de SNES para consolas de doble pantalla (Anbernic RG DS Plus / RG DS) que usa la segunda pantalla para
el mapa, el HUD o el menú de cada juego. Sigue el modelo de [gba-ds](https://github.com/kyannon/gba-ds).

- [INVESTIGACION.md](INVESTIGACION.md): viabilidad y análisis.
- [ROADMAP.md](ROADMAP.md): fases, estado y riesgos.

## Qué hace ahora (A Link to the Past, USA)

- **Pantalla de juego:** el juego sin el HUD.
- **Segunda pantalla:** el mapa del mundo en vivo, que un núcleo oculto abre y captura en un hilo aparte cada
  ~1,5 s y al cambiar de zona, con el marcador animado. Encima va el HUD (corazones, objeto, magia, rupias).
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
F2 intercambia las pantallas · F3 activa o desactiva la separación del HUD · F5/F7 guardan y cargan estado ·
Tab avance rápido · F11 pantalla completa · Esc salir.
Opciones (`--layout=side`, `--screen=1024x768`, `--save-dir`, `--no-map`…): ver la cabecera de
[src/main.c](src/main.c).

## Harness (investigar juegos sin la consola)

```bash
./build/harness.exe ROM out scripts/zelda/new_game.txt
```

Ejecuta el juego frame a frame desde un script: pulsaciones, capturas por capa, `peek`/`watch`/`expect`
sobre la RAM, estados y captura del mapa. Los comandos están en la cabecera de [tools/harness.c](tools/harness.c).
`scripts/zelda/new_game.txt` hace de test de regresión: llega desde el encendido hasta el mapa y el menú,
comprobando por el camino los valores de RAM y la captura del mapa.

## Estructura

| Ruta | Qué es |
|---|---|
| `src/core.h/.c` | Interfaz del núcleo (LakeSnes detrás) |
| `src/profiles.h/.c` | Todo lo específico de cada juego |
| `src/mapcap.h/.c` | Captura del mapa con núcleo oculto, síncrona o en hilo |
| `src/screen2.h/.c` | Qué muestra la segunda pantalla |
| `src/main.c` | La app (SDL2) |
| `tools/harness.c` | Harness sin ventana |
| `third_party/lakesnes` | [LakeSnes](https://github.com/angelo-wf/LakeSnes) (MIT), parcheado ([PATCHES.md](third_party/lakesnes/PATCHES.md)) |
| `third_party/stb_image_write.h` | Escritura de PNG (dominio público) |
