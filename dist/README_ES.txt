SNES DS para ROCKNIX (Anbernic RG DS Plus / RG DS)
==================================================
(In English: README.txt)

Instalar
--------
Descomprime el zip en /storage/roms/ports/. Quedará:
  /storage/roms/ports/SNES DS.sh       la entrada en Ports
  /storage/roms/ports/snes-ds/         el programa

Las ROMs van donde siempre: /storage/roms/snes/ (.sfc, .smc o .zip, también
en subcarpetas). "SNES DS" en Ports abre la lista de juegos de esa carpeta
que tienen perfil de doble pantalla (por ahora, The Legend of Zelda: A Link
to the Past, USA; Super Metroid, Japón/USA; Super Castlevania IV, USA). Si
EmulationStation ya tiene carátulas descargadas (gamelist.xml, images/), la
lista las usa; si no, cada juego muestra su icono.
Si hace falta reiniciar EmulationStation para que salga en Ports, hazlo.

Desde una terminal (SSH) también se puede abrir un juego directamente:
  /storage/roms/ports/snes-ds/launch.sh "/storage/roms/snes/Juego.sfc" [opciones]

La lista de juegos
------------------
En la pantalla de abajo (táctil): toca un juego para jugarlo. Con los
botones: la cruceta elige, A juega, X abre los ajustes; arriba del todo
están los ajustes y el botón de salir.

El menú (en partida)
--------------------
Pulsar el stick izquierdo (L3) durante la partida pausa el juego y abre
el menú en la pantalla de abajo: continuar, guardar y cargar estado (4
ranuras, con su captura), ajustes, salir del juego (a la lista) o salir de
SNES DS. También lo abren el botón R (el de inicio), tocar la pantalla de
abajo, R3 y Select + Start. B, L3 o R vuelven al juego.

Si el botón de menú no hace nada, snes-ds.log apunta los dispositivos de
entrada y las primeras pulsaciones ("input: ..."): abre un issue en GitHub
con él (o prueba tú --menu-button=NOMBRE en launch.sh).

Ajustes
-------
Formato de imagen (original / 4:3), intercambiar pantallas, HUD y menús
abajo, mapa en la pantalla de abajo, volumen, mostrar FPS, idioma y
controles. Se guardan en snes-ds/snes-ds.cfg y valen para todos los juegos.

Controles: elige un botón de la SNES (B, A, Y, X, L, R, Start, Select o el
menú), pulsa A y luego el botón de la consola que quieras para él (vale un
gatillo). Si otro botón ya lo tenía, se intercambian. Si un botón no hace
lo que debe (Select, por ejemplo), asígnalo aquí. "Restablecer" lo deja
todo como venía.

La interfaz arranca en inglés. Para ponerla en español: Settings, la
última fila (Language), Español.

Controles en partida
--------------------
Los del SNES, por posición: abajo = B, derecha = A, arriba = X, izquierda = Y
(salvo los que cambies en Ajustes > Controles).
Atajos (Select mantenido + otro botón):
  Select + Start      el menú        Select + X   intercambiar pantallas
  L1 + Start + Select salir          Select + Y   aspecto: original / 4:3
  Select + R          guardar estado Select + A   HUD en la 2ª pantalla sí/no
  Select + L          cargar estado  Select + Derecha  avance rápido sí/no
Guardar y cargar estado con atajo usan la ranura 1, y ocurren al soltar R
o L. Select solo sigue llegando al juego al soltarlo (o si lo mantienes
medio segundo).

Opciones útiles (en launch.sh, después de la ROM si la hay)
-----------------------------------------------------------
  --no-map           sin el núcleo oculto del mapa, diga lo que diga el ajuste
  --no-threads       dibujar en el mismo hilo que la emulación (por defecto
                     se dibuja en otro núcleo de la CPU, que es más rápido)
  --layout=stack     si el juego y la 2ª pantalla salen mal colocados
  --menu-button=X    el botón del menú (nombre de SDL, o joy:N)

Partidas
--------
La partida del juego (.srm) se guarda junto a la ROM, en el mismo formato
que RetroArch. Los estados guardados van a snes-ds/states/.

Rendimiento: qué mirar
----------------------
1. snes-ds/snes-ds.log: cada 10 segundos una línea como
     stats: 60.0 fps, emulation 9.80 ms/frame (max 14.20), drawing 1.10 ms,
            audio underruns 0, map captures 12 (850 ms avg, 1200 max)
   - fps: 60 es velocidad completa.
   - emulation: tiene que estar por debajo de 16,6 ms.
   - audio underruns: cortes de sonido (deberían ser 0).
   - map captures: cuánto tarda el núcleo oculto en sacar el mapa.
2. snes-ds/bench.sh: mide la velocidad del emulador sin ventana y la deja en
   snes-ds/bench.log. Ejecútalo por SSH:
     /storage/roms/ports/snes-ds/bench.sh
Para un problema de velocidad, adjunta los dos ficheros (snes-ds.log y
bench.log) a un issue en GitHub.

Si no arranca, snes-ds.log dice por qué (y qué hizo screens.sh con las
pantallas).

Licencia: GPL-3.0 o posterior (LICENSE). Licencias de terceros en licenses/.
https://github.com/lunasvalle02-design/SNES-DS
