#!/bin/bash

search=""
selected=0
option=""
last_watched=0
wanted_server=""
download_dir="/home/donar/.anime"
declare -A servers
mode="stream"
dub=0
options=()
S='\033[0;32m'
NS='\033[0m'


while getopts "Ddh" arguments; do
  case $argument in
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
    slug=$1
    link="https://animeav1.com/media/${slug}"
    episodes_number=$(curl -s "$link" | grep -o "href=\"/media/${slug}/" | grep -c "href")
    }
get_episode_servers()
    {
    slug=$1
    episode_page_link="https://animeav1.com/media/${slug}/${episode_index}"
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
    if [ "$dub" -eq 0 ]; then
        if [ "$first" = "DUB" ]; then
            episode_data="${episode_data_sub_dub##*SUB}"
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
    if [ "$wanted_server" = "" ]; then
        if [[ -v servers[PDrain] ]]; then
            wanted_server="PDrain"
            embedded_link="${servers[PDrain]}"
            file_id="${embedded_link##*/}"
            file_link="https://pixeldrain.com/api/file/${file_id}"
        else
            wanted_server="MP4Upload"
        fi
    fi
    if [ "$wanted_server" = "MP4Upload" ]; then
        if [[ -v servers[MP4Upload] ]]; then
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
    fi
}

read_last_episode()
{
    slug=$1
    while IFS= read -r line; do
        anime=${line%% *}
        if [[ "$anime" = "${slug}" ]]; then
            last_watched="${line##* }"
            return 0;
        fi
    done < "${download_dir}/.last_episodes"
    last_watched=1
}

save_last_episode()
    {
    slug=$1
    line_index=1
    anime_found=0
    while IFS= read -r line; do
        anime=${line%% *}
        if [[ "$anime" = "${slug}" ]]; then            
            anime_found=1
            break;
        fi
        ((line_index++))
    done < "${download_dir}/.last_episodes"

    if [[ "$anime_found" -eq 1 ]]; then
        sed -i "${line_index}d" "${download_dir}/.last_episodes" # Eliminates the last entry of that anime
    fi

    # Adds the anime that s being watched to the top of the list followed by the last watched episode number
    if [ -s "${download_dir}/.last_episodes" ]; then
        sed -i "1i ${slug} ${episode_index}" "${download_dir}/.last_episodes"
    else
        echo "${slug} ${episode_index}" > "${download_dir}/.last_episodes"
    fi

    return 0;
    }

link_check()
    {
    check=$(curl -s -k "$file_link" | jq -r '.results[].uri' 2> /dev/null ".status")
    if [ "$check" = "false" ]; then
        wanted_server="MP4Upload" 
        get_episode_link
    fi
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
                      -H 'Priority: u=0, i' --output "${download_dir}/${slug}-Ep${episode_index}.mp4}"
    else
        echo "Descargando episodio de $file_link"
        wget "$file_link" -O "${download_dir}/${slug}-Ep${episode_index}.mp4}"
    fi
    }

set_options_to_episodes()
{
        get_episodes "$option"
        read_last_episode "$option"
        search="$last_watched"
        if (( last_watched + 20 > episodes_number )); then
            last_number="$episodes_number"
        else
            last_number=$(( last_watched + 20 ))
        fi
        echo "Ultimo visto: $last_watched Ultimo numero: $last_number"
        options=($(seq "$last_watched" 1 "$last_number"))
}

execute_option()
{
    save_last_episode "$option"
    get_episode_servers "$option"
    get_episode_link
    link_check
    if [ "$mode" = "stream" ]; then
        stream_episode
    elif [ "$mode" = "download" ]; then
        download_episode
    else
        echo "Invalid mode error"
    fi
    search=""
    options=("Continuar viendo" "Volver al inicio" "Salir")
}

enter_pressed()
{
    if [ "$search" = "" ]; then
        if [ "$option" = "" ]; then
            option="${options[${selected}]}"
            set_options_to_episodes
            search=""
            menu_2
        else
            if [ "${options[${selected}]}" = "Continuar viendo" ]; then
                episode_index=$(( $episode_index + 1 ))
                wanted_server=""
                declare -gA servers
                execute_option
                menu_2
            elif [ "${options[${selected}]}" = "Volver al inicio" ]; then
                declare -gA servers
                wanted_server=""
                option=""
                search=""
                options=()
                selected=0
                list_watched_animes
                menu_2                    
            elif [ "${options[${selected}]}" = "Salir" ]; then
                exit 1
            else
                episode_index="${options[${selected}]}"
                execute_option
                menu_2
            fi
        fi
    else
        if [ "$option" = "" ]; then
            search_anime "$search"
            options=("${avaliable_animes[@]}")
            search=""
            menu_2
        else
            episode_index="$search"
            execute_option
            menu_2
        fi
    fi
}

list_watched_animes()
{
    # Add to options the watched animes
    line_number=0
    while IFS= read -r line; do
        anime=${line%% *}
        options[$line_number]="$anime"
        line_number=$(( $line_number + 1 ))
    done < "${download_dir}/.last_episodes"
}

menu_2()
{
    selected=0
        
    while [ 1 -eq 1 ]; do
        #Draw menu and search
        clear
        echo "Búsqueda: $search"
        for ((i = 0; i < "${#options[@]}"; i++))
        do
        if [ $i -eq $selected ]; then
            echo -e "${S}$(( i + 1 )). ${options[${i}]}${NS}"
        else
            echo -e "$(( i + 1 )). ${options[${i}]}"
        fi
        done

        # Read the input and execute it
        escape_char=$(printf "\u1b")
        IFS= read -rsn1 key # get 1 character

        if [[ $key == $escape_char ]]; then
            read -rsn2 key # read 2 more chars
            case $key in
                '[A')
                if (( $selected > 0 )); then
                    selected=$(( $selected - 1 ))
                fi
                ;;
                '[B')
                if (( $selected < $(( ${#options[@]} - 1 )) )); then
                    selected=$(( $selected + 1 ))
                fi
                ;;
                '')
                    enter_pressed
                    break
                    ;;
                " ")
                    search="${search} ";;
            esac
        else
            case $key in
                '')
                enter_pressed
                break
                ;;
                ' ')
                search="${search} ";;
                $'\x7f')
                    if [ ! "$search" = "" ]; then
                        search=${search::-1}
                    fi
                    ;;
                *)
                search="${search}${key}";;
            esac    
        fi
    done
}

cleanup()
{
    tput cnorm
}

tput civis
trap cleanup EXIT

list_watched_animes
menu_2    
