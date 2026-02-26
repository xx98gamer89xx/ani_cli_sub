#!/bin/sh

version_number="4.10.4"


# checks if dependencies are present
dep_ch() {
    for dep; do
        command -v "${dep%% *}" >/dev/null || die "Program \"${dep%% *}\" not found. Please install it."
    done
}

where_iina() {
    [ -e "/Applications/IINA.app/Contents/MacOS/iina-cli" ] && echo "/Applications/IINA.app/Contents/MacOS/iina-cli" && return 0
    printf "%s" "iina" && return 0
}

where_mpv() {
    command -v "flatpak" >/dev/null && flatpak info io.mpv.Mpv >/dev/null 2>&1 && printf "%s" "flatpak_mpv" && return 0
    printf "%s" "mpv" && return 0
}

# SCRAPING

# extract the video links from response of embed urls, extract mp4 links form m3u8 lists
get_links() {
    response="$(curl -e "$allanime_refr" -s "https://${allanime_base}$*" -A "$agent")"
    episode_link="$(printf '%s' "$response" | sed 's|},{|\
|g' | sed -nE 's|.*link":"([^"]*)".*"resolutionStr":"([^"]*)".*|\2 >\1|p;s|.*hls","url":"([^"]*)".*"hardsub_lang":"en-US".*|\1|p')"

    case "$episode_link" in
        *repackager.wixmp.com*)
            extract_link=$(printf "%s" "$episode_link" | cut -d'>' -f2 | sed 's|repackager.wixmp.com/||g;s|\.urlset.*||g')
            for j in $(printf "%s" "$episode_link" | sed -nE 's|.*/,([^/]*),/mp4.*|\1|p' | sed 's|,|\
|g'); do
                printf "%s >%s\n" "$j" "$extract_link" | sed "s|,[^/]*|${j}|g"
            done | sort -nr
            ;;
        *master.m3u8*)
            m3u8_refr=$(printf '%s' "$response" | sed -nE 's|.*Referer":"([^"]*)".*|\1|p') && printf '%s\n' "m3u8_refr >$m3u8_refr" >"$cache_dir/m3u8_refr"
            extract_link=$(printf "%s" "$episode_link" | head -1 | cut -d'>' -f2)
            relative_link=$(printf "%s" "$extract_link" | sed 's|[^/]*$||')
            m3u8_streams="$(curl -e "$m3u8_refr" -s "$extract_link" -A "$agent")"
            printf "%s" "$m3u8_streams" | grep -q "EXTM3U" && printf "%s" "$m3u8_streams" | sed 's|^#EXT-X-STREAM.*x||g; s|,.*|p|g; /^#/d; $!N; s|\n| >|;/EXT-X-I-FRAME/d' |
                sed "s|>|cc>${relative_link}|g" | sort -nr
            printf '%s' "$response" | sed -nE 's|.*"subtitles":\[\{"lang":"en","label":"English","default":"default","src":"([^"]*)".*|subtitle >\1|p' >"$cache_dir/suburl"
            ;;
        *) [ -n "$episode_link" ] && printf "%s\n" "$episode_link" ;;
    esac

    printf "%s" "$*" | grep -q "tools.fast4speed.rsvp" && printf "%s\n" "Yt >$*"
    printf "\033[1;32m%s\033[0m Links Fetched\n" "$provider_name" 1>&2
}

