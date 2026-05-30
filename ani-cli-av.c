#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ncurses.h>
#include <curl/curl.h>
#include "cJSON.h"

struct anime_data
{
    char slug[1000];
    char last_episode[10];
};

struct server_data
{
    char name[100];
    char embedded_link[2000];
    char download_link[2000];
};

char response[10000000] = "";
struct anime_data *avaliable_animes = NULL;
struct server_data *avaliable_servers = NULL;
int avaliable_animes_count = 0;
int avaliable_servers_index = 0;

size_t callback(void *ptr, size_t size, size_t nmemb, void *userdata)
{
    strncat(response, ptr, size * nmemb);
    return size * nmemb;
}

static size_t discard_body(void *ptr,
                           size_t size,
                           size_t nmemb,
                           void *userdata)
{
    return size * nmemb;
}

static int response_length = 0;

size_t header_callback(void *buffer, size_t size, size_t nitems, void *userdata)
{
    size_t total = size * nitems;

    if (response_length + total >= sizeof(response) - 1)
    {
        printf("HEADER BUFFER OVERFLOW\n");
        return 0;
    }

    memcpy(response + response_length, buffer, total);

    response_length += total;
    response[response_length] = '\0';

    return total;
}

void parse_search(cJSON *array)
{
  cJSON *slug;
  cJSON *id;
  cJSON *item = array ? array->child : 0;
  int i = 0;
  while (item)
  {
        bool saved = false;
        slug = cJSON_GetObjectItem(item, "slug");
        item=item->next;
        for (int i = 0; i < avaliable_animes_count; i++)
        {
            if (strcmp(avaliable_animes[i].slug, slug->valuestring) == 0)
            {
                saved = true;
                break;
            }
            else
            {
                saved = false;
            }
        }
        if (saved != true)
        {
            avaliable_animes_count += 1;
            avaliable_animes = realloc(avaliable_animes, avaliable_animes_count * sizeof(struct anime_data));
            strcpy(avaliable_animes[i].slug, slug->valuestring);
            strcpy(avaliable_animes[i].last_episode, "1");
        }
        i++;
  }
}

char* search_animes(char search[])
{
    response_length = 0;
    response[0] = '\0';
    strcpy(response, "");
    CURLcode result;
    CURL *curl;
    struct curl_slist *slist1;

    char search_string[1000] = "{\"query\":\"";
    strcat(search_string, search);
    strcat(search_string, "\"}");

    slist1 = NULL;
    slist1 = curl_slist_append(slist1, "User-Agent: Mozilla/5.0");
    slist1 = curl_slist_append(slist1, "Content-Type: application/json");

    curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_BUFFERSIZE, 102400L);
    curl_easy_setopt(curl, CURLOPT_URL, "https://animeav1.com/api/search");
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, search_string);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE_LARGE, strlen(search_string));
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, slist1);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "curl/8.20.0");
    curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 50L);
    curl_easy_setopt(curl, CURLOPT_SSLVERSION, (long)CURL_SSLVERSION_TLSv1_2);
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
    curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, callback);
    result = curl_easy_perform(curl);



    curl_easy_cleanup(curl);
    curl = NULL;
    curl_slist_free_all(slist1);
    slist1 = NULL;

    printf("%s", response);
    cJSON *json_response = cJSON_Parse(response);
    parse_search(json_response);
    cJSON_Delete(json_response);
}

char* get_episodes(char slug[], int *max_episodes)
{
  response_length = 0;
  response[0] = '\0';
  strcpy(response, "");
  
  CURLcode result;
  CURL *curl;

  char anime_url[100] = "https://animeav1.com/media/";
  strcat(anime_url, slug);

  curl = curl_easy_init();
  curl_easy_setopt(curl, CURLOPT_BUFFERSIZE, 102400L);
  curl_easy_setopt(curl, CURLOPT_URL, anime_url);
  curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
  curl_easy_setopt(curl, CURLOPT_USERAGENT, "curl/8.20.0");
  curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 50L);
  curl_easy_setopt(curl, CURLOPT_SSLVERSION, (long)CURL_SSLVERSION_TLSv1_2);
  curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, callback);
  result = curl_easy_perform(curl);

  curl_easy_cleanup(curl);
  curl = NULL;

  bool iterating = true;
  char *coincidence = &response[0];
  char substring[100] = "href=\"/media/";
  strcat(substring, slug);

  while (iterating == true)
  {
    char *search = strstr(coincidence, substring);
    if ( search == NULL)
    {
        iterating = false;
    }
    else
    {
        coincidence = search + strlen(substring);
    }
  }
  char *episode_string_back = coincidence + 1;
  char *episode_string_front = strstr(episode_string_back, "\"");
  int episodes_number_digits = episode_string_front - episode_string_back;
  static char episodes_number[100];
  for (int i = 0; i < episodes_number_digits; i++)
  {
    episodes_number[i] = *(episode_string_back + i);
  }
  printf("%s", episodes_number);
  *max_episodes = atoi(episodes_number);
  return 0;
}

