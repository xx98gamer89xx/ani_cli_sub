#!/bin/bash
wanted_server="PDrain"
download_dir="/home/donar"
declare -A servers
mode="stream"

while getopts "dh" option; do
  case $option in
    d)
      mode="download"
      ;;
    h)
      echo "Ejecuta sin argumentos para stremear y con -d para descargar en el download_dir especificado en el .sh (${download_dir})"
      ;;
  esac
done

search_anime() 
    {
    query="$1"
    result=$(curl -s 'https://animeav1.com/api/search' \
        --compressed \
        -X POST \
        -H 'User-Agent: Mozilla/5.0' \
        -H 'Content-Type: application/json' \
        --data-raw "{\"query\":\"$query\"}")
    mapfile -t avaliable_animes < <(echo "$result" | jq -r '.[].slug')
    }
get_episodes()
    {
    link="https://animeav1.com/media/${avaliable_animes["$slug_index"]}"
    episodes_number=$(curl -s "$link" | grep -o "href=\"/media/${avaliable_animes["$slug_index"]}/" | grep -c "href")
    }
get_episode_link()
    {
    episode_page_link="https://animeav1.com/media/${avaliable_animes["$slug_index"]}/${episode_index}"
    episode_data=$(curl -s "$episode_page_link" | grep -m2 "data" | tail -n1 | grep -o "SUB.*")
    servers_list=$(echo "${episode_data//\}/\}$'\n'}" | grep -o "server.*")
    while IFS= read -r line; do
        server=${line#server:\"}
        server=${server%%\"*}

        url=${line#*url:\"}
        url=${url%\"*}

        servers["$server"]="$url"
    done <<< "$servers_list"
    }
stream_episode()
    {
    ## Hay que implementar la opción de distintos servidores, de momento, solo PDrain por su sencillez
    if [ "$wanted_server" = "PDrain" ]; then
        embedded_link="${servers[PDrain]}"
        file_id="${embedded_link##*/}"
        file_link="https://pixeldrain.com/api/file/${file_id}"
        mpv "$file_link"
    fi
    }
download_episode()
    {
    if [ "$wanted_server" = "PDrain" ]; then
        embedded_link="${servers[PDrain]}"
        file_id="${embedded_link##*/}"
        file_link="https://pixeldrain.com/api/file/${file_id}"
        wget "$file_link" -O "${download_dir}/${avaliable_animes[${slug_index}]-Ep${episode_index}.mp4}"
    fi
    }
menu()
    {
    echo "Introduce tu búsqueda: "
    read search
    search_anime "$search"
    clear
    echo "Elige entre los disponibles: "
    for (( i=0; i<${#avaliable_animes[@]}; i++ )); do
        echo "$(( i + 1 )). ${avaliable_animes[${i}]}"
    done
    read anime_index
    slug_index=$(( anime_index - 1 ))
    get_episodes    
    clear
    echo "Elige episodio (${episodes_number}): "
    read episode_index
    get_episode_link
    if [ mode="strem" ]; then
        stream_episode
    elif [ mode="download" ]; then
        download_episode
    else
        echo "Invalid mode error"
    fi
}       
menu    
    
