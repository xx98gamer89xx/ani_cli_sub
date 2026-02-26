#!/bin/bash

source ./ani_lib.sh
source ./subs_embedder_lib.sh
anime_path="/Your/anime/path"

avaliable_animes=()
avaliable_codes=()
result_number_sub=()
result_number_dub=()

search() {
    search=$1

    results_sub=$(search_anime "${search}" "sub")
    echo "$results_sub" >> "${anime_path}/sub.list"
    results_dub=$(search_anime "${search}" "dub")
    echo "$results_dub" >> "${anime_path}/dub.list"

    line_finished=0
    maching_codes=0
    return_values=()
    line_number_dub=0
    line_number_sub=0
 
    while read -r line_dub; do
        line_number_dub=$(( ${line_number_dub} + 1))
        code_dub="${line_dub:0:17}"
        while read -r line_sub; do
            line_number_sub=$(( ${line_number_sub} + 1))
            code_sub="${line_sub:0:17}"  
            if [ ${code_sub} == ${code_dub} ]; then
                avaliable_animes[maching_codes]="${line_dub}"
	            avaliable_codes[maching_codes]="${code_sub}"
                maching_codes=$(( ${maching_codes} + 1 ))
            fi
        done < "${anime_path}/sub.list"
    done < "${anime_path}/dub.list"
}

select_anime() {
    clear
    echo "Type what you want to search: "
    read search
    search "${search}"
    echo "Select the anime you want to see: "
    for (( i=0 ; i<${#avaliable_animes[*]} ; i++ )); do
        echo "$(( i + 1 )). ${avaliable_animes["${i}"]}"
    done
    read anime_index
    episodes=$(episodes_list "${avaliable_codes[$(( anime_index - 1))]}" "dub")
    echo "Select the episode you want to see:"
    echo "${episodes}"
    read episode_index
    sub_episode_url=$(get_episode_url "${avaliable_codes[anime_index - 1]}" "sub" "$episode_index")
    dub_episode_url=$(get_episode_url "${avaliable_codes[anime_index - 1]}" "dub" "$episode_index")
    download "${sub_episode_url}" "sub" "${anime_path}" 
    echo "Sub version downloaded"
    download "${dub_episode_url}" "dub" "${anime_path}"
    echo "Dub version downloaded"
    echo "Print the season number you want to download: "
    read season_number
    echo "${search} ${season_number} ${episode_index} ${episode_index} ${episode_index} ${anime_path}"
    main "${search}" "${season_number}" "${episode_index}" "${anime_path}"
    echo "Episode downloaded correctly on ${anime_path}"
}
select_anime
rm "${anime_path}/sub.list"
rm "${anime_path}/dub.list"