int parse_episode_servers_and_links(int orientation, char* episode_dub_position, char* episode_sub_position, char* server)
{
    free(avaliable_servers);
    avaliable_servers = NULL;
    avaliable_servers_index = 0;

    printf("\n PARSEADOR \n");
    char* last_sub_dub_position;
    int while_condition;
    if (orientation > 0)
    {
        if (episode_dub_position == NULL)
        {
            episode_dub_position = strstr(server, "server:\"") + strlen(strstr(server, "server:\""));
        }
        last_sub_dub_position = episode_dub_position;
        while_condition = 1;
        printw("\n 1 .LAST SUB_DUB POSI1111TION: %s \n", last_sub_dub_position);
    }
    else
    {
        while_condition = 0;
        last_sub_dub_position = episode_sub_position;
        printw("\n2.  LAST SUB_DUB POSI2222TION: %s \n", last_sub_dub_position);
    }

    bool iterating = true;
    while (iterating == true)
    {
        char *search;
        int condition;
        if (while_condition == 0)
        {
            printf("\n EL ULTIMO ES EL SUB\n ");
            search = strstr(episode_sub_position, "server:\"");
            condition = (search == NULL);
        }
        else
        {
            printf("\n EL ULTIMO ES EL DUB\n");
            search = strstr(server, "server:\"");
            condition = (last_sub_dub_position - search < 0);
        }
        if (condition)
        {
            iterating = false;
        }
        else
        {
            char server_name[100];
            int server_characters_number = strstr(search + strlen("server:\""), "\"") - (search + strlen("server:\""));
            for (int i = 0; i < server_characters_number; i++)
            {
                server_name[i] = *(search + strlen("server:\"") + i);
            }
            server_name[server_characters_number] = '\0';
            if (strcmp(server_name, "PDrain") == 0 || strcmp(server_name, "MP4Upload") == 0)
            {
                avaliable_servers_index += 1;
                struct server_data *temp = realloc(avaliable_servers, avaliable_servers_index * sizeof(struct server_data));
                if (temp != NULL)
                {
                    avaliable_servers = temp;
                }
                printf("\n NOMBRE DEL SERVIDOR: %s \n", server_name);
                strcpy(avaliable_servers[avaliable_servers_index - 1].name, server_name);
                char *url_start = strstr(search, "url:\"");
                char *url_end = strstr(url_start + strlen("url:\""), "\"");
                int url_lenght = url_end - (url_start + strlen("url:\""));
                char episode_link[1000];
                for (int i = 0; i < url_lenght; i++)
                {
                    episode_link[i] = *(url_start + strlen("url:\"") + i);
                }
                episode_link[url_lenght] = '\0';
                printf("\n LINK DE EPISODIO: %s\n", episode_link);
                strcpy(avaliable_servers[avaliable_servers_index - 1].embedded_link, episode_link);
            }
            if (while_condition == 0)
            {
                episode_sub_position = episode_sub_position + strlen("server:\"");
            }
            else
            {
                server = search + strlen("server:\"");
            }
        }
    } 
    return 0;   
}


