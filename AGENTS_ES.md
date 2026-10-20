# AGENTS.md (español)

[English](AGENTS.md) · **Español**

Instrucciones para agentes de programación (y personas) que trabajan en SNES DS. Léelas antes de cambiar nada; lee
el [README_ES.md](README_ES.md) para saber qué es el proyecto. La versión de referencia es `AGENTS.md`; este fichero
es su traducción.

El trabajo principal para el que existen estas instrucciones: **añadir compatibilidad de doble pantalla para un juego
nuevo, un juego por rama.**

## Reglas básicas

No son negociables. Un cambio que rompa una no se fusiona, por bueno que sea el resto.

1. **Nunca datos de juegos en el repositorio.** Ni ROMs, ni partidas, ni estados, ni volcados de memoria, ni capturas
   o grabaciones de un juego, ni nada extraído de una ROM (gráficos, música, texto). Todo eso va en `out/`, que git
   ignora. Tampoco subas capturas de juegos a pull requests ni a issues: describe con palabras lo que ves. Los hashes
   CRC32 y SHA-1, las direcciones de memoria y las líneas `hash` del harness sí valen.
2. **Un juego, una rama, un directorio.** Una rama de juego solo cambia `games/<id>/`, su línea en
   [`games/games.c`](games/games.c) y la fila de ese juego en la documentación (ver
   [Terminarlo](#6-terminarlo)). Nada de `src/`, `third_party/`, `tools/` ni del directorio de otro juego.
3. **El motor no conoce ningún juego.** El código de `src/` nunca pregunta por un juego en concreto. Si un juego
   necesita algo que el motor aún no hace, se añade al motor en una rama `engine/` aparte, de forma genérica y
   desactivado por defecto (ver [Código propio de un juego](#código-propio-de-un-juego-y-optimizaciones)).
4. **El núcleo de emulación sigue idéntico bit a bit.** `scripts/check.sh` compara los hashes de la imagen, la imagen
   separada y la RAM con [`games/zelda_alttp/tests/core_check.expected`](games/zelda_alttp/tests/core_check.expected).
   Un cambio que los altere cambia lo que hace el emulador, y necesita la aprobación expresa de la mantenedora (una
   corrección de precisión, por ejemplo), junto con la referencia nueva.
5. **Todos los tests pasan antes de fusionar:** `scripts/check.sh CARPETA_DE_ROMS` con todas las ROMs que tengas, como
   mínimo A Link to the Past y el juego que añades.
6. **Dos idiomas.** Los textos de la interfaz se escriben junto a su traducción, `T("Settings", "Ajustes")`
   ([`src/i18n.h`](src/i18n.h)); el `about` de un perfil va en inglés y en español. La documentación va en inglés en
   `NOMBRE.md` y en español en `NOMBRE_ES.md`, siempre a la par.
7. **Nunca hagas push a `main`**, nunca fuerces el push de una rama que no es tuya y nunca reescribas historia ya
   publicada. Todo llega a `main` mediante un pull request revisado.

## Ramas

| Rama | Para | Puede cambiar |
|---|---|---|
| `main` | Lo publicado; siempre compila y pasa `scripts/check.sh` | Solo mediante pull requests |
| `game/<id>` | Añadir o mejorar un juego | `games/<id>/`, su línea en `games/games.c`, sus filas de documentación |
| `engine/<tema>` | Funciones del motor o del núcleo que necesita un juego: campos nuevos del perfil, hooks, parches del núcleo | `src/`, `tools/`, `third_party/` (con [PATCHES.md](third_party/lakesnes/PATCHES.md)), documentación |
| `fix/<tema>` | Correcciones fuera de un juego concreto | Lo que necesite la corrección |
| `docs/<tema>` | Solo documentación | `*.md`, `docs/` |

**`<id>` es el nombre del módulo del juego**, la misma cadena en todas partes: el directorio `games/<id>/`, el `.id`
del perfil, el símbolo `game_<id>` y la rama `game/<id>`. Es:

- letras ASCII minúsculas, dígitos y guiones bajos (tiene que ser un identificador de C válido);
- el nombre del juego, no su región: las otras regiones son más CRC en el mismo perfil si sus direcciones coinciden
  (un perfil por región, en el mismo directorio, si no);
- corto pero sin ambigüedad, con el prefijo de la saga cuando ayude: `super_metroid`, `super_mario_world`,
  `super_castlevania_iv`, `castlevania_dracula_x`, `chrono_trigger`, `final_fantasy_vi`, `secret_of_mana`.

`<tema>` es libre, en minúsculas, con las palabras unidas por guiones: `engine/split-hooks`, `fix/menu-button-release`.

Por qué `game/` y no `agent/`: el prefijo dice qué *contiene* la rama, no quién la escribió. Agentes y personas siguen
el mismo camino, y quién escribió un commit queda en el propio commit (ver [Commits](#commits-y-pull-requests)).

**La vida de una rama:**

1. Comprueba que nadie esté ya con ese juego: `git ls-remote --heads origin 'game/*'` y los pull requests abiertos.
2. Crea la rama desde el `main` más reciente: `git switch -c game/<id> origin/main`.
3. Haz push pronto y abre un pull request en **borrador**, para que el juego quede reservado.
4. Mantenla al día con `git rebase origin/main` (solo tu rama). Si necesita un cambio en el motor, ese cambio va antes
   en su propia rama `engine/` y su pull request; cuando se fusione, rebasa sobre `main`.
5. Cuando `scripts/check.sh` pase y la documentación esté hecha, marca el pull request como listo para revisar.
6. Tras la fusión (squash), la rama se borra.

## Preparación

```bash
# Windows: MSYS2 UCRT64 (gcc, cmake, ninja, SDL2); Linux: build-essential cmake ninja-build libsdl2-dev
cmake -S . -B build -G Ninja
cmake --build build
scripts/check.sh /ruta/a/tus/roms        # los tests de todos los juegos
```

- `build/harness` (`.exe` en Windows) es el harness sin ventana; `build/snes-ds`, la app. Sin SDL2 solo se compila el
  harness, que es todo lo que necesita la investigación de un juego.
- Las ROMs las da la mantenedora o quien ejecute al agente, en una ruta que te indicará; nunca las busques en
  internet y nunca las copies al repositorio. Si no tienes la ROM del juego que te piden añadir, para y dilo.
- La app se puede manejar sin ventana, para comprobar la interfaz: `SDL_VIDEODRIVER=offscreen` con `--script` (ver el
  principio de [src/main.c](src/main.c)).
- La consola (una Anbernic RG DS Plus con ROCKNIX) solo la tiene la mantenedora. Lo que solo se pueda comprobar allí
  va en la descripción del pull request.

## Añadir un juego

### 1. Conocer la ROM

Pásale cualquier guion del harness: la primera línea muestra el título del header, el CRC32 por el que se guían los
perfiles (sin header de copiador) y si ya hay un perfil que encaje. Apunta la región y la versión; mejor el volcado
limpio más habitual.

### 2. Investigarlo con el harness

El harness ([tools/harness.c](tools/harness.c), comandos al principio) ejecuta el juego fotograma a fotograma desde
un guion. Mientras exploras, escribe tus guiones en `out/`; los que merezcan la pena se convierten en los tests
(paso 4).

- **Llega a las pantallas que importan** (juego normal, cada menú, el mapa) con `wait`, `hold`, `tap`, `until` y
  `untiltap`, y guarda (`save`) un estado en cada una para volver directamente con `load`.
- **Averigua qué capa lleva qué** con `layershot NOMBRE`: una imagen por capa (`_bg1` … `_bg4`, `_obj`), para ver si el
  HUD o un menú tienen una capa para ellos solos. Después, `split MÁSCARA Y0 Y1` con `splitbox` y `splitshot` para
  encontrar las líneas exactas.
- **Encuentra la memoria que dice qué pantalla hay:** `watch DIR LONG` imprime un rango cada vez que cambia; `peek`,
  `dumpram` y comparar volcados entre dos estados hacen el resto. Busca primero el byte de modo o estado principal
  del juego; los desensamblados y mapas de RAM (Data Crystal y similares) son buenos puntos de partida, pero
  **comprueba cada dirección tú mismo** con el harness, y di en el README cómo.
- **El mapa, si el juego tiene:** las pulsaciones que lo abren desde el juego, la memoria que dice que está abierto y
  los valores que cambian cuando el jugador cambia de sala o de zona. `map NOMBRE` ejecuta la captura del perfil en
  un núcleo oculto, como hace la app, e imprime el resultado.
- **Sonido:** `audio N NOMBRE` graba el sonido en un WAV e imprime los puertos de la APU y la configuración del DSP
  cuando empieza un sonido, por si el juego hace un ruido que el perfil tenga que silenciar (`bootMuteFrames`).

Cosas que ya han importado antes: una capa puede llevar a la vez el HUD y parte del mundo (se separa por líneas, no
por capa); el HUD puede moverse en las transiciones; los cuadros de texto pueden compartir la capa del HUD; los mapas
pueden aparecer tras unos fotogramas en negro.

### 3. Escribir el perfil

Copia [`docs/game-template/`](docs/game-template) a `games/<id>/` y rellena `profile.c`. Los campos se describen en
[`src/profiles.h`](src/profiles.h):

| Campo | Qué es |
|---|---|
| `id`, `title`, `shortTitle`, `about[LANG_EN]`, `about[LANG_ES]` | Nombres, y una frase sobre lo que muestra la segunda pantalla |
| `crc32[]` | Las ROMs a las que corresponde (hasta 4) |
| `split[]` | Hasta 4 reglas, gana la primera que encaje: sondas de memoria (`PROBE`, `PROBE_EITHER`) → qué capas y líneas van a la segunda pantalla, como franja de HUD (`SPLIT_HUD`, sobre el mapa) o pantalla completa (`SPLIT_FULL`, un menú) |
| `hudShowY0/Y1` | Las líneas de una franja de HUD que merece la pena mostrar |
| `bootMuteFrames` | Fotogramas de sonido que silenciar tras el encendido |
| `map` | Cómo abre el mapa el núcleo oculto, cuándo puede empezar una captura (`live`), cómo saber que está abierto (`shown`), qué cambios lanzan una captura nueva (`room`) y cómo reconocer una imagen estable y no vacía |

Comenta cada valor con qué es y cómo se encontró, como hace `games/zelda_alttp/profile.c`. Después regístralo en
[`games/games.c`](games/games.c): `extern const Profile game_<id>;` y `&game_<id>,` en la lista. CMake recoge
`games/*/*.c` por sí solo.

### 4. Escribir sus tests

En `games/<id>/tests/`:

- Guiones que van del encendido a cada pantalla que trata el perfil y comprueban por el camino, con `expect`, los
  valores de memoria en los que se basa el perfil, y con `map NOMBRE ok` (o `no_map` donde no lo haya) la captura del
  mapa. Salen con 1 si algo no cuadra. Usa `untiltap` en vez de esperas fijas donde los tiempos puedan variar.
- No deben necesitar ninguna partida guardada: crea el progreso que necesiten (escribir con `poke` en la zona de la
  partida vale, como hace `games/zelda_alttp/tests/sanctuary.txt`).
- `tests/order` los lista en el orden en que los ejecuta `scripts/check.sh`; los posteriores pueden cargar (`load`)
  los estados que guardan (`save`) los anteriores.

### 5. Probarlo en la app

`build/snes-ds --rom-dir=CARPETA_DE_ROMS` (o con `--windowed --layout=side --screen=1024x768` para la disposición de
la consola) y juega lo que cubre el perfil: el HUD o el menú en la segunda pantalla, que no falte nada en ninguna de
las dos y que el mapa se actualice. Comprueba el sonido del arranque. Las capturas que hagas, en `out/`.

### 6. Terminarlo

- **Su icono** para la lista de juegos: `games/<id>/icon.py` (desde la plantilla) describe un dibujo original con
  formas sencillas, y `python tools/icon.py games/<id>/icon.py` lo convierte en `games/<id>/icon.png` (512x384);
  sube los dos. Nunca el logo del juego, su carátula, sus sprites ni nada suyo (regla 1): una escena u objeto que
  recuerde al juego, dibujado desde cero. La carátula que haya descargado el frontend del jugador (la de
  EmulationStation) sigue yendo primero.
- `games/<id>/README.md` y `README_ES.md` (desde la plantilla): la ROM, lo que muestra la segunda pantalla, el mapa
  de memoria con cómo se comprobó cada valor, los hallazgos, los tests y lo que falta.
- Añade la fila del juego a "Juegos compatibles" en [README.md](README.md) y [README_ES.md](README_ES.md), y márcalo
  en la fase 6 de [ROADMAP.md](ROADMAP.md) y [ROADMAP_ES.md](ROADMAP_ES.md). Esas filas son las únicas líneas
  compartidas que toca una rama de juego, así que las fusiones son triviales.
- `scripts/check.sh CARPETA_DE_ROMS` pasa, con la ROM de Zelda también allí.

## Código propio de un juego y optimizaciones

**Primero, datos.** La mayoría de los juegos solo necesitan un perfil, y un perfil es fácil de revisar: prefiérelo
siempre que pueda expresar lo que necesitas, aunque haga falta alguna regla más.

Cuando no puede (una segunda pantalla que se dibuja a su manera, una decisión que las sondas de memoria no pueden
expresar, un atajo de velocidad que solo sirve para un juego):

1. **El código del juego va en su propio directorio,** `games/<id>/*.c` (CMake lo compila), y solo se llega a él a
   través de su perfil.
2. **El motor recibe un hook genérico para ello, primero en una rama `engine/<tema>`:** un campo opcional de `Profile`
   (un puntero a función o un indicador) que el motor llama o respeta cuando está puesto. Sin el campo, nada cambia
   para ningún otro juego: `scripts/check.sh` lo demuestra, hashes incluidos. Documenta el hook en `src/profiles.h`,
   y hazlo tan pequeño como lo necesite el primer juego que lo use; no lo diseñes para juegos en los que no trabaja
   nadie.
3. **Las optimizaciones que cambian lo que hace el emulador** (saltarse trabajo que un juego nunca necesita, por
   ejemplo) son indicadores opcionales del perfil, medidas con `bench` en el harness y nunca activadas por defecto.
   Las optimizaciones que lo mantienen idéntico bit a bit son para todos: van en una rama `engine/`.
4. **Los cambios en el núcleo** (`third_party/lakesnes`) se listan en [PATCHES.md](third_party/lakesnes/PATCHES.md),
   se marcan con `snes-ds` en el código y mantienen los hashes idénticos (regla básica 4). Un cambio en lo que guarda
   un estado sube además `stateVersion` en `snes_other.c`, para que los estados antiguos se rechacen en vez de
   cargarse mal.
5. **Los ficheros que conserva el jugador** (partidas, estados, ajustes) se escriben con
   [`src/fileutil.h`](src/fileutil.h), nunca con un simple `fopen`: un apagado no debe dejar un fichero a medias.

## Estilo de código

- C11. Tabuladores en el código propio del proyecto (LakeSnes mantiene su estilo de dos espacios). `-Wall -Wextra`
  tiene que seguir sin avisos.
- Sigue el código de alrededor: funciones cortas, nombres que dicen qué son las cosas, comentarios que dicen por qué.
  Cada fichero empieza con un comentario sobre para qué sirve.
- Ninguna dependencia nueva, ni nada descargado al compilar, sin la aprobación de la mantenedora. El código de
  terceros va en `third_party/` con su licencia.
- Las rutas son UTF-8 en todas partes (también en Windows: los ejecutables llevan un manifiesto para ello).

## Commits y pull requests

- Un cambio lógico por commit. La línea de resumen va en inglés, con 72 caracteres como máximo, precedida del id del
  juego o del área: `super_metroid: split the HUD's minimap`, `engine: add a split hook to Profile`.
- Si eres un agente, dilo en un trailer, por ejemplo `Co-Authored-By: <nombre del agente> <dirección>`.
- Título del pull request: `game: <Título completo> (<región>)` para un juego; si no, como el resumen de un commit. La
  descripción dice:
  - qué muestra la segunda pantalla, y dónde;
  - la región, la versión y el CRC32 de la ROM;
  - las direcciones de memoria usadas y cómo se comprobó cada una;
  - la salida de `scripts/check.sh CARPETA_DE_ROMS`;
  - lo que solo puede confirmarse en la consola;
  - con palabras, no con imágenes, lo que parezca estar mal.

## Juegos candidatos

De la fase 6 de la [hoja de ruta](ROADMAP_ES.md). Los nombres de rama siguen las reglas de arriba.

| Juego | Rama | Segunda pantalla |
|---|---|---|
| Super Metroid | `game/super_metroid` | El mapa de pausa; el minimapa del HUD |
| Super Mario World | `game/super_mario_world` | El mapa del mundo; el HUD |
| Secret of Mana | `game/secret_of_mana` | Los menús en anillo; el mapa del mundo en Mode 7 |
| Chrono Trigger | `game/chrono_trigger` | Los menús; el mapa del mundo |
| Final Fantasy VI | `game/final_fantasy_vi` | Los menús; el mapa del mundo en Mode 7 |
| Super Castlevania IV | `game/super_castlevania_iv` | El HUD |
| Castlevania: Dracula X | `game/castlevania_dracula_x` | El HUD |

## Dónde está cada cosa

| Ruta | |
|---|---|
| [`src/profiles.h`](src/profiles.h) | Los campos del perfil: la referencia del paso 3 |
| [`src/screen2.c`](src/screen2.c), [`src/mapcap.c`](src/mapcap.c) | Cómo se usan los perfiles: la segunda pantalla, la captura del mapa |
| [`src/core.h`](src/core.h) | Todo lo que el resto del código puede pedirle al emulador |
| [`tools/harness.c`](tools/harness.c) | El harness y sus comandos |
| [`scripts/check.sh`](scripts/check.sh) | Los tests de todos los juegos, y de la app (`tools/app_tests/`) |
| [`games/zelda_alttp/`](games/zelda_alttp) | Un ejemplo completo: perfil, README, tests |
| [`docs/game-template/`](docs/game-template) | El punto de partida de un juego nuevo |
