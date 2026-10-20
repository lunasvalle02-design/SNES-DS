# Super Castlevania IV

[English](README.md) · **Español**

| | |
|---|---|
| Módulo | `super_castlevania_iv` ([profile.c](profile.c)) |
| ROM | EE. UU. v1.0, LoROM, 1 MB, sin SRAM (contraseñas) |
| CRC32 | `B64FFB12` |
| SHA-1 | `684C1DFAFF8E5999422C24D48054D96BB12DA2F4` |
| Fuente de las direcciones | Encontradas con el harness (volcados de RAM comparados entre pantallas, pokes observados en el HUD) |

## Segunda pantalla

- **HUD** (puntos, la vida de Simon y la del enemigo, el recuadro del arma secundaria, bloque, corazones, vidas,
  tiempo): las líneas 0-31 de BG3 y los sprites que hay en ellas (el icono del arma secundaria en el recuadro es un
  sprite). Sale de la pantalla de juego durante la partida (`$0032` = `04`) mientras el estado de la fase `$0070` es
  `04` aparecer, `05` jugar (también en pausa), `06` morir, `0A` recibir un golpe o `0C` caer a un foso; las 32 líneas
  de arriba de la pantalla de juego quedan con el color de fondo.
- **Mapa:** el juego no tiene; la captura del mapa está apagada.

## Memoria (WRAM, comprobada con el harness)

| Dirección | Qué es | Valores vistos |
|---|---|---|
| `$7E0032` | Modo principal | `02` logo de Konami, título y prólogo, `01` selección de partida, `03` fundido al salir de ella, `04` nombre y partida |
| `$7E0070` | Estado de la fase | `00` nombre y la escena de la puerta del castillo, `01`-`03` carga (pantalla apagada), `04` aparecer, `05` jugar, `06` morir, `0A` golpe (retroceso, Simon parpadea), `0C` caer a un foso |
| `$7E0066` | Pausa | `01` en pausa, `00` no |
| `$7E007C` | Vidas | `05` al empezar: el HUD muestra una menos (`P=04`) |
| `$7E008E` | Arma secundaria | `00` ninguna, `01` daga, `02` hacha, `03` agua bendita, `04` cruz, `05` reloj (con poke, vista en el recuadro) |
| `$7E13F0` | Tiempo (16 bits, BCD) | `$0449` al empezar la 1-1, uno menos por segundo |
| `$7E13F2` | Corazones | `05` al empezar |
| `$7E13F4` / `$7E13F6` | Vida de Simon / del enemigo | `10` (16, la barra entera) |

Al jugar, BG1 es el frente de la fase y BG2 su fondo de la línea 32 hacia abajo; BG3 solo lleva el HUD, por encima de
la línea 32.

## Hallazgos

- El HUD se dibuja en todos los estados de la fase menos el `03` (y el primer fotograma o dos del `04`): esos cargan
  con la pantalla apagada y no tienen regla, así que la segunda pantalla conserva el último HUD en vez de quedarse en
  blanco.
- La pausa no cambia `$0070`: el HUD sigue en la segunda pantalla en pausa.
- Los sprites de la fase pueden dibujarse sobre las líneas del HUD (se ve moviendo uno con poke), pero al jugar no fue
  ninguno; uno que fuera aparecería en la segunda pantalla con el HUD.
- Start durante el logo de Konami va directo a la selección de partida; un Start de más en la pantalla del nombre
  vuelve al título; si nadie toca nada en ella, también vuelve al título. Los tests esperan al título y pulsan Start
  una vez por pantalla.
- Sin vida, Simon muere al siguiente golpe, no en el acto.
- Al encender: el primer sonido es el del logo de Konami (fotograma 534), nada que silenciar.

## Tests

Hace falta tu propia ROM; las partidas no, los guiones las crean.

```bash
scripts/check.sh CARPETA_DE_ROMS super_castlevania_iv
```

| Guion | Comprueba |
|---|---|
| `new_game.txt` | Del encendido, por la selección de partida, el nombre y la escena de la puerta, hasta Simon en el bloque 1-1: los modos, corazones, vida, vidas, sin arma secundaria, las líneas del HUD |
| `hud.txt` | El HUD separado al jugar, con un arma secundaria (su sprite en el recuadro), en pausa, al morir y recargar el bloque, al caer a un foso |

No hay comprobación con `map`: la orden `map` del harness rechaza un juego sin mapa.

## Pendiente

- Los estados entre bloques y fases (la salida de un bloque, un jefe, fase superada, el mapa del recorrido entre
  fases, fin de la partida), a los que los tests aún no llegan: si alguno dibuja el HUD con un valor que no está en la
  lista, ahí el HUD se queda en la pantalla de juego.
- En la RG DS Plus: qué tal se ve la franja del HUD en la segunda pantalla, y la franja vacía arriba de la pantalla de
  juego.
- Otras regiones (*Super Castlevania IV* europeo, *Akumajō Dracula* japonés): sus CRC32 y direcciones.
