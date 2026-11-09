# Micrófono de Xbox Cloud Gaming — versión experimental

Esta modificación añade a LunarNX una opción **Micrófono** en **Ajustes de Xbox**.
Está desactivada por defecto y **solo afecta a Xbox Cloud Gaming**. No altera
Xbox Remote Play ni PlayStation Remote Play.

## Requisitos

- Nintendo Switch normal con Atmosphère, ejecutando Homebrew Menu en modo título
  (mantener R al abrir un juego, para disponer de toda la memoria).
- Auriculares con micrófono conectados al puerto de 3,5 mm de la Switch.
- Una compilación `.nro` generada a partir de esta modificación.

## Instalación y prueba

1. Guarda una copia de tu LunarNX actual y de su configuración.
2. Copia el nuevo archivo `LunarNX.nro` a
   `sdmc:/switch/LunarNX/LunarNX.nro`.
3. Abre LunarNX y entra en **Xbox > Ajustes**.
4. Activa **Xbox Cloud Gaming voice chat > Microphone**.
5. Cierra los ajustes e inicia una sesión *nueva* de Xbox Cloud Gaming.
6. Entra en un juego con chat de grupo y pregunta a un compañero si te oye.

## Si no te oyen

Revisa `sdmc:/switch/LunarNX/lunarnx.log` y extrae únicamente las líneas
`[cloud-mic]` o `mic|`. **No compartas el registro completo**: puede contener
credenciales, identificadores y datos de red.

- `SDP upstream permitted=false`: el servicio no aceptó enviar voz en esta sesión.
- `capture unavailable`: libnx no pudo iniciar el micrófono.
- `mic| encoded>0 sent>0 peak>0`: se capturó y transmitió voz; puede quedar un
  problema de chat del juego o del servicio.
- `mic| peak=0`: entrada silenciada o el hardware no entregó muestras.
- `send_failed>0`: libpeer no pudo emitir algunos paquetes Opus.

La compilación y las pruebas automatizadas comprueban el código; solo una
prueba en la Switch física con otra persona confirma que la voz se oye.

## Para compilar

El repositorio incluye un workflow de GitHub Actions llamado
`Build Nintendo Switch application`. Al ejecutarlo desde un fork que tenga
el parche, descargue el artefacto `LunarNX.nro` del job `build-switch`.
También puede usarse Docker con la imagen `devkitpro/devkita64:20251117`
siguiendo `AGENTS.md` y `scripts/build_switch_in_container.sh`.
