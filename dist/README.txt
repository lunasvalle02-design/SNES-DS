snes-ds para ROCKNIX (Anbernic RG DS Plus / RG DS)
==================================================

Instalar
--------
Descomprime el zip en /storage/roms/ports/. Quedará:
  /storage/roms/ports/SNES DS.sh       la entrada en Ports
  /storage/roms/ports/snes-ds/         el programa

Las ROMs van donde siempre: /storage/roms/snes/ (.sfc, .smc o .zip).
"SNES DS" en Ports abre el primer juego de esa carpeta que tenga perfil de
segunda pantalla (por ahora, The Legend of Zelda: A Link to the Past, USA).
Si hace falta reiniciar EmulationStation para que salga en Ports, hazlo.

Desde una terminal (SSH) también se puede abrir un juego concreto:
  /storage/roms/ports/snes-ds/launch.sh "/storage/roms/snes/Juego.sfc" [opciones]

Controles
---------
Los del SNES, por posición: abajo = B, derecha = A, arriba = X, izquierda = Y.
Atajos (Select, o el botón de función/home si lo hay, mantenido + otro):
  Select + Start      salir          Select + X   intercambiar pantallas
  L1 + Start + Select salir          Select + Y   aspecto: original / 4:3
  Select + R          guardar estado Select + A   HUD en la 2ª pantalla sí/no
  Select + L          cargar estado  Select + Derecha  avance rápido sí/no
Guardar y cargar estado ocurren al soltar R o L.
Select solo sigue llegando al juego al soltarlo (o si lo mantienes medio
segundo).

Opciones útiles (después de la ROM en launch.sh)
------------------------------------------------
  --aspect=4:3       llenar los paneles (por defecto, la imagen original a
                     escala entera: 3x, con bordes)
  --no-map           sin el núcleo oculto del mapa (si va lento)
  --no-threads       dibujar en el mismo hilo que la emulación (por defecto
                     se dibuja en otro núcleo de la CPU, que es más rápido)
  --layout=stack     si el juego y la 2ª pantalla salen mal colocados

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
Pásame los dos ficheros (snes-ds.log y bench.log).

Si no arranca, snes-ds.log dice por qué (y qué hizo screens.sh con las
pantallas).