# initialises provider_name and provider_id. First argument is the provider name, 2nd is the regex that matches that provider's link
provider_init() {
    provider_name=$1
    provider_id=$(printf "%s" "$resp" | sed -n "$2" | head -1 | cut -d':' -f2 | sed 's/../&\
/g' | sed 's/^79$/A/g;s/^7a$/B/g;s/^7b$/C/g;s/^7c$/D/g;s/^7d$/E/g;s/^7e$/F/g;s/^7f$/G/g;s/^70$/H/g;s/^71$/I/g;s/^72$/J/g;s/^73$/K/g;s/^74$/L/g;s/^75$/M/g;s/^76$/N/g;s/^77$/O/g;s/^68$/P/g;s/^69$/Q/g;s/^6a$/R/g;s/^6b$/S/g;s/^6c$/T/g;s/^6d$/U/g;s/^6e$/V/g;s/^6f$/W/g;s/^60$/X/g;s/^61$/Y/g;s/^62$/Z/g;s/^59$/a/g;s/^5a$/b/g;s/^5b$/c/g;s/^5c$/d/g;s/^5d$/e/g;s/^5e$/f/g;s/^5f$/g/g;s/^50$/h/g;s/^51$/i/g;s/^52$/j/g;s/^53$/k/g;s/^54$/l/g;s/^55$/m/g;s/^56$/n/g;s/^57$/o/g;s/^48$/p/g;s/^49$/q/g;s/^4a$/r/g;s/^4b$/s/g;s/^4c$/t/g;s/^4d$/u/g;s/^4e$/v/g;s/^4f$/w/g;s/^40$/x/g;s/^41$/y/g;s/^42$/z/g;s/^08$/0/g;s/^09$/1/g;s/^0a$/2/g;s/^0b$/3/g;s/^0c$/4/g;s/^0d$/5/g;s/^0e$/6/g;s/^0f$/7/g;s/^00$/8/g;s/^01$/9/g;s/^15$/-/g;s/^16$/./g;s/^67$/_/g;s/^46$/~/g;s/^02$/:/g;s/^17$/\//g;s/^07$/?/g;s/^1b$/#/g;s/^63$/\[/g;s/^65$/\]/g;s/^78$/@/g;s/^19$/!/g;s/^1c$/$/g;s/^1e$/&/g;s/^10$/\(/g;s/^11$/\)/g;s/^12$/*/g;s/^13$/+/g;s/^14$/,/g;s/^03$/;/g;s/^05$/=/g;s/^1d$/%/g' | tr -d '\n' | sed "s/\/clock/\/clock\.json/")
}

# generates links based on given provider
generate_link() {
    case $1 in
        1) provider_init "wixmp" "/Default :/p" ;;    # wixmp(default)(m3u8)(multi) -> (mp4)(multi)
        2) provider_init "youtube" "/Yt-mp4 :/p" ;;   # youtube(mp4)(single)
        3) provider_init "sharepoint" "/S-mp4 :/p" ;; # sharepoint(mp4)(single)
        *) provider_init "hianime" "/Luf-Mp4 :/p" ;;  # hianime(m3u8)(multi)
    esac
    [ -n "$provider_id" ] && get_links "$provider_id"
}

select_quality() {
    # removing urls which have soft subs to avoid playing on android, iSH and vlc (m3u8 streams don't get correct referrer)
    printf '%s' "$player_function" | cut -f1 -d" " | grep -qE '(android|iSH|vlc)' && links=$(printf '%s' "$links" | sed '/cc>/d;/subtitle >/d;/m3u8_refr >/d')
    printf '%s' "$player_function" | cut -f1 -d" " | grep -qE '(android|iSH)' && links=$(printf '%s' "$links" | sed '/Yt >/d')
    case "$1" in
        best) result=$(printf "%s" "$links" | head -n1) ;;
        worst) result=$(printf "%s" "$links" | grep -E '^[0-9]{3,4}' | tail -n1) ;;
        *) result=$(printf "%s" "$links" | grep -m 1 "$1") ;;
    esac
    [ -z "$result" ] && printf "Specified quality not found, defaulting to best\n" 1>&2 && result=$(printf "%s" "$links" | head -n1)

    # add refr,sub flags for m3u8 and refr flag for yt
    printf '%s' "$result" | grep -q "cc>" && subtitle="$(printf '%s' "$links" | sed -nE 's|subtitle >(.*)|\1|p')" &&
        [ -n "$subtitle" ] && subs_flag="--sub-file=$subtitle"
    printf '%s' "$result" | grep -q "cc>" && m3u8_refr="$(printf '%s' "$links" | sed -nE 's|m3u8_refr >(.*)|\1|p')" && refr_flag="--referrer=$m3u8_refr"
    printf "%s" "$result" | grep -q "tools.fast4speed.rsvp" && refr_flag="--referrer=$allanime_refr"

    ! (printf '%s' "$result" | grep -qE "(cc>|tools.fast4speed.rsvp)") && unset refr_flag
    ! (printf '%s' "$result" | grep -q "cc>") && unset subs_flag
    episode=$(printf "%s" "$result" | cut -d'>' -f2)
}

