#!/bin/bash
api_key="Your opensubs api key"
api_token="Your opensubs api token"
sub_language="sub language"

clean() {
 rm "${dir}/sub.mp4"
 rm "${dir}/dub.mp4"
 rm "${dir}/subs.srt"
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
general_search="$(curl -s -L -G "https://api.opensubtitles.com/api/v1/subtitles" --data-urlencode "query=${search}" --data-urlencode "season_number=${season_number}" --data-urlencode "episode_number=${episode_number}" --data-urlencode "languages=${sub_language}"\
                    -H "Api-Key: ${api_key}" \
                    -H "Authorization: Bearer ${api_token}"\
                    -H "User-Agent: ani-cli-subs/1.0")"
test=$(echo "${general_search}" | jq '.total_pages')
if [ "$test" -eq 0 ]; then
echo "Couldn't find subs for this episode, type the global episode number (like it was all one season)"
read global_episode_number
general_search="$(curl -s -L -G "https://api.opensubtitles.com/api/v1/subtitles" --data-urlencode "query=${search}" --data-urlencode "season_number=${season_number}" --data-urlencode "episode_number=${episode_number_global}" --data-urlencode "languages=${sub_language}"\
                    -H "Api-Key: ${api_key}" \
                    -H "Authorization: Bearer ${api_token}"\
                    -H "User-Agent: ani-cli-subs/1.0")"    
fi
subs_id=$(echo "$general_search" | jq '.data[0].attributes.files[0].file_id')
    echo "${general_search}"
    # Obtener la URL del subtítulo en .vtt desde OpenSubtitles
    subtitle_url="$(curl -s --header "Content-Type: application/json" --request POST --data "{\"file_id\": ${subs_id}}" "https://api.opensubtitles.com/api/v1/download"\
                    -H "Api-Key: ${api_key}" \
                    -H "Authorization: Bearer ${api_token}" \
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
 episode_number="$3"
 dir="$4"
 download_subtitles
 convert_video
 clean
}
