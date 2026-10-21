# Registro de cambios

[English](CHANGELOG.md) · **Español**

Lo que trajo cada versión de SNES DS. Las versiones siguen el [versionado semántico](https://semver.org/lang/es/);
antes de la 1.0 todo puede cambiar todavía (ver [lo que promete la 1.0](ROADMAP_ES.md#camino-a-la-10)). Las
descargas están en la [página de releases](https://github.com/lunasvalle02-design/SNES-DS/releases).

## 1.1.2

### Añadido
- **Super Mario World 2: Yoshi's Island** (EE. UU., Rev 1): la segunda pantalla muestra la pantalla de pausa, siempre
  al día, sin pausar: las estrellas, monedas rojas y flores del nivel, los puntos, las vidas, monedas y objetos,
  capturados en segundo plano. Con su propio icono en la lista de juegos. En la RG DS Plus todavía tiene algunos
  tirones en los niveles (lo que queda está en su README).
- **El Super FX** (GSU-1 y GSU-2) en el núcleo de emulación, que LakeSnes no tenía: los juegos de Super FX (Star Fox,
  Yoshi's Island, Doom…) ya no se quedan en una pantalla negra. Va por detrás de la CPU, a tramos.
- La línea `stats:` del log dice cuánto tarda el hilo de dibujo por frame, y cuánto lo espera la emulación.

### Cambiado
- **La PPU decodifica los tiles de los fondos con una tabla**: alrededor de un tercio menos de tiempo de dibujo, en
  todos los juegos, con las mismas imágenes.
- **Las partidas guardadas se escriben en un hilo propio**, cuando dejan de cambiar (o una vez por minuto si no paran
  nunca, como con la memoria del Super FX), y al abrir el menú: la emulación ya no espera a la tarjeta SD.

## 1.1.1

### Añadido
- **Super Mario World** (EE. UU.): la segunda pantalla muestra el mapa del mundo tal como Mario lo dejó, capturado en
  segundo plano, y en los niveles la barra de estado (vidas, tiempo, monedas, puntos) encima; el objeto de reserva se
  queda en la pantalla de juego. Con su propio icono en la lista de juegos.
- Para los perfiles: la captura del mapa puede dejar fuera capas en algunas líneas (`hideLayers`, `hideY0`/`hideY1`
  en `MapSpec`), para un mapa con texto propio donde se dibuja el HUD encima, como las vidas y el nombre del nivel de
  Super Mario World.

## 1.1.0

### Añadido
- **Una imagen tras el HUD**: cuando la segunda pantalla solo tiene el HUD, o nada (un juego sin mapa, como Super
  Castlevania IV, un juego sin perfil, o antes de la primera captura del mapa), la carátula o el icono del juego
  llena el resto, oscurecido para que el HUD se lea. Una imagen propia va en
  `snes-ds/backgrounds/<id o nombre de la ROM>.png` (o `.jpg`). Activado por defecto; Ajustes → Imagen tras el HUD lo
  desactiva.

## 1.0.0

La primera versión estable: la 0.6.0, usada en la consola varios días sin problemas. A partir de aquí se cumple
[lo que promete la 1.0](ROADMAP_ES.md#camino-a-la-10) (el formato de `snes-ds.cfg`, que las partidas y los estados
sigan cargando, las opciones de línea de comandos y el formato de los perfiles); romper algo de eso sería una 2.0.
Los arreglos llegan como 1.0.x.

- Las capturas del README muestran la interfaz actual: los iconos de los juegos y la página de controles.

## 0.6.0 — la candidata a 1.0

### Añadido
- **Controles en los ajustes** (Ajustes → Controles): cada botón de la SNES (B, A, Y, X, L, R, Start, Select) y un
  botón extra para el menú pueden pasar a cualquier botón de la consola, un gatillo o una tecla, pulsándolo. Si otro
  ya lo tenía, se intercambian; Restablecer los deja todos como venían. En la RG DS Plus, así Select tiene un botón que
  llega al juego.
- **Un icono para cada juego** en la lista de juegos, un dibujo original por juego (la carátula que haya descargado
  EmulationStation sigue yendo primero). `tools/icon.py` los dibuja a partir de una descripción corta; los juegos
  nuevos traen el suyo (AGENTS.md).
- **CI en GitHub Actions**: cada pull request compila para PC (sin avisos) y para ROCKNIX; una etiqueta `v*` sube el
  paquete de ROCKNIX a su release.
- Las instrucciones de la consola en inglés (`README.txt` en el zip), con las españolas al lado (`README_ES.txt`).
- Este registro de cambios.

### Cambiado
- **A Link to the Past: el HUD se queda en la pantalla del juego**; la de abajo muestra solo el mapa en vivo, que se
  lee mejor sobre los mapas de las mazmorras.

### Comprobado en la consola
- Suspender y reanudar durante una partida.
- La página de controles nueva y los iconos.

## 0.5.0

### Añadido
- **Super Metroid** (Japón y EE. UU., v1.0): el mapa de pausa de la zona en la que está Samus, que se actualiza
  mientras explora, con el HUD (energía, munición, minimapa) encima, en la pantalla de abajo.
- **Super Castlevania IV** (EE. UU., v1.0): el HUD en la pantalla de abajo, al jugar y en pausa.
- Los perfiles pueden recortar la pantalla del juego mientras su HUD está en la segunda (`gameShowY0/Y1`), así la sala
  de Super Metroid llena la pantalla de arriba.

## 0.4.0-dev
- Comprobada en la consola: 60 fps sin cortes de audio; el menú se abre con L3 o el botón R (inicio).
- Partidas, estados y ajustes escritos de forma segura: un apagado no puede corromperlos.
- El log de velocidad ya no cuenta el tiempo en el menú.

## 0.3.0-dev
- La interfaz táctil, en inglés y español: la lista de juegos, el menú de pausa, cuatro ranuras de estado con su
  captura, los ajustes. El menú también se abre tocando la pantalla de abajo o con cualquier botón libre.
- Corregido: un ruido breve al arrancar un juego.

## 0.2.0-dev
- El núcleo unas 3,7 veces más rápido (idéntico bit a bit); captura del mapa más barata; L1 + Start + Select para
  salir.

## 0.1.0-dev
- La primera versión para ROCKNIX, con A Link to the Past. Medido en la consola: 18–39 ms por fotograma emulado
  (25–50 fps), cortes de audio.