int get_episode_link(char slug[], char episode_number[])
{
    response_length = 0;
    response[0] = '\0';
    strcpy(response, "");

    char episode_page_link[1000] = "https://animeav1.com/media/";
    strcat(episode_page_link, slug);
    strcat(episode_page_link, "/");
    strcat(episode_page_link, episode_number);
  
  CURLcode result;
  CURL *curl;

  curl = curl_easy_init();
  curl_easy_setopt(curl, CURLOPT_BUFFERSIZE, 102400L);
  curl_easy_setopt(curl, CURLOPT_URL, episode_page_link);
  curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
  curl_easy_setopt(curl, CURLOPT_USERAGENT, "curl/8.20.0");
  curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 50L);
  curl_easy_setopt(curl, CURLOPT_SSLVERSION, (long)CURL_SSLVERSION_TLSv1_2);
  curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, callback);

  result = curl_easy_perform(curl);

  curl_easy_cleanup(curl);
  curl = NULL;

  bool iterating = true;
  char *coincidence = &response[0];
  char substring[10] = "downloads";

  while (iterating == true)
  {
    char *search = strstr(coincidence, substring);
    if ( search == NULL)
    {
        iterating = false;
    }
    else
    {
        coincidence = search + strlen(substring);
    }
  } 

    char* episode_sub_position = strstr(coincidence, "SUB");
    char* episode_dub_position = strstr(coincidence, "DUB");

    int orientation;
    if (episode_dub_position != NULL)
    {
        orientation = episode_dub_position - episode_sub_position;
    }
    else
    {
        orientation = -1;
    }

    printf("\n SUB: %s \n", episode_sub_position);
    printf("\n DUB: %s \n", episode_dub_position);

    parse_episode_servers_and_links(orientation, episode_dub_position, episode_sub_position, coincidence);
}



int get_mp4upload_download_link(char cookies_jar[], char embedded_link[], char file_id[], int avaliable_servers_index)
{
  printf("MP4 UPLOAD REQUEST");
  char post_data[1000] = "op=download2&id=";
  strcat(post_data, file_id);
  strcat(post_data, "&rand=&referer=https%3A%2F%2Fwww.mp4upload.com%2F&method_free=Free+Download");
  response_length = 0;
  response[0] = '\0';
  strcpy(response, "");

  CURLcode result;
  CURL *curl;
  struct curl_slist *slist1;

  slist1 = NULL;
  slist1 = curl_slist_append(slist1, "Content-Type: application/x-www-form-urlencoded");

  curl = curl_easy_init();
  curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
  curl_easy_setopt(curl, CURLOPT_BUFFERSIZE, 102400L);
  curl_easy_setopt(curl, CURLOPT_URL, embedded_link);
  curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data);
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, slist1);
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 0L);
  curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0");
  curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 50L);
  curl_easy_setopt(curl, CURLOPT_COOKIEFILE, cookies_jar);
  curl_easy_setopt(curl, CURLOPT_SSLVERSION, (long)CURL_SSLVERSION_TLSv1_2);
  curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, discard_body);
  curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);

  result = curl_easy_perform(curl);

  // Search for link on http headers

  printf("MP4 UPLOAD REQUEST");

  bool iterating = true;
  char *coincidence = response;
  char substring[11] = "location: ";
  static char download_link[2000];
  while (iterating == true)
  {
    char *search = strstr(coincidence, substring);
    if ( search == NULL)
    {
        char* download_link_start = coincidence;
        printf("EMPIEZO: %s", download_link_start);
        if (strstr(coincidence, "\r\n") == NULL)
        {
            printf("NOOOOOO");
            return 1;
        }
        else
        {
        char* download_link_end = strstr(coincidence, "\r\n");
        if (download_link_end != NULL)
        {
            int url_length = download_link_end - download_link_start;
            printf("\nLONGITUD URL=%i \n", url_length);
            for (int i = 0; i < url_length; i++)
            {
                download_link[i] = *(download_link_start + i);
            }
            download_link[url_length] = '\0';
            printf("LINK DE DESCARGA: %s", download_link);
            iterating = false;
        }
        else
        {
            printf("NOOO LA POLI");
        }
        }
    }
    else
    {
        coincidence = search + strlen(substring);
    }
  } 

  curl_easy_cleanup(curl);
  curl = NULL;
  curl_slist_free_all(slist1);
  slist1 = NULL;
  printf("NUMERO DE SERVIDORES %i", avaliable_servers_index);
  //IMPORTANTE DESCOMENTAR
  strcpy(avaliable_servers[avaliable_servers_index].download_link, download_link); 

    printf("\n DOWNLOAD LINK: %s \n", download_link);

  return 0;
}

