# Super Mario World 2: Yoshi's Island

[English](README.md) · **Español**

| | |
|---|---|
| Módulo | `super_mario_world_2_yoshis_island` ([profile.c](profile.c)) |
| ROM | USA Rev 1 (v1.1), Super FX 2 (GSU-2), 2 MB, 32 KB de RAM del GSU (con pila) |
| CRC32 | `CF98DDAA` |
| SHA-1 | `34612A93741F156D6E497462AB7F253CB8A959A0` |
| Fuentes de las direcciones | Encontradas con el harness (`watch`, volcados de RAM comparados entre estados, valores escritos y leídos de vuelta en la pantalla de pausa) |

Funciona con el Super FX 2, que LakeSnes no tiene: el Super FX del núcleo está en
[`third_party/lakesnes/gsu.c`](../../third_party/lakesnes/gsu.c) (ver
[PATCHES.md](../../third_party/lakesnes/PATCHES.md)). Sin él el juego se queda en una pantalla negra.

## Segunda pantalla

- **La pantalla de pausa, siempre al día** (las estrellas, monedas rojas y flores del nivel con sus puntos, el total
  y el récord, y la barra con las vidas, las monedas, las estrellas y los objetos). Un nivel no tiene HUD propio: el
  juego solo muestra esto en la pantalla de pausa, que ocupa toda la imagen (`PAUSE` en sprites, el resto BG1, sobre
  negro). Así que el núcleo oculto pausa el juego para la segunda pantalla (el "mapa" del perfil): desde el juego
  normal (`$0118` = `0F`, sin pausa, sin cuadro de mensaje) pulsa START y toma la imagen cuando la pantalla de pausa
  está entera (`$0B10` = `01`, `$0B0F` = `13`, ~39 frames). El título `PAUSE` se deja fuera (sprites en las líneas
  0-49): en la segunda pantalla el juego no está en pausa. Se vuelve a tomar en cuanto cambian las monedas, las
  monedas rojas, las flores o las vidas, y cada 2 segundos por las estrellas.
- **Sin división.** La cuenta atrás de estrellas (un sprite arriba cuando baby Mario cae) va y viene sobre el juego;
  los cuadros de mensaje de los bloques de pistas son BG3, que también lleva parte del nivel, y una ventana tapa de
  negro las otras capas bajo ellos, así que dejarían un agujero negro en la pantalla del juego. La pantalla de pausa
  de verdad se queda en la pantalla del juego, donde se eligen sus objetos.

## Memoria (WRAM, comprobada con el harness)

| Dirección | Qué es | Valores vistos |
|---|---|---|
| `$7E0118` | Modo del juego | `01`-`07` la introducción, `08`-`0A` el título y el menú de archivos, `37`/`38` saliendo del menú, `39` la historia, `0E` el cartel de un nivel (espera un botón), `0F` un nivel, también en pausa; `12`, `15` se llevan a baby Mario |
| `$7E0B10` | En pausa | `01` desde START hasta que START cierra la pantalla de pausa, `00` si no |
| `$7E0B0F` | La pantalla de pausa entrando | `01` a `13` (entera, ~35 frames tras START), y de vuelta a `00` al cerrarse |
| `$7E0D0F` | El cuadro de mensaje de un bloque de pistas | `09`, `0B`, `0D` mientras se abre y pasa páginas, `00` sin él |
| `$7E0379` | Vidas | `03` en un archivo nuevo (escrito `07`: la pantalla de pausa muestra 7) |
| `$7E037B` | Monedas | sube al recogerlas en 1-1 (`01`, `02`…) |
| `$7E03B4` | Monedas rojas del nivel | escrito `03`: la pantalla de pausa muestra 3/20 |
| `$7E03B8` | Flores del nivel | escrito `02`: la pantalla de pausa muestra 2/5 |

El byte siguiente a cada cuenta se queda en `00` (la segunda pantalla las lee como valores de 16 bits). La cuenta de
estrellas no es un byte sin más: `$7E03B6` vale `6D` con 10 estrellas, y al escribirlo salían otros valores, así que
las estrellas van por el refresco periódico.

## Hallazgos

- En un nivel, BG1 y BG2 son el nivel, BG3 los cuadros de mensaje (arriba) y parte del nivel (las plantas de abajo en
  1-1), y los sprites Yoshi, baby Mario, los enemigos y la cuenta atrás de estrellas.
- La pantalla de pausa se dibuja sola (BG1 y sprites sobre negro, el juego oculto); en el núcleo oculto tarda unos
  39 frames en aparecer y quedarse quieta: ~40 ms por captura en un PC.
- Velocidad (PC, esta máquina): ~800 fps en la introducción, ~480 fps en 1-1: el Super FX trabaja mucho al jugar,
  más o menos la mitad de velocidad que un juego sin él. Queda medirlo en la consola.
- Encendido: el primer sonido es la melodía de "Nintendo Presents" (frame 81, con el logo en pantalla): nada que
  silenciar.
- Los estados guardados incluyen el Super FX: un estado guardado en 1-1 y vuelto a cargar sigue hasta la misma imagen
  y la misma RAM, con el hilo de dibujo activado y sin él.

## Tests

Hace falta tu propia ROM; partidas guardadas no, los scripts las crean.

```bash
scripts/check.sh ROMDIR super_mario_world_2_yoshis_island
```

| Script | Comprueba |
|---|---|
| `new_game.txt` | Del encendido, por la introducción, el título, un archivo nuevo y la historia, hasta 1-1: el modo del juego por el camino, ninguna captura mientras hay un cuadro de mensaje, y luego las cuentas al empezar y la captura de la pantalla de pausa |
| `play.txt` | Monedas recogidas en 1-1 y la captura con ellas; monedas rojas, flores y vidas (escritas) y la captura; la pausa de verdad: ninguna captura en pausa, y otra vez después |

## Pendiente

- La velocidad en la consola: el Super FX es lo más pesado que ha movido el núcleo hasta ahora.
- El resto del juego, al que no llegan los tests: el mapa del mundo entre niveles (ahí la segunda pantalla mantiene
  la última captura), los jefes (escalado y rotación del Super FX), los minijuegos. Si algo se dibuja mal, es cosa de
  la emulación del Super FX (de una rama `engine/`) más que del perfil.
- La dirección de la cuenta de estrellas, para capturar en cuanto cambie en vez de cada 2 segundos.
