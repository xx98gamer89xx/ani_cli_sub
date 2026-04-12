#!/bin/bash

wanted_server=""
download_dir="/home/donar/.anime"
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
    pos_sub=$(awk -v a="$episode_data_sub_dub" 'BEGIN{print index(a,"SUB")}')
    pos_dub=$(awk -v a="$episode_data_sub_dub" 'BEGIN{print index(a,"DUB")}')
    if (( pos_sub > 0 && (pos_sub < pos_dub || pos_dub == 0) )); then
        first="SUB"
        pos=$pos_sub
    else
        first="DUB"
        pos=$pos_dub
    fi
    echo "All episodes avaliable: $episode_data_sub_dub"
    if [ "$dub" -eq 0 ]; then
        if [ "$first" = "DUB" ]; then
            episode_data="${episode_data_sub_dub##*SUB}"
            echo "Avaliable episodes first DUB: $episode_data"
	else
            episode_data="${episode_data_sub_dub%%DUB*}"
	fi
    else
        if [ "$first" = "DUB" ]; then
            episode_data="${episode_data_sub_dub%%SUB*}"
        else
            episode_data="${episode_data_sub_dub##*DUB}"
        fi
    fi
    servers_list=$(echo "${episode_data//\}/\}$'\n'}" | grep -o "server.*")
    while IFS= read -r line; do
        server=${line#server:\"}
        server=${server%%\"*}
        if [ "$server" = "MP4Upload" ] || [ "$server" = "PDrain" ]; then
            url=${line#*url:\"}
            url=${url%\"*}

            servers["$server"]="$url"
        fi
    done <<< "$servers_list"
    }
get_episode_link() 
    {
    if [[ -v servers[PDrain] ]]; then
        wanted_server="PDrain"
        embedded_link="${servers[PDrain]}"
        file_id="${embedded_link##*/}"
        file_link="https://pixeldrain.com/api/file/${file_id}"
    elif [[ -v servers[MP4Upload] ]]; then
        wanted_server="MP4Upload"
        ## Generate cookies
        curl -s -c "${download_dir}/.cookies.txt" \
                      -A "Mozilla/5.0" \
                      https://www.mp4upload.com/j70hobym0b7k \
                      -o /dev/null
        embedded_link="${servers[MP4Upload]}"
        file_id=${embedded_link##*/}
        base_link=$( curl -s -v "$embedded_link" \
                      -b "${download_dir}/.cookies.txt" \
                      -A "Mozilla/5.0" \
                      -H 'Content-Type: application/x-www-form-urlencoded' \
                      --data "op=download2&id=${file_id}&rand=&referer=https%3A%2F%2Fwww.mp4upload.com%2F&method_free=Free+Download" 2>&1 | grep "location" )
        file_link="${base_link#*:}"
        file_link=$(echo $file_link | tr -d '\r\n')
    else 
        echo "No hay servidor válido disponible"
        exit
    fi
    }

read_last_episode()
{
    while IFS= read -r line; do
        anime=${line%% *}
        if [[ "$anime" = "${avaliable_animes[${slug_index}]}" ]]; then
            echo "El último capítulo que viste fue: ${line##* }"
            return 0;
        fi
    done < "${download_dir}/.last_episodes"
    echo "No hay información del último episodio visto"
}

save_last_episode()
    {
    line_index=1
    anime_found=0
    while IFS= read -r line; do
        anime=${line%% *}
        if [[ "$anime" = "${avaliable_animes[${slug_index}]}" ]]; then            
            anime_found=1
            break;
        fi
        ((line_index++))
    done < "${download_dir}/.last_episodes"
    if [[ "$anime_found" -eq 1 ]]; then
        sed "${line_index}s/.*/${avaliable_animes[${slug_index}]} ${episode_index}/"\
             "${download_dir}/.last_episodes" > "${download_dir}/.last_episodes_tmp"
        cp "${download_dir}/.last_episodes_tmp" "${download_dir}/.last_episodes"    
    else
        echo "${avaliable_animes[${slug_index}]} ${episode_index}" >> "${download_dir}/.last_episodes"
    fi
    return 0;
    }

stream_episode()
    {
    if [ "$wanted_server" = "MP4Upload" ]; then
        echo "Streaming desde link: $file_link"
        mpv --really-quiet\
                      --tls-verify=no \
                      --http-header-fields="Referer: https://www.mp4upload.com/" \
                      --cookies-file="${download_dir}/.cookies.txt" \
                      "$file_link"
    else    
        echo "Streaming desde link: $file_link"
        mpv --really-quiet "$file_link"    
    fi
    }
download_episode()
    {
    if [ "$wanted_server" = "MP4Upload" ];then
        curl -s -k "$file_link" \
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
        echo "Descargando episodio de $file_link"
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
    read_last_episode    
    echo "Elige episodio (${episodes_number}): "
    read episode_index
    save_last_episode
    get_episode_servers
    get_episode_link
    if [ "$mode" = "stream" ]; then
        stream_episode
    elif [ "$mode" = "download" ]; then
        download_episode
    else
        echo "Invalid mode error"
    fi
}       
menu    