# gets embed urls, collects direct links into provider files, selects one with desired quality into $episode
get_episode_url() {
    # First argument id Second argument mode Third argument ep_no
    # get the embed urls of the selected episode
    #shellcheck disable=SC2016
    episode_embed_gql='query ($showId: String!, $translationType: VaildTranslationTypeEnumType!, $episodeString: String!) { episode( showId: $showId translationType: $translationType episodeString: $episodeString ) { episodeString sourceUrls }}'

    resp=$(curl -e "$allanime_refr" -s -G "${allanime_api}/api" --data-urlencode "variables={\"showId\":\"$1\",\"translationType\":\"$2\",\"episodeString\":\"$3\"}" --data-urlencode "query=$episode_embed_gql" -A "$agent" | tr '{}' '\n' | sed 's|\\u002F|\/|g;s|\\||g' | sed -nE 's|.*sourceUrl":"--([^"]*)".*sourceName":"([^"]*)".*|\2 :\1|p')
    # generate links into sequential files
    cache_dir="$(mktemp -d)"
    providers="1 2 3 4"
    for provider in $providers; do
        generate_link "$provider" >"$cache_dir"/"$provider" &
    done
    wait
    # select the link with matching quality
    links=$(cat "$cache_dir"/* | sort -g -r -s)
    rm -r "$cache_dir"
    select_quality "$quality"
    if printf "%s" "$ep_list" | grep -q "^$ep_no$"; then
        [ -z "$episode" ] && die "Episode is released, but no valid sources!"
    else
        [ -z "$episode" ] && die "Episode not released!"
    fi
    echo "$episode"
}

# search the query and give results
search_anime() {
    # Mode is the second argument
    #shellcheck disable=SC2016
    search_gql='query( $search: SearchInput $limit: Int $page: Int $translationType: VaildTranslationTypeEnumType $countryOrigin: VaildCountryOriginEnumType ) { shows( search: $search limit: $limit page: $page translationType: $translationType countryOrigin: $countryOrigin ) { edges { _id name availableEpisodes __typename } }}'

    curl -e "$allanime_refr" -s -G "${allanime_api}/api" --data-urlencode "variables={\"search\":{\"allowAdult\":false,\"allowUnknown\":false,\"query\":\"$1\"},\"limit\":40,\"page\":1,\"translationType\":\"$2\",\"countryOrigin\":\"ALL\"}" --data-urlencode "query=$search_gql" -A "$agent" | sed 's|Show|\
| g' | sed -nE "s|.*_id\":\"([^\"]*)\",\"name\":\"(.+)\",.*${mode}\":([1-9][^,]*).*|\1	\2 (\3 episodes)|p" | sed 's/\\"//g'
}

time_until_next_ep() {
    animeschedule="https://animeschedule.net"
    query="$(printf "%s\n" "$*" | tr ' ' '+')"
    curl -s -G "$animeschedule/api/v3/anime" --data "q=${query}" | sed 's|"id"|\n|g' | sed -nE 's|.*,"route":"([^"]*)","premier.*|\1|p' | while read -r anime; do
        data=$(curl -s "$animeschedule/anime/$anime" | sed '1,/"anime-header-list-buttons-wrapper"/d' | sed -nE 's|.*countdown-time-raw" datetime="([^"]*)">.*|Next Raw Release: \1|p;s|.*countdown-time" datetime="([^"]*)">.*|Next Sub Release: \1|p;s|.*english-title">([^<]*)<.*|English Title: \1|p;s|.*main-title".*>([^<]*)<.*|Japanese Title: \1|p')
        status="Ongoing"
        color="33"
        printf "%s\n" "$data"
        ! (printf "%s\n" "$data" | grep -q "Next Raw Release:") && status="Finished" && color="32"
        printf "Status:  \033[1;%sm%s\033[0m\n---\n" "$color" "$status"
    done
    exit 0
}

# get the episodes list of the selected anime
episodes_list() {
    #First argument show id Second argument mode
    #ShowId is the first argument
    #shellcheck disable=SC2016
    episodes_list_gql='query ($showId: String!) { show( _id: $showId ) { _id availableEpisodesDetail }}'

    curl -e "$allanime_refr" -s -G "${allanime_api}/api" --data-urlencode "variables={\"showId\":\"$1\"}" --data-urlencode "query=$episodes_list_gql" -A "$agent" | sed -nE "s|.*$2\":\[([0-9.\",]*)\].*|\1|p" | sed 's|,|\
|g; s|"||g' | sort -n -k 1
}

# PLAYING

process_hist_entry() {
    ep_list=$(episodes_list "$id")
    latest_ep=$(printf "%s\n" "$ep_list" | tail -n1)
    title=$(printf "%s\n" "$title" | sed "s|[0-9]\+ episodes|${latest_ep} episodes|")
    ep_no=$(printf "%s" "$ep_list" | sed -n "/^${ep_no}$/{n;p;}") 2>/dev/null
    [ -n "$ep_no" ] && printf "%s\t%s - episode %s\n" "$id" "$title" "$ep_no"
}

update_history() {
    if grep -q -- "$id" "$histfile"; then
        sed -E "s|^[^	]+	${id}	[^	]+$|${ep_no}	${id}	${title}|" "$histfile" >"${histfile}.new"
    else
        cp "$histfile" "${histfile}.new"
        printf "%s\t%s\t%s\n" "$ep_no" "$id" "$title" >>"${histfile}.new"
    fi
    mv "${histfile}.new" "$histfile"
}

download() {
    # First argument episode_url Second argument mode Third argument download_dir
    # download subtitle if it's set
    download_dir=$3
    case $1 in
        *m3u8*)
            if command -v "yt-dlp" >/dev/null; then
                if [ $2 == "dub" ]; then
                  yt-dlp --referer "$m3u8_refr" "$1" --no-skip-unavailable-fragments --fragment-retries infinite -N 16 -o "$download_dir/dub.mp4"
		else
		  yt-dlp --referer "$m3u8_refr" "$1" --no-skip-unavailable-fragments --fragment-retries infinite -N 16 -o "$download_dir/sub.mp4"
		fi
            else
                if [ $2 == "dub" ]; then
                 ffmpeg -extension_picky 0 -referer "$m3u8_refr" -loglevel error -stats -i "$1" -c copy "$download_dir/dub.mp4"
                else
                 ffmpeg -extension_picky 0 -referer "$m3u8_refr" -loglevel error -stats -i "$1" -c copy "$download_dir/sub.mp4" 
                fi
           fi
            # embed subs into downloads
            # [ -e "$download_dir/$2.vtt" ] && ffmpeg -i "$download_dir/$2.mp4" -i "$download_dir/$2.vtt" -c copy -c:s mov_text "$download_dir/$2.bak.mp4" && mv "$download_dir/$2.bak.mp4" "$download_dir/$2.mp4"
            ;;
        *)
            # shellcheck disable=SC208
            if [ $2 == "dub" ]; then
                 aria2c --referer="$allanime_refr" --enable-rpc=false --check-certificate=false --continue $iSH_DownFix --summary-interval=0 -x 16 -s 16 "$1" --dir="$download_dir" -o "dub.mp4" --download-result=hide       
                else
                 aria2c --referer="$allanime_refr" --enable-rpc=false --check-certificate=false --continue $iSH_DownFix --summary-interval=0 -x 16 -s 16 "$1" --dir="$download_dir" -o "sub.mp4" --download-result=hide
            fi            
            ;;
    esac
}

play_episode() {
    subs_flag="--sub-file=home/donar/clones/ani-cli2/attack.vtt"
    [ "$log_episode" = 1 ] && [ "$player_function" != "debug" ] && [ "$player_function" != "download" ] && command -v logger >/dev/null && logger -t ani-cli "${allanime_title}${ep_no}"
    [ "$skip_intro" = 1 ] && skip_flag="$(ani-skip -q "$mal_id" -e "$ep_no")"
    [ -z "$episode" ] && get_episode_url
    # shellcheck disable=SC2086
    case "$player_function" in
        debug)
            printf "All links:\n%s\nSelected link:\n" "$links"
            printf "%s\n" "$episode"
            ;;
        mpv*)
            if [ "$no_detach" = 0 ]; then
                nohup $player_function $skip_flag --force-media-title="${allanime_title}Episode ${ep_no}" "$episode" $subs_flag $refr_flag >/dev/null 2>&1 &
            else
                $player_function $skip_flag $subs_flag $refr_flag --force-media-title="${allanime_title}Episode ${ep_no}" "$episode"
                mpv_exitcode=$?
                [ "$exit_after_play" = 1 ] && [ -z "$range" ] && exit "$mpv_exitcode"
            fi
            ;;
        android_mpv) nohup am start --user 0 -a android.intent.action.VIEW -d "$episode" -n is.xyz.mpv/.MPVActivity >/dev/null 2>&1 & ;;
        android_vlc) nohup am start --user 0 -a android.intent.action.VIEW -d "$episode" -n org.videolan.vlc/org.videolan.vlc.gui.video.VideoPlayerActivity -e "title" "${allanime_title}Episode ${ep_no}" >/dev/null 2>&1 & ;;
        *iina*)
            [ -n "$subs_flag" ] && subs_flag="--mpv-${subs_flag#--}"
            [ -n "$refr_flag" ] && refr_flag="--mpv-${refr_flag#--}"
            if pgrep -f "IINA" >/dev/null 2>&1; then
                # omit --keep-running when an IINA instance exists to prevent hanging
                nohup $player_function --no-stdin --mpv-force-media-title="${allanime_title}Episode ${ep_no}" $subs_flag $refr_flag "$episode" >/dev/null 2>&1 &
            else
                nohup $player_function --no-stdin --keep-running --mpv-force-media-title="${allanime_title}Episode ${ep_no}" $subs_flag $refr_flag "$episode" >/dev/null 2>&1 &
            fi
            ;;
        flatpak_mpv) flatpak run io.mpv.Mpv --force-media-title="${allanime_title}Episode ${ep_no}" "$episode" $subs_flag $refr_flag >/dev/null 2>&1 & ;;
        vlc*) nohup $player_function --http-referrer="${allanime_refr}" --play-and-exit --meta-title="${allanime_title}Episode ${ep_no}" "$episode" >/dev/null 2>&1 & ;;
        *yncpla*) nohup $player_function "$episode" -- --force-media-title="${allanime_title}Episode ${ep_no}" $subs_flag $refr_flag >/dev/null 2>&1 & ;;
        download) "$player_function" "$episode" "${allanime_title}Episode ${ep_no}" "$subtitle" ;;
        catt) nohup catt cast "$episode" -s "$subtitle" >/dev/null 2>&1 & ;;
        iSH)
            printf "\e]8;;vlc://%s\a~~~~~~~~~~~~~~~~~~~~\n~ Tap to open VLC ~\n~~~~~~~~~~~~~~~~~~~~\e]8;;\a\n" "$episode"
            sleep 5
            ;;
        *) nohup $player_function "$episode" >/dev/null 2>&1 & ;;
    esac
    replay="$episode"
    unset episode
    update_history
    [ "$use_external_menu" = "1" ] && wait
}

play() {
    start=$(printf "%s" "$ep_no" | grep -Eo '^(-1|[0-9]+(\.[0-9]+)?)')
    end=$(printf "%s" "$ep_no" | grep -Eo '(-1|[0-9]+(\.[0-9]+)?)$')
    [ "$start" = "-1" ] && ep_no=$(printf "%s" "$ep_list" | tail -n1) && unset start
    [ -z "$end" ] || [ "$end" = "$start" ] && unset start end
    [ "$end" = "-1" ] && end=$(printf "%s" "$ep_list" | tail -n1)
    line_count=$(printf "%s\n" "$ep_no" | wc -l | tr -d "[:space:]")
    if [ "$line_count" != 1 ] || [ -n "$start" ]; then
        [ -z "$start" ] && start=$(printf "%s\n" "$ep_no" | head -n1)
        [ -z "$end" ] && end=$(printf "%s\n" "$ep_no" | tail -n1)
        range=$(printf "%s\n" "$ep_list" | sed -nE "/^${start}\$/,/^${end}\$/p")
        [ -z "$range" ] && die "Invalid range!"
        for i in $range; do
            tput clear
            ep_no=$i
            printf "\33[2K\r\033[1;34mPlaying episode %s...\033[0m\n" "$ep_no"
            [ "$i" = "$end" ] && unset range
            play_episode
        done
    else
        play_episode
    fi
    # moves up to stored position and deletes to end
    [ "$player_function" != "debug" ] && [ "$player_function" != "download" ] && tput rc && tput ed
}

# MAIN

# setup
agent="Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:109.0) Gecko/20100101 Firefox/121.0"
allanime_refr="https://allmanga.to"
allanime_base="allanime.day"
allanime_api="https://api.${allanime_base}"
mode="${ANI_CLI_MODE:-sub}"
download_dir="${ANI_CLI_DOWNLOAD_DIR:-/home/donar/.anime}"
log_episode="${ANI_CLI_LOG:-1}"
quality="${ANI_CLI_QUALITY:-best}"
case "$(uname -a | cut -d " " -f 1,3-)" in
    *Darwin*) player_function="${ANI_CLI_PLAYER:-$(where_iina)}" ;;   # mac OS
    *ndroid*) player_function="${ANI_CLI_PLAYER:-android_mpv}" ;;     # Android OS (termux)
    *MINGW* | *WSL2*) player_function="${ANI_CLI_PLAYER:-mpv.exe}" ;; # Windows OS
    *ish*) player_function="${ANI_CLI_PLAYER:-iSH}" ;;                # iOS (iSH)
    *) player_function="${ANI_CLI_PLAYER:-$(where_mpv)}" ;;           # Linux OS
esac

no_detach="${ANI_CLI_NO_DETACH:-0}"
exit_after_play="${ANI_CLI_EXIT_AFTER_PLAY:-0}"
use_external_menu="${ANI_CLI_EXTERNAL_MENU:-0}"
external_menu_normal_window="${ANI_CLI_EXTERNAL_MENU_NORMAL_WINDOW:-0}"
skip_intro="${ANI_CLI_SKIP_INTRO:-0}"
# shellcheck disable=SC2154
skip_title="$ANI_CLI_SKIP_TITLE"
[ -t 0 ] || use_external_menu=1
hist_dir="${ANI_CLI_HIST_DIR:-${XDG_STATE_HOME:-$HOME/.local/state}/ani-cli}"
[ ! -d "$hist_dir" ] && mkdir -p "$hist_dir"
histfile="$hist_dir/ani-hsts"
[ ! -f "$histfile" ] && : >"$histfile"
search="${ANI_CLI_DEFAULT_SOURCE:-scrape}"


