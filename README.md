# ani-cli-av
Herramienta de terminal desarrollada desde 0 (sin partir de ani_cli_sub o ani-cli), para scrappear la web animeav1.com, web con anime subtitulado en español.
## Uso
Actualmente, hay dos formas de usar ani-cli-av, o bien con el .sh (solo usable en sistemas linux o con bash instalado), o a través del ejecutable en c.
### Archivo sh
Para usarla simplemente clona el repositorio, da permisos de ejecución y ejecuta el sh, necesitarás componentes de bash y mpv instalado.
Una vez ejecutado el .sh:
- Busca y elige el anime que quieres ver
- Elige entre los episodios (puedes usar la búsqueda para elegir uno de los que no aparece)
Por defecto está en modo streaming, si quieres descargar el archivo en lugar de reproducirlo ejecuta con la opción -d.
### Archivo .c
Puedes elegir si compilarlo por ti mismo (necesitarás cjson, ncurses y libcurl, además de las librerías por defecto de c), o usar una versión precompilada por mí que puedes encontrar en realeses, necesita mpv instalado y añadido al PATH.
- Busca o elige el anime a ver
- Elige entre los episodios (puedes usar la búsqueda para elegir uno de los que no aparece)
Esta versión todavía no tiene modo descarga o doblado, pero esas funciones se implementarán eventualmente.
## Contribuir
Si tienes alguna sugerencia para mejorar el funcionamiento, añadir nuevas funciones y quieres compartirla o quieres modificar el código tu mismo, no dudes en hacerlo.
