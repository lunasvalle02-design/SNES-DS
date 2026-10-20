# <Título completo>

[English](README.md) · **Español**

| | |
|---|---|
| Módulo | `<id>` ([profile.c](profile.c)) |
| ROM | <Región> v<x.y>, <LoROM/HiROM>, <tamaño>, <SRAM> |
| CRC32 | `<XXXXXXXX>` |
| SHA-1 | `<…>` |
| Fuente de las direcciones | <Desensamblados, wikis, páginas de Data Crystal… o "encontradas con el harness"> |

## Segunda pantalla

- **<HUD / menú / mapa>:** <qué capa, qué líneas, cuándo (los indicadores de memoria), qué queda en la pantalla de juego>.

## Memoria (WRAM, comprobada con el harness)

| Dirección | Qué es | Valores vistos |
|---|---|---|
| `$7E….` | <Modo principal, sala, …> | <`00` título, `01` juego…> |

<Qué capa lleva qué, en cada modo.>

## Hallazgos

<Lo que haya sorprendido: cambios de registros a mitad de fotograma, capas compartidas entre el HUD y el mundo, tiempos, sonido.>

## Tests

Hace falta tu propia ROM; las partidas no, los guiones las crean.

```bash
scripts/check.sh CARPETA_DE_ROMS <id>
```

| Guion | Comprueba |
|---|---|
| `<boot>.txt` | <Del encendido a …> |

## Pendiente

- <Lo que falta, lo que hay que comprobar en la consola.>
