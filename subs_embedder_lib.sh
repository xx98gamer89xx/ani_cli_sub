#!/bin/bash
api_key="opensubskey"
api_token="opensubstoken"
sub_language="language"

clean() {
 rm "${dir}"/new.mp4
 rm "${dir}"/*.srt
 rm "${dir}/dub.mp4"
 rm "${dir}/sub.mp4"
}

convert_video() {
    local output_name="${search}_Season=${season_number}_Episode=${episode_number}"
    ffmpeg -i "${dir}/dub.mp4" \
           -i "${dir}/sub.mp4" \
           -i "${dir}/subs.srt" \
           -map 0:v:0 \
           -map 1:a:0 \
           -map 2:0 \
           -c:v copy \
           -c:a copy \
           -c:s mov_text \
           "${dir}/${output_name}.mp4"
}


download_subtitles() {
general_search="$(curl -L -G "https://api.opensubtitles.com/api/v1/subtitles" --data-urlencode "query=${search}" --data-urlencode "season_number=${season_number}" --data-urlencode "episode_number=${episode_number}" --data-urlencode "languages=${sub_language}"\
                    -H "Api-Key: ${api_key}" \
                    -H "Authorization: Bearer ${api_token}"\
                    -H "User-Agent: ani-cli-subs/1.0")"
subs_id=$(echo "$general_search" | jq '.data[0].attributes.files[0].file_id')
    echo "${general_search}"
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

main() {
 search="$1"
 season_number="$2"
 result_number_sub="$3"
 result_number_dub="$4"
 episode_number="$5"
 dir="$6"
 download_subtitles
 convert_video
 clean
}
