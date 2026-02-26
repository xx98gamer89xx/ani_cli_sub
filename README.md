# ani_cli_sub
Derivated from https://github.com/pystardust/ani-cli (terminal program to see anime sub/dub on english), this project aims to allow not english fluent people to enjoy anime too using opensubtitles API. 
# How to use
Clone the repository.

## Change the enviromental variables:
### main.sh
- anime_path: global path to the folder where the scripts are located
### subs_embedder_lib
- api_key: your opensusbtitles API key (you can get one for personal use freely on https://www.opensubtitles.com/)
- api_token: your opensubtitles API token (you can generate one on https://www.opensubtitles.com/)
- sub_language: language you want the anime to be sub on (example for Spanish: language="es")

## Execute ./main.sh
(You have to give it execution permission)
```
sudo chmod +x main.sh
./main.sh
```

- Introduce the anime you want to search (example Frieren).
- Select one of the given options
- Select the episode you want to see
- Give the season number (for opensubtitles to search the subtitles)
- It may fail searching the subs, if that's the case, introduce the episode number like the anime had only one season (for example Frieren first season has 28 episodes, so the second episode of the second season would be 30)
- Now there will be a new .mp4 file on the directory you cloned the repo. Enjoy It!

# Helping
I would like to improve this repo over time so if you have some ideas or wanna colaborate, please contact me!