int get_download_links()
{
    printf("INICIADO GET DOWNLOAD LINKS \n");
    for (int i = 0; i < avaliable_servers_index; i++)
    {
        if (strcmp(avaliable_servers[i].name, "PDrain") == 0)
        {
            printf("PDRAIN \n");
            bool iterating = true;
            char *coincidence = &avaliable_servers[i].embedded_link[0];
            printf("COINCIDENCE: %s \n", coincidence);
            char substring[2] = "/";

            while (iterating == true)
                {
                char *search = strstr(coincidence, substring);
                if ( search == NULL)
                {
                    iterating = false;
                    char final_link[10000] = "https://pixeldrain.com/api/file/";
                    char* file_id = coincidence;
                    strcat(final_link, file_id);
                    printw("LINK ANTES DE GUARDAR%s", final_link);
                    strcpy(avaliable_servers[i].download_link, final_link);
                }
                else
                {
                    coincidence = search + strlen(substring);
                }

            } 
        }
        else if (strcmp(avaliable_servers[i].name, "MP4Upload") == 0)
        {
            printf("MP4UPLOAD \n");
            // Request to generate cookies
            CURLcode result;
            CURL *curl;

            curl = curl_easy_init();
            curl_easy_setopt(curl, CURLOPT_BUFFERSIZE, 102400L);
            curl_easy_setopt(curl, CURLOPT_URL, "https://www.mp4upload.com/j70hobym0b7k");
            curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
            curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0");
            curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 50L);
            curl_easy_setopt(curl, CURLOPT_COOKIEJAR, "/home/donar/.anime/.cookies.txt");
            curl_easy_setopt(curl, CURLOPT_SSLVERSION, (long)CURL_SSLVERSION_TLSv1_2);
            curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, discard_body);
        
            result = curl_easy_perform(curl);

            curl_easy_cleanup(curl);
            curl = NULL;

            printf("\n COOKIES GENERATED \n");

            bool iterating = true;
            char *coincidence = avaliable_servers[i].embedded_link;

            while (iterating == true)
            {
                char *search = strstr(coincidence, "/");
                if ( search == NULL)
                {   
                    iterating = false;
                    char* file_id = coincidence;
                    get_mp4upload_download_link("/home/donar/.anime/.cookies.txt", avaliable_servers[i].embedded_link, file_id, i);
                }
                else
                {
                    coincidence = search + strlen("/");
                }
            }
        }
        
    }
}

int save_last_episode(char *last_episodes_file, char slug[], char episode_number[])
{
    FILE *fptr;
    fptr = fopen(last_episodes_file, "r");
    fseek(fptr, 0, SEEK_END);

    long last_episodes_file_size = ftell(fptr);
    rewind(fptr);

    char *last_episode_string = malloc((last_episodes_file_size + 1) * sizeof(char));

    fread(last_episode_string, sizeof(char), last_episodes_file_size, fptr);
    last_episode_string[last_episodes_file_size] = '\0';

    printf("STRIGN DE ULTOMOS EPISODIOS: %s", last_episode_string);

    char *coincidence = &last_episode_string[0];
    char *slug_start_position, *slug_end_position;

    // IF SLUG WAS ALREADY SAVED DELETE IT
    if (strstr(coincidence, slug) != NULL)
    {
        char *search = strstr(coincidence, slug);
        slug_start_position = search;
        
        bool iterating = true;
        coincidence = slug_start_position;

        while (iterating == true)
        {
            char *search = strstr(coincidence, "\n");
            if ( search > slug_start_position)
            {
                slug_end_position = search;
                iterating = false;
            }
            else
            {
                coincidence = search + 1;
            }
        }

        int slug_length = slug_end_position - slug_start_position;
        int slug_index = slug_start_position - &last_episode_string[0];
        printf("SLUG LENGHT = %i", slug_length);

        for (int i = slug_index ; i + slug_length < strlen(last_episode_string); i++)
        {
            last_episode_string[i] = last_episode_string[i + slug_length + 1];
        }
        
        printf("SLUG ELIMINATED STRING: %s", last_episode_string);
        fclose(fptr);
    }
    // ADD SLUG TO BEGGINING OF FILE
    char last_episode_file[last_episodes_file_size * 2];
    strcpy(last_episode_file, slug);

    strcat(last_episode_file, " ");
    strcat(last_episode_file, episode_number); 
    strcat(last_episode_file, "\n");
    strcat(last_episode_file, last_episode_string);
    printf("ARCHIVO DE ULTOMOS EPISODIOS: %s", last_episode_file);
    fptr = fopen(last_episodes_file, "w");

    fprintf(fptr, "%s", last_episode_file);

    fclose(fptr);

    free(last_episode_string);

    return 0;
}

