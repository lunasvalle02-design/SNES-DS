# Super Metroid

[English](README.md) · **Español**

| | |
|---|---|
| Módulo | `super_metroid` ([profile.c](profile.c)) |
| ROM | Japón y EE. UU. (una sola ROM, texto en inglés y japonés) v1.0, LoROM, 3 MB, SRAM de 8 KB |
| CRC32 | `D63ED5F8` |
| SHA-1 | `DA957F0D63D14CB441D215462904C4FA8519C613` |
| Fuente de las direcciones | Los mapas de RAM de la comunidad (estado del juego `$0998`, sala `$079B`, zona `$079F`), cada uno comprobado con el harness |

## Segunda pantalla

- **HUD** (energía, misiles, supermisiles, superbombas, garfio, rayos X, el minimapa): BG3, líneas 0-30. Sale de la
  pantalla de juego al jugar y al cruzar puertas (`$0998` = `08`-`0B`) y en la pantalla de pausa (`0C`, `0F`, `10`,
  `12`), y se dibuja sobre el mapa. Las 32 líneas de arriba de la pantalla de juego quedan en negro.
- **Mapa:** el núcleo oculto pulsa Start; la pantalla de pausa se abre en el mapa de la zona en la que está Samus,
  centrado en ella (`$0998` = `0F`, `$0727` = `00`), unos 70 fotogramas después, tras un fundido. Se vuelve a
  capturar al cambiar de sala o de zona, y cada 3 segundos (el mapa se va rellenando mientras Samus explora una sala).
  - Donde Samus no puede pausar (la estación Ceres, al salir de la nave, en ascensores, en escenas) el último mapa se
    atenúa.

## Memoria (WRAM, comprobada con el harness)

| Dirección | Qué es | Valores vistos |
|---|---|---|
| `$7E0998` | Estado del juego | `01` título, `02` opciones, `04` selección de archivo, `06`/`07` carga, `08` juego, `09` tocando una puerta, `0B` cruzándola, `0C` oscureciendo para pausar, `0D`/`0E` cargando la pantalla de pausa, `0F` pantalla de pausa, `10` oscureciéndola, `11` cargando la sala, `12` aclarando hasta el juego, `1E` intro, `1F` partida nueva, `20`-`22` huida de Ceres → Zebes |
| `$7E0727` | Página de la pantalla de pausa | `00` mapa, `01` equipo, `02`-`04` entre ellas |
| `$7E079B` | Sala (16 bits, su cabecera en el banco `$8F`) | `$DF45` la primera sala de Ceres, `$91F8` el lugar de aterrizaje de Crateria, `$92FD` la sala a su izquierda |
| `$7E079F` | Zona | `00` Crateria, `06` estación Ceres |
| `$7E09C2` / `$7E09C4` | Energía / máximo (16 bits) | `99` / `99` al empezar |

Al jugar, modo 1: BG1 es el primer plano de la sala, BG2 su fondo, BG3 el HUD por encima de la línea 31 y la niebla,
el agua o la lava de la sala por debajo (una IRQ en la línea 31 cambia la configuración). En la pantalla de pausa BG1
son las salas del mapa, BG2 la cuadrícula y el marco, BG3 otra vez el HUD (líneas 0-31), y los sprites el marcador de
Samus y los botones L/R.

## Hallazgos

- Aquí la intro se detiene tras cada página de texto hasta que se pulsa un botón (A sirve, Start no); si nadie toca
  nada, nunca llega a Ceres (probado durante 20000 fotogramas). En una consola puede que siga sola, y entonces es una
  diferencia del núcleo que conviene mirar en la consola; en cualquier caso A la pasa. Los tests pulsan A.
- En la estación Ceres no se puede pausar: Start no hace nada. Los tests se saltan la huida de Ceres poniendo el
  estado del juego a `20` (Samus llegó al ascensor): el juego hace la explosión y el viaje, aterriza la nave en Zebes
  y sigue como lo haría tras la huida (`$079F` = `00`, sala `$91F8`).
- Samus no puede pausar durante unos 10 segundos tras aterrizar, mientras sale de la nave.
- El HUD no se dibuja en unos pocos fotogramas alrededor de la pantalla de pausa (`0D`/`0E`, `11` y el primer
  fotograma de `12` cargan con la pantalla apagada); esos estados no tienen regla, así que la segunda pantalla
  conserva el último HUD en vez de parpadear.
- En la pantalla de pausa, una pulsación de R o Start de 4 fotogramas a veces se pierde; con 12 siempre funciona.
- Al encender: no suena nada hasta que empieza la intro (fotograma 369), así que no se silencia nada.
- Una captura lleva unos 170 fotogramas del núcleo oculto: 70 para abrir, el fundido, y 8 fotogramas del marcador
  parpadeando, separados 8 fotogramas.

## Tests

Hace falta tu propia ROM; las partidas no, los guiones las crean.

```bash
scripts/check.sh CARPETA_DE_ROMS super_metroid
```

| Guion | Comprueba |
|---|---|
| `new_game.txt` | Del encendido, con un archivo nuevo y la intro, hasta la estación Ceres: el estado del juego, la zona, la energía, que no hay mapa allí, las líneas del HUD |
| `zebes.txt` | Ceres saltada, la nave aterriza en Zebes: sin mapa mientras aterriza, luego el mapa de pausa; el HUD al cruzar una puerta; el mapa en la sala siguiente; las páginas de la pantalla de pausa y la vuelta al juego |

## Pendiente

- En la RG DS Plus: si una captura cada 3 segundos mantiene el juego a toda velocidad, y qué tal se ve el HUD encima
  de la parte de arriba del mapa de pausa (tapa la copia del HUD que trae el propio mapa).
- Otras regiones (Europa): su CRC32, y si sus direcciones cambian.
- Quizá la página de equipo en la segunda pantalla durante la pausa.
