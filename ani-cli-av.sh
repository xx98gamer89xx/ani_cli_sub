#!/bin/bash
wanted_server="PDrain"
download_dir="/path/to/download/"
declare -A servers
mode="stream"
dub=0

while getopts "Ddh" option; do
  case $option in
    D)
     dub=1
     ;;
    d)
      mode="download"
      ;;
    h)
      printf "Opciones: \n
-h Muestra este mensaje \n        
-D Activa el modo dub, para ver versiones dobladas (es posible que no haya version doblada, por defecto está desactivado) \n
-d Activa el modo descargar (no stremea, guarda el archivo en ${download_dir})"
      exit
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
get_episode_servers()
    {
    episode_page_link="https://animeav1.com/media/${avaliable_animes["$slug_index"]}/${episode_index}"
    episode_data_sub_dub=$(curl -s "$episode_page_link" | grep -o "SUB.*" | grep -o "downloads.*")
    if [ "$dub" -eq 0 ]; then
        episode_data="${episode_data_sub_dub%%DUB*}"
    else
        episode_data="${episode_data_sub_dub##*DUB}"
    fi
    servers_list=$(echo "${episode_data//\}/\}$'\n'}" | grep -o "server.*")
    while IFS= read -r line; do
        server=${line#server:\"}
        server=${server%%\"*}

        url=${line#*url:\"}
        url=${url%\"*}

        servers["$server"]="$url"
    done <<< "$servers_list"
    }
get_episode_link() 
    {
    if [ "$wanted_server" = "PDrain" ]; then
        embedded_link="${servers[PDrain]}"
        file_id="${embedded_link##*/}"
        file_link="https://pixeldrain.com/api/file/${file_id}"
        wget "$file_link" -O "${download_dir}/${avaliable_animes[${slug_index}]-Ep${episode_index}.mp4}"
    elif [ "$wanted_server" = "MP4Upload" ]; then
        ## Generate cookies
        curl -c "${download_dir}/.cookies.txt" \
                      -A "Mozilla/5.0" \
                      https://www.mp4upload.com/j70hobym0b7k \
                      -o page.html
        embedded_link="${servers[MP4Upload]}"
        file_id=${embedded_link##*/}
        base_link=$( curl -v "$embedded_link" \
                      -b "${download_dir}/.cookies.txt" \
                      -A "Mozilla/5.0" \
                      -H 'Content-Type: application/x-www-form-urlencoded' \
                      --data 'op=download2&id=j70hobym0b7k&rand=&referer=https%3A%2F%2Fwww.mp4upload.com%2F&method_free=Free+Download' 2>&1 | grep "location" )
        file_link="${base_link#*:}"
        file_link=$(echo $file_link | tr -d '\r\n')
    fi
    }
stream_episode()
    {
    echo "Streaming episode"
    if [ "$wanted_server" = "MP4Upload" ]; then
        mpv \
                      --tls-verify=no \
                      --http-header-fields="Referer: https://www.mp4upload.com/" \
                      --cookies-file="${download_dir}/.cookies.txt" \
                      "$file_link"
    else
        mpv "$file_link"    
    fi
    }
download_episode()
    {
    echo "Downloading episode"
    if [ "$wanted_server" = "MP4Upload" ];then
        curl -k "$file_link" \
                      -H 'User-Agent: Mozilla/5.0' \
                      -H 'Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8' \
                      -H 'Accept-Language: en-US,en;q=0.9'\
                      -H 'Accept-Encoding: gzip, deflate, br, zstd' \
                      -H 'Referer: https://www.mp4upload.com/' \
                      -H 'Sec-GPC: 1' \
                      -H 'Connection: keep-alive' \
                      -c "${download_dir}/.cookies.txt" \
                      -H 'Upgrade-Insecure-Requests: 1' \
                      -H 'Sec-Fetch-Dest: document' \
                      -H 'Sec-Fetch-Mode: navigate' \
                      -H 'Sec-Fetch-Site: same-site' \
                      -H 'Sec-Fetch-User: ?1' \
                      -H 'Priority: u=0, i' --output "${download_dir}/${avaliable_animes[${slug_index}]-Ep${episode_index}.mp4}"
    else
        wget "$file_link" -O "${download_dir}/${avaliable_animes[${slug_index}]-Ep${episode_index}.mp4}"
    fi
    }
menu()
    {
    echo "Introduce tu búsqueda: "
    read search
    search_anime "$search"
    echo "Elige entre los disponibles: "
    for (( i=0; i<${#avaliable_animes[@]}; i++ )); do
        echo "$(( i + 1 )). ${avaliable_animes[${i}]}"
    done
    read anime_index
    slug_index=$(( anime_index - 1 ))
    get_episodes    
    echo "Elige episodio (${episodes_number}): "
    read episode_index
    get_episode_servers
    get_episode_link
    if [ "$mode" = "stream" ]; then
        echo "Mode=$mode"
        stream_episode
    elif [ "$mode" = "download" ]; then
        echo "Mode=$mode"
        download_episode
    else
        echo "Invalid mode error"
    fi
}       
menu    
