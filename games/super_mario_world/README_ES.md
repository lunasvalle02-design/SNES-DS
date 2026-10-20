# Super Mario World

[English](README.md) · **Español**

| | |
|---|---|
| Módulo | `super_mario_world` ([profile.c](profile.c)) |
| ROM | EE. UU. v1.0, LoROM, 512 KB, SRAM de 2 KB |
| CRC32 | `B19ED489` |
| SHA-1 | `6B47BB75D16514B6A476AA0C73A683A2A4C18765` |
| Fuente de las direcciones | Los mapas de RAM de la comunidad (modo de juego `$0100`, el estado del mapa del mundo `$13D9`, la posición de Mario en él `$1F17`/`$1F19`, submapa `$1F11`), cada uno comprobado con el harness |

## Segunda pantalla

- **Barra de estado** (vidas, estrellas de bonus, el objeto de reserva, tiempo, monedas, puntos): BG3, dibujada en
  las líneas 9-36 de 0-36. Sale de la pantalla de juego en un nivel (`$0100` = `14`), también en pausa. El BG1, el
  BG2 y los sprites del nivel se dibujan detrás, así que la pantalla de juego conserva sus 224 líneas; el BG3 propio
  del nivel por debajo de la línea 37 (cuadros de mensaje, mareas) se queda en la pantalla de juego. Entre niveles
  (fundidos, cargas, el mapa del mundo) no hay regla, y la segunda pantalla conserva la última barra de estado.
- **Mapa:** el propio mapa del mundo. El juego no tiene un mapa que abrir desde un nivel, así que el núcleo oculto
  captura el mapa del mundo mientras Mario está parado en él (`$0100` = `0E`, `$13D9` = `03`): no hay nada que abrir,
  y la imagen se estabiliza en unos 5 fotogramas. Se vuelve a capturar cuando Mario pasa a otra casilla o a otro
  submapa, y cada 2 segundos mientras está parado (un nivel superado abre caminos nuevos). En un nivel se queda la
  última captura: la segunda pantalla muestra por dónde entró Mario, con la barra de estado encima.

## Memoria (WRAM, comprobada con el harness)

| Dirección | Qué es | Valores vistos |
|---|---|---|
| `$7E0100` | Modo de juego | `01` Nintendo Presents, `02`-`06` el título apareciendo, `07` título, `08` selección de archivo, `0A` jugadores, `0B` fundido hacia el mapa del mundo, `0C`/`0D` cargándolo y aclarándolo, `0E` mapa del mundo, `0F`/`10` fundido hacia un nivel, `11`-`13` cargándolo y aclarándolo, `14` nivel |
| `$7E13D9` | Qué hace el mapa del mundo | `0C` y `00` mientras aparece, `03` Mario parado, esperando un botón, `04` recorriendo un camino, `05` recién parado en una casilla (`03` al fotograma siguiente) |
| `$7E1F17` / `$7E1F19` | X / Y de Mario en el mapa del mundo (16 bits) | `$0068`, `$0078` en la casa de Yoshi; `$0038`, `$0088` en Yoshi's Island 1 |
| `$7E1F11` | Submapa de Mario | `01` Yoshi's Island |
| `$7E13BF` | El nivel, por su número en el mapa del mundo | `29` Yoshi's Island 1 |
| `$7E13D4` | Pausa | `01` en pausa, `00` no |
| `$7E0DBE` | Vidas | `04` al empezar: la barra de estado muestra una más (5) |
| `$7E0071` | Estado de animación de Mario | `09` muriendo (escrito a mano: el nivel se funde hacia el mapa del mundo, con una vida menos) |

En un nivel, el BG1 y el BG2 son el nivel, a toda altura; el BG3 es la barra de estado por encima de la línea 37 y la
capa 3 propia del nivel por debajo (una IRQ le cambia el scroll ahí). En el mapa del mundo, el BG1 y el BG2 son el
mapa, el BG3 su marco y la línea de arriba (las vidas de Mario y el nombre de la casilla), y Mario es un sprite.

## Hallazgos

- Los tiles de la barra de estado no llegan a la línea 37: `splitbox` sobre el BG3 de las líneas 0-36 da las líneas
  9-36 en todos los niveles probados (la intro, Yoshi's Island 1, en pausa), y el cuadro de mensaje de la intro (BG3,
  líneas 40-110) se queda en la pantalla de juego.
- El objeto de reserva es un sprite: se queda en la pantalla de juego (solo baja el BG3, así que Mario y los sprites
  del nivel, que pueden subir a las líneas de arriba cuando Mario salta o vuela, nunca se parten).
- El mapa del mundo tiene su propia línea de arriba (la cara de Mario, sus vidas y el nombre de la casilla, líneas
  17-37), sobre la que se dibuja la barra de estado en la segunda pantalla: las dos se solapan un poco (la fila de
  abajo de la barra y el cuadro de reserva sobre el nombre de la casilla). Las dos se leen; un perfil todavía no puede
  dejar líneas fuera de una captura.
- En el mapa del mundo, la segunda pantalla muestra el mismo mapa que la de juego (una captura de hace un momento),
  con la barra de estado del último nivel encima.
- Al encender: el primer sonido es el tintineo de Nintendo Presents (fotograma 88, con el logo en pantalla), nada que
  silenciar.
- La pausa necesita un momento antes de que Start la quite: los tests pulsan hasta que cambia `$13D4`.

## Tests

Hace falta tu propia ROM; las partidas guardadas no, los scripts las crean.

```bash
scripts/check.sh ROMDIR super_mario_world
```

| Script | Comprueba |
|---|---|
| `new_game.txt` | Desde el encendido, por el título, un archivo nuevo y el nivel de la intro (las líneas de la barra de estado, el cuadro de mensaje fuera de ellas, ninguna captura en un nivel) hasta el mapa del mundo en la casa de Yoshi: el modo, el estado del mapa, la posición, la captura |
| `yoshis_island_1.txt` | Una casilla nueva y su captura, y Yoshi's Island 1: la barra de estado en el nivel y en pausa, ninguna captura ahí; Mario muere, de vuelta en el mapa del mundo: una vida menos, la captura otra vez |

## Por hacer

- Las salas de jefe en Mode 7 (Iggy, Morton, Bowser…), las entradas de castillos y casas fantasma, el juego de bonus,
  los palacios de interruptores y el final, a los que los tests aún no llegan: si alguno dibuja la barra de estado en
  otro sitio, se notará.
- En la RG DS Plus: cómo se ve la barra de estado sobre la línea de arriba del mapa del mundo, y el refresco del mapa.
- Dos jugadores: si la captura sigue a Luigi en sus turnos (su posición y su submapa aún no se han comprobado).
- Otras regiones (Europa, la japonesa *Super Mario World: Super Mario Bros. 4*): sus CRC32 y direcciones.