char* read_last_episode(char last_episodes_file[])
{
    FILE *fptr;
    fptr = fopen(last_episodes_file, "r");
    fseek(fptr, 0, SEEK_END);
    long last_episodes_file_size = ftell(fptr);
    rewind(fptr);

    char *last_episodes_string = malloc((last_episodes_file_size + 1) * sizeof(char));
    fread(last_episodes_string, sizeof(char), last_episodes_file_size, fptr);
    last_episodes_string[last_episodes_file_size] = '\0';

    fclose(fptr);

    char* slug_start_position = last_episodes_string;

    char slug[1000];
    char episode_number[1000];
    bool iterating = true;

    while (iterating == true)
    {
        char* episode_end_position = strstr(slug_start_position, "\n");
        if (episode_end_position != NULL)
        {
            char* episode_number_start_position = strstr(slug_start_position, " ");
            for (int i = 0; i < episode_number_start_position - slug_start_position; i++ )
            {
                slug[i] = *(slug_start_position + i);
            }

            slug[episode_number_start_position - slug_start_position] = '\0';
            avaliable_animes_count += 1;
            avaliable_animes = realloc(avaliable_animes, avaliable_animes_count * sizeof(struct anime_data));
            strcpy(avaliable_animes[avaliable_animes_count - 1].slug, slug);
            printw("ANIME: %s", slug);
            for (int i = 0; i < episode_end_position - episode_number_start_position; i++ )
            {
                printw("EP: %c", i);
                episode_number[i] = *(episode_number_start_position + 1 + i);
            }

            episode_number[episode_end_position - episode_number_start_position] = '\0';
            strcpy(avaliable_animes[avaliable_animes_count - 1].last_episode, episode_number);
            printw("\nULTIMO EPISODIO DE %s: %s\n", avaliable_animes[avaliable_animes_count - 1].slug, episode_number);

            slug_start_position = episode_end_position + 1;
        }
        else
        {
            iterating = false;
            free(last_episodes_string);
        }
    }
}

int reset_logic(int *selection, char *mode, int *selected_anime, int *selected_episode, int *max_selection, char*** options)
{
    for (int i = 0; i < *max_selection + 1; i++) 
    {
        free((*options)[i]);
    }
    free(*options);
    *options = NULL;
    response_length = 0;
    response[0] = '\0';
    strcpy(response, "");
    free(avaliable_animes);
    free(avaliable_servers);
    avaliable_animes = NULL;
    avaliable_servers = NULL;
    avaliable_animes_count = 0;
    avaliable_servers_index = 0;
    *mode = 'a';
    *selection = 0;
    *selected_anime = 0;
    *selected_episode = 1;
    read_last_episode("/home/donar/.anime/.last_episodes");
    *max_selection = avaliable_animes_count - 1;

    for (int i = 0; i < avaliable_animes_count; i++)
    {
        *options = realloc(*options, ((i + 1) * sizeof(char*)));
        (*options)[i] = strdup(avaliable_animes[i].slug);
    }

    return 0;
}

int mpv_mp4upload_playback(char cookies_file_flag[], char file_link[])
{
    char reproduce_command[1000] = "mpv --really-quiet --tls-verify=no --http-header-fields='Referer: https://www.mp4upload.com/' ";
    strcat(reproduce_command, cookies_file_flag);
    strcat(reproduce_command, file_link);
    system(reproduce_command);
    return 0;
}

int mpv_pdrain_playback(char file_link[])
{
    char reproduce_command[100] ="mpv --really-quiet ";
    strcat(reproduce_command, file_link);
    system(reproduce_command);
    return 0;
}

