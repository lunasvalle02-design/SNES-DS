# Secret of Mana

[English](README.md) · **Español**

| | |
|---|---|
| Módulo | `secret_of_mana` ([profile.c](profile.c)) |
| ROM | EE. UU., HiROM, 2 MB, 8 KB de partida con pila |
| CRC32 | `D0176B24` |
| SHA-1 | `8133041A363E3CC68CEDEF40B49B6D20D03C505D` |
| Fuente de las direcciones | Encontradas con el harness (volcados de RAM comparados entre el juego, el menú en anillo, sus pantallas, los cuadros de texto, el título y la intro; después cada byte observado en los cambios). El mapa de RAM de Data Crystal no respondía mientras se escribía esto |

## Segunda pantalla

- **HUD** (la vida y la carga del arma de cada personaje): las líneas 199-216 de BG3 y la cara al lado, un sprite (la
  del chico en x 88-102, líneas 199-214). Baja en el campo (`$009B` = `02`) mientras el menú en anillo está cerrado
  (`$007D` = `00`), también con cuadros de texto: se dibujan arriba (líneas 9-68). El campo sigue bajo el HUD, así que
  la pantalla de juego conserva todas sus líneas. Antes de que el chico tenga la espada el juego no dibuja HUD, y la
  franja baja vacía.
- **Menú en anillo** (Y): mientras está abierto (`$007D` = `01`, `$00CC` = `00`), BG3 (el nombre del objeto arriba,
  líneas 9-36, y el HUD) y todos los sprites (el anillo, los personajes) pasan a la segunda pantalla; el campo
  oscurecido se queda en la pantalla de juego.
- **Las pantallas que abre el anillo** (Estado, Cuadrícula de acción, Mando, Ventana...; `$007D` = `01`, `$00CC` =
  `08`): ocupan toda la imagen (un dibujo en BG1, las ventanas en BG2, los personajes como sprites), así que baja
  entera y la pantalla de juego queda a oscuras hasta que se cierra el menú.
- **Mapa:** todavía ninguno. El único mapa del juego es el del mundo en Mode 7 que se ve volando en Flammie, ya
  avanzada la partida, al que los tests no llegan sin una partida guardada: `.map` sigue apagado.

## Memoria (WRAM, comprobada con el harness)

| Dirección | Qué es | Valores vistos |
|---|---|---|
| `$7E009B` | Tipo de pantalla | `00` el logo, el título, Game Select, el nombre (y unos 6 fotogramas entre dos mapas), `01` los rótulos de la intro, `02` el campo: el juego, sus cuadros de texto, el menú en anillo y sus pantallas |
| `$7E007D` | Menú en anillo abierto | `01` desde el fotograma en que se dibuja el anillo hasta que se cierra, también en las pantallas que abre; `00` si no |
| `$7E00CC` | Estado del menú | `00` jugando y en el anillo, `04` el fotograma en que se abre el anillo, `08` una pantalla abierta desde el anillo (también algunos cuadros de texto, con `$007D` = `00`), `0C` mientras esa pantalla se cierra (a oscuras), luego `00` un fotograma después de `$007D` |
| `$7E005A` | Menú en anillo (otra señal) | `01` jugando, `11` con el anillo abierto; no se usa |

Al jugar, BG1 y BG2 son el campo (suelo y agua, árboles), BG3 tiene el HUD abajo y los cuadros de texto arriba, y los
sprites son los personajes, los enemigos y las caras del HUD. Los rótulos de la intro también son BG3.

## Hallazgos

- **El texto en alta resolución cuesta leerlo**, en la pantalla de juego y en la segunda: el nombre, Game Select y las
  pantallas que abre el anillo dibujan sus letras en el modo de 512 píxeles de la SNES, y el núcleo se queda con una
  de cada dos columnas (ver `third_party/lakesnes/ppu.c`, "only the main screen's pixel"). Es cosa del motor, igual
  para cualquier juego que use ese modo; los cuadros de diálogo y los nombres del anillo usan letras normales y se
  leen bien.
- El chico nunca bajó de la línea 176 (andando hasta el borde de abajo de tres mapas): los sprites no llegan a las
  líneas del HUD, así que solo bajan con él sus caras. Un enemigo que llegara tan abajo bajaría también.
- Los botones: B es atacar y "aceptar", Y el menú en anillo, Arriba y Abajo cambian de anillo; X no mostró nada en los
  primeros mapas.
- Al encender: el primer sonido es la música de la intro con el logo de Squaresoft (fotograma 122), nada que silenciar.
- El anillo se cierra desde una de sus pantallas directamente al juego con Y: `$00CC` pasa a `0C` mientras la pantalla
  se funde, `$007D` a `00`, y `$00CC` lo sigue un fotograma después.

## Tests

Hace falta tu propia ROM; las partidas no, los scripts las crean.

```bash
scripts/check.sh ROMDIR secret_of_mana
```

| Script | Comprueba |
|---|---|
| `new_game.txt` | Desde el encendido, por el título, una partida nueva, el nombre, la intro y la caída al río, hasta la espada en la piedra (andando con pulsaciones fijas): `$009B` por el camino, el anillo cerrado, las líneas del HUD y su cara |
| `menus.txt` | El menú en anillo (`$007D`, `$00CC`, sus capas), la pantalla de Estado que abre (`$00CC` = `08`, sus ventanas en BG2), cerrarlo todo, y un cuadro de texto arriba, lejos del HUD |

No hay comprobación de `map`: la orden `map` del harness rechaza un juego sin mapa.

## Pendiente

- El mapa del mundo (Mode 7, volando en Flammie), con un test que llegue allí con pokes, y los menús que solo se ven
  más tarde: tiendas, los anillos de magia, las pantallas de la chica y del duende.
- El HUD con tres personajes (más ancho en las mismas líneas, por lo que muestra uno) y sus cambios de color (veneno,
  poca vida).
- El texto en alta resolución, como cambio del motor (mezclar cada par de columnas en vez de quedarse con una).
- En la RG DS Plus: cómo se ven la franja del HUD y el anillo en la segunda pantalla.
- Otras regiones (Europa, el *Seiken Densetsu 2* japonés): sus CRC32 y direcciones.
