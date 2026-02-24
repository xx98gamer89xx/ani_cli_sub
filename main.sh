#!/bin/bash
api_key="l36mOH0G9EPEAEQocFuQYX2GD5ZbS1Au"
api_token="eyJ0eXAiOiJKV1QiLCJhbGciOiJIUzI1NiJ9.eyJpc3MiOiJuTTFidjdPczFzenhKWlFiVjdoRnpXVEdEOEFwM1dmMiIsImV4cCI6MTc3MjAzMDg1NH0.fLHftQQz7TVYEDaMaL5UrrTVELpvk-KvERvVQsFTlBI"
search="$1"
season_number="$2"
result_number_sub="$3"
result_number_dub="$4"
episode_number="$5"
dir="/home/donar/.anime"
sub_language="es"
ani_cli_dir="/home/donar/scripts"

echo "Arguments: 1 = search, 2 = season_number, 3 = result_number_sub (check ani-cli), 4 = result_number_dub, 5 = episode_number"

main() {
 download_subtitles
 download_video
 clean
}

clean() {
 rm "${dir}"/sub.mp4
 rm "${dir}"/dub.mp4
 rm "${dir}"/new.mp4
 rm "${dir}"/*.srt
}

download_video() {
    local output_name="${search}_Season=${season_number}_Episode=${episode_number}"
    $ani_cli_dir/ani_cli_sub --dub -d "$search" -S $result_number_dub -e $episode_number
    $ani_cli_dir/ani_cli_sub -d "$search" -S $result_number_sub -e $episode_number
    ffmpeg -i "${dir}/dub.mp4" -i "${dir}/sub.mp4" -c copy -map 0:v:0 -map 1:a:0 "${dir}/new.mp4"
    echo "${dir}/new.mp4"
    ffmpeg -i "${dir}/new.mp4" -i "${dir}/subs.srt" -c copy -c:s mov_text "${dir}/${output_name}.mp4"
}

download_subtitles() {
general_search="$(curl -L -G "https://api.opensubtitles.com/api/v1/subtitles" --data-urlencode "query=${search}" --data-urlencode "season_number=${season_number}" --data-urlencode "episode_number=${episode_number}" --data-urlencode "languages=${sub_language}"\
                    -H "Api-Key: ${api_key}" \
                    -H "Authorization: ${api_token}"\
                    -H "User-Agent: ani-cli-subs/1.0")"
subs_id=$(echo "$general_search" | jq '.data[0].attributes.files[0].file_id')
    
    # Obtener la URL del subtítulo en .vtt desde OpenSubtitles
    subtitle_url="$(curl --header "Content-Type: application/json" --request POST --data "{\"file_id\": ${subs_id}}" "https://api.opensubtitles.com/api/v1/download"\
                    -H "Api-Key: l36mOH0G9EPEAEQocFuQYX2GD5ZbS1Au" \
                    -H "Authorization: Bearer eyJ0eXAiOiJKV1QiLCJhbGciOiJIUzI1NiJ9.eyJpc3MiOiJuTTFidjdPczFzenhKWlFiVjdoRnpXVEdEOEFwM1dmMiIsImV4cCI6MTc3MjAzMDg1NH0.fLHftQQz7TVYEDaMaL5UrrTVELpvk-KvERvVQsFTlBI" \
                    -H "User-Agent: ani-cli-subs/1.0")"

    # Verificar que se obtuvo URL
    if [ -z "$subtitle_url" ] || [ "$subtitle_url" == "null" ]; then
        echo "No se pudo obtener la URL del subtítulo para subs_id $subs_id"
        return 1
    fi

    # Descargar el subtítulo
    
    actual_sub_url=$(echo "$subtitle_url" | jq -r .link)
    
    wget -O "${dir}/subs.srt" "${actual_sub_url}"
  
    }

main