int check_pdrain_link(char download_link[])
{
    response_length = 0;
    response[0] = '\0';
    strcpy(response, "");
  CURLcode result;
  CURL *curl;

  curl = curl_easy_init();
  curl_easy_setopt(curl, CURLOPT_BUFFERSIZE, 102400L);
  curl_easy_setopt(curl, CURLOPT_URL, download_link);
  curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
  curl_easy_setopt(curl, CURLOPT_USERAGENT, "curl/8.20.0");
  curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 50L);
  curl_easy_setopt(curl, CURLOPT_SSLVERSION, (long)CURL_SSLVERSION_TLSv1_2);
  curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, callback);


  result = curl_easy_perform(curl);

  curl_easy_cleanup(curl);
  curl = NULL;

  char *response_start = strstr(response, "success\":");
  char *first_f = strstr(response, "f");
  if (first_f == NULL || (response_start + strlen("success\":")) - first_f == 0 )
  {
    printf("RESPONSE: 1");
    return 1;
  } 
  else
  {
    printf("RESPONSE: 0");
    return 0;
  }
}

int menu_selected_episode_playback(char*** options, int selected_episode, int selected_anime, int **selection, int **max_selection, char **mode)
{
    char selected_episode_string[10];
    printw("MENU PLAYBACK SELECTED EPISODE: %i", selected_episode);
    sprintf(selected_episode_string, "%i", selected_episode);
    printw("FROM MENU PLAYBACK: %s, %i", selected_episode_string, selected_episode);
    get_episode_link(avaliable_animes[selected_anime].slug, selected_episode_string);
    get_download_links();
    bool reproduced = false;
    for (int i = 0; i < avaliable_servers_index; i++)
    {
        printw("\n SERVER NAME: %s, SERVER LINK: %s \n", avaliable_servers[i].name, avaliable_servers[i].download_link);
        refresh();
        printf("\n SERVER NAME: %s, SERVER LINK: %s \n", avaliable_servers[i].name, avaliable_servers[i].download_link);
        printw("\nNOMBRE: %s LINK: %s", avaliable_servers[i].name,avaliable_servers[i].download_link);
        printw("\n NOMRE DE SERVIDOR'%s'\n", avaliable_servers[i].name);
        printw("TOTAL SERVERS: %d\n", avaliable_servers_index);
        if (strcmp(avaliable_servers[i].name, "PDrain") == 0)
        {
            int link_status = check_pdrain_link(avaliable_servers[i].download_link);
            printw("LINK_STATUS: %i", link_status);
            refresh();
            if (link_status == 1)
            {
                printw("FAILED PDRAIN");
            }
            else
            {
                reproduced = true;
                printw("REPRODUCING PDRAIN");
                mpv_pdrain_playback(avaliable_servers[i].download_link);
            }
        }
        else if (strcmp(avaliable_servers[i].name, "MP4Upload") == 0)
        {
            printw("REPRODUCE VALUE: %i", reproduced);
            printf("REPRODUCE VALUE: %i", reproduced);
            refresh();
            if (reproduced == false)
            {
                reproduced = true;
                printw("REPRODUCING MP4UPLOAD: %s", avaliable_servers[i].download_link);
                mpv_mp4upload_playback(" --cookies-file=/home/donar/.anime/.cookies.txt ", avaliable_servers[i].download_link);
            }
        }
    }
    save_last_episode("/home/donar/.anime/.last_episodes", avaliable_animes[selected_anime].slug, selected_episode_string);
    **mode = 'f';
    for (int i = 0; i < **max_selection + 1; i++)
    {
        free((*options)[i]);
    }
    free(*options);
    *options = NULL;
    *options = realloc(*options, 3 * sizeof(char*));

    (*options)[0] = strdup("Continuar viendo");
    (*options)[1] = strdup("Volver a inicio");
    (*options)[2] = strdup("Salir");

    **selection = 0;
    **max_selection = 2;
}

int manage_enter(int *selection, char*** options, char *mode, int *max_selection, int* selected_anime, WINDOW *search,WINDOW *menu, int *selected_episode)
{
    if (*mode == 'a')
    {
        *selected_anime = *selection;

        int max_episodes;
        get_episodes(avaliable_animes[*selection].slug, &max_episodes);

        if (max_episodes > atoi(avaliable_animes[*selection].last_episode) + 20)
        {
            max_episodes = atoi(avaliable_animes[*selection].last_episode) + 20;
        }

        for (int i = 0; i < *max_selection + 1; i++)
        {
            free((*options)[i]);
        }
        free(*options);
        *options = NULL;

        for (int i = 0; i < max_episodes; i++)
        {
            char episode_number[10];
            sprintf(episode_number, "%i", atoi(avaliable_animes[*selection].last_episode) + i);
            *options = realloc(*options, (i + 1) * sizeof(char*));
            (*options)[i] = strdup(episode_number);
        }
        *selection = 0;
        *max_selection = max_episodes - 1;
        *mode = 'e';
    }
    else if ( *mode == 'e')
    {
        *selected_episode = atoi(*options[0]) + *selection;
        printw("PLAYBACK CALLED FROM MODE E");
        *mode = 'f';
        menu_selected_episode_playback(options, *selected_episode, *selected_anime, &selection, &max_selection, &mode);
        *mode = 'f';
    }
    else if ( *mode == 'f')
    {
        if (*selection == 0)
        {
            *mode = 'f';
            *selected_episode += 1;
            printw("FROM F MODE, SELECTED_EPISODE: %i", *selected_episode);
            printw("PLAYBACK CALLED FROM MODE F");
            menu_selected_episode_playback(options, *selected_episode, *selected_anime, &selection, &max_selection, &mode);
            *mode = 'f';
        }
        if (*selection == 1)
        {
            reset_logic(selection, mode, selected_anime, selected_episode, max_selection, options);
        }
        if (*selection == 2)
        {  
            for (int i = 0; i < 3; i++) 
            {
                free((*options)[i]);
            }
            free(*options);
            *options = NULL;
            free(avaliable_animes);
            free(avaliable_servers);
            delwin(search);
            delwin(menu);
            endwin();
            exit(0);
        }
    
    }
    return 0;
}

int manage_arrow_keys(int input, int* selection, int max_selection)
{   
    if (input == KEY_UP)
    {
        if (*selection > 0)
        {
            *selection -=  1;
        }
    }
    else if (input == KEY_DOWN)
    {
        if (*selection < max_selection)
        {
            *selection += 1;
        }
    }
    return 0;
}

int draw_menu_options(char **options, int selection, WINDOW *menu, int max_selection)
{
    wclear(menu);
    for (int i = 0; i < max_selection + 1; i++)
    {
        if (i == selection)
        {
            wattron(menu, COLOR_PAIR(1));
            wprintw(menu, "%i. %s\n", i + 1, options[i]);
            wattroff(menu, COLOR_PAIR(1));
        }
        else
        {
             wprintw(menu, "%i. %s\n", i + 1, options[i]);
        }
    }
    refresh();
    wrefresh(menu);
    refresh();
}

int menu_logic()
{
    initscr();
    cbreak();
    noecho();
    start_color();
    keypad(stdscr, true);
    curs_set(0);

    init_pair(1, COLOR_GREEN, COLOR_BLACK);

    int maxy = getmaxy(stdscr);
    int maxx = getmaxx(stdscr);
    int startx;
    int starty;
    int width = maxx;
    int height = maxy - 2;
    starty = (maxy - (maxy - 2));
    startx = 0;

    WINDOW* search = newwin(1, maxx, 0, 0);
    wprintw(search, "Search: ");
    WINDOW* menu = newwin(maxy - 1, maxx, 1, 0);
    keypad(menu, true);
    keypad(search, true);
    
    read_last_episode("/home/donar/.anime/.last_episodes");
    int selection = 0;
    int max_selection = avaliable_animes_count - 1;
    int selected_anime;
    int selected_episode;
    char** options = NULL;

    for (int i = 0; i < avaliable_animes_count; i++)
    {
        options = realloc(options, ((i + 1) * sizeof(char*)));
        options[i] = strdup(avaliable_animes[i].slug);
    }

    draw_menu_options(options, selection, menu, max_selection);

    refresh();
    wrefresh(search);
    wrefresh(menu);
    refresh();

    bool menu_running = true;
    char mode = 'a'; // A de anime E de episodes S de seach y F de final
    while (menu_running == true)
    {
        int input = getch();
        switch (input)
        {
            case KEY_UP:
                manage_arrow_keys(input, &selection, max_selection);
            break; 
            case KEY_DOWN:
                manage_arrow_keys(input, &selection, max_selection);
            break;
            case '\n':
                manage_enter(&selection, &options, &mode, &max_selection, &selected_anime, search, menu, &selected_episode);
            break;  
            case 'q':
                return 0;
            break;
        }
        draw_menu_options(options, selection, menu, max_selection);
    }
    return 0;
}

int main()
{
    menu_logic();
}