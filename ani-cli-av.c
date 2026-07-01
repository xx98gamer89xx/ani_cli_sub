#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <ctype.h>
#ifdef _WIN32
    #include <curl/curl.h>
    #include "cjson/cJSON.h"
    #include "curses.h"
    #include "stdbool.h"
#else
    #include "cJSON.h"
    #include "ncurses.h"
    #include <curl/curl.h>
#endif

char *home;
char last_episodes_path[1024];

char cookies_file_path[1024];


struct anime_data
{
    char slug[1000];
    int max_episode;
    char last_episode[10];
};

struct server_data
{
    char name[100];
    char embedded_link[2000];
    char download_link[2000];
};

char* response;
struct anime_data *avaliable_animes = NULL;
struct server_data *avaliable_servers = NULL;
int avaliable_animes_count = 0;
int avaliable_servers_index = 0;
int search_chars_count = 0;
int dubbed = 0;
int download = 0;

struct MemoryStruct {
  char *memory;
  size_t size;
};
 
static size_t callback(char *contents, size_t size, size_t nmemb, void *userp)
{
  size_t realsize = size * nmemb;
  struct MemoryStruct *mem = (struct MemoryStruct *)userp;
 
  char *ptr = realloc(mem->memory, mem->size + realsize + 1);
  if(!ptr) {
    /* out of memory! */
    fprintf(stderr, "Fallo de memoria\n");
    return 0;
  }
 
  mem->memory = ptr;
  memcpy(&(mem->memory[mem->size]), contents, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = 0;
 
  return realsize;
}

size_t download_callback(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    size_t written = fwrite(ptr, size, nmemb, stream);
    return written;
}

static int progress_callback(void *clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow)
{   
    static int count = 0;
    count = (count + 1) % 20;
    if (count == 0)
    {
        double dl_percentage = 100 * ((double)dlnow / ((double)1024 * (double)1024)) / ((double)dltotal / ((double)1024 * (double)1024));
        clear();
        printw("\rDescargando: %i MB / %i MB (%.1f%%)", dlnow / (1024 * 1024), dltotal / (1024 * 1024), dl_percentage);
        refresh();
    }
    return 0;
}

static size_t discard_body(void *ptr,
                           size_t size,
                           size_t nmemb,
                           void *userdata)
{
    return size * nmemb;
}

static int response_length = 0;

static size_t header_callback(void *buffer, size_t size, size_t nitems, void *userdata)
{
    size_t realsize = size * nitems;
    struct MemoryStruct *mem = (struct MemoryStruct *)userdata;

    char *ptr = realloc(mem->memory, mem->size + realsize + 1);
    if (ptr == NULL)
    {
        fprintf(stderr, "Fallo de memoria\n");
        return 0; // Hace que libcurl aborte la transferencia
    }

    mem->memory = ptr;

    memcpy(mem->memory + mem->size, buffer, realsize);

    mem->size += realsize;
    mem->memory[mem->size] = '\0';

    return realsize;
}

void startup_checks()
{
    // MPV CHECK
    if (system("mpv --version") != 0)
    {
        printf("MPV is not found, install it and add it to your path");
    }
    // FILE CREATIONS
    char dir_name[1000];
    #ifdef _WIN32
        snprintf(dir_name, sizeof(dir_name), "%s\\.anime", home);
    #else
        snprintf(dir_name, sizeof(dir_name), "%s/.anime", home);
    #endif
    struct stat statbuf;

    if (stat(dir_name, &statbuf) == 0) {
    } else {
        #ifdef _WIN32
            if (mkdir(dir_name) != 0) 
            {
                fprintf(stderr, "Error creando directorio ~/.anime");
            }
        #else
            if (mkdir(dir_name, 0755) != 0) {
                fprintf(stderr, "Error creando directorio ~/.anime");
            }
        #endif
    }

    FILE *flast_episodes;
    flast_episodes = fopen(last_episodes_path, "a");

    fclose(flast_episodes);

    FILE *fcookies;
    fcookies = fopen(cookies_file_path, "a");
    fclose(fcookies);
}

void parse_search(cJSON *array)
{
  cJSON *slug;
  cJSON *id;
  cJSON *item = array ? array->child : 0;
  int i = 0;
  avaliable_animes_count = 0;
  while (item)
  {
        bool saved = false;
        slug = cJSON_GetObjectItem(item, "slug");
        item=item->next;
        avaliable_animes_count += 1;
        avaliable_animes = realloc(avaliable_animes, avaliable_animes_count * sizeof(struct anime_data));
        strcpy(avaliable_animes[i].slug, slug->valuestring);
        strcpy(avaliable_animes[i].last_episode, "1");
        i++;
  }
}

char* search_animes(char search[])
{
    struct MemoryStruct response_data;
    response_data.memory = malloc(1);
    response_data.size = 0;  
    response_length = 0;
    
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
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 0);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, search_string);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE_LARGE, strlen(search_string));
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, slist1);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "curl/8.20.0");
    curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 50L);
    curl_easy_setopt(curl, CURLOPT_SSLVERSION, (long)CURL_SSLVERSION_TLSv1_2);
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
    curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response_data);
    result = curl_easy_perform(curl);

    response = realloc(response, response_data.size + 1);
    memcpy(response, response_data.memory, response_data.size);
    *(response + response_data.size) = '\0';
    response_length = response_data.size;


    curl_easy_cleanup(curl);
    curl = NULL;
    curl_slist_free_all(slist1);
    slist1 = NULL;
    cJSON *json_response = cJSON_Parse(response);
    parse_search(json_response);
    cJSON_Delete(json_response);
    free(response_data.memory);
    free(response);
}

char* get_episodes(char slug[], int *max_episodes)
{
    struct MemoryStruct response_data;
    response_data.memory = malloc(1);
    response_data.size = 0;  

    response_length = 0;
    response = NULL;
    
    CURLcode result;
    CURL *curl;

    char anime_url[100] = "https://animeav1.com/media/";
    strcat(anime_url, slug);
    strcat(anime_url, "/1");

    curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_BUFFERSIZE, 102400L);
    curl_easy_setopt(curl, CURLOPT_URL, anime_url);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "curl/8.20.0");
    curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 50L);
    curl_easy_setopt(curl, CURLOPT_SSLVERSION, (long)CURL_SSLVERSION_TLSv1_2);
    curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response_data);
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 0);

    result = curl_easy_perform(curl);

    curl_easy_cleanup(curl);
    curl = NULL;

    response = realloc(response, response_data.size + 1);
    memcpy(response, response_data.memory, response_data.size);
    *(response + response_data.size) = '\0';
    response_length = response_data.size;

    bool iterating = true;
    char *coincidence = &response[0];
    char substring[100] = "number:";
    char episode_list_end_substring[100] = ",relations:";
    char* episode_list_end = strstr(coincidence, episode_list_end_substring);
    int i = 0;
    while (iterating == true)
    {
        char *search = strstr(coincidence, substring);
        if (search == NULL || search >= episode_list_end)
        {
            iterating = false;
        }
        else
        {
            coincidence = search + strlen(substring);
        }
        i+=1;
    }
    char *episode_string_back = coincidence;
    char *episode_string_front = strstr(episode_string_back, "}");
    int episodes_number_digits = episode_string_front - episode_string_back;
    static char episodes_number[100];
    for (int i = 0; i < episodes_number_digits; i++)
    {
        episodes_number[i] = *(episode_string_back + i);
    }
    episodes_number[episodes_number_digits] = '\0';
    *max_episodes = atoi(episodes_number);
    free(response_data.memory);
    free(response);
    return 0;
}

int parse_episode_servers_and_links(int orientation, char* episode_dub_position, char* episode_sub_position, char* server, char* mode)
{
    if (episode_dub_position == NULL)
    {
        if (dubbed == 1)
        {
            clear();
            printw("NO HAY VERSION DOBlADA DE ESTE CAPITULO, pulsa cualquier tecla para salir");
            *mode = 'e';
            refresh();
            getch();
            return 1;
        }
    }
    free(avaliable_servers);
    avaliable_servers = NULL;
    avaliable_servers_index = 0;

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
    }
    else
    {   
        while_condition = 0;
        last_sub_dub_position = episode_sub_position;
    }

    bool iterating = true;
    while (iterating == true)
    {
        char *search;
        int condition;
        if (while_condition == 0)
        {
            if (dubbed == 0)
            {
                search = strstr(episode_sub_position, "server:\"");
                condition = (search == NULL);
            }
            else
            {
                search = strstr(episode_dub_position, "server:\"");
                condition = (episode_sub_position - search < 0);
            }
        }
        else
        {
            if (dubbed == 1)
            {   
                search = strstr(episode_dub_position, "server:\"");
                condition = (search == NULL);
            }
            else
            {
                search = strstr(episode_sub_position, "server:\"");
                condition = (last_sub_dub_position - search < 0);
            }
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
                strcpy(avaliable_servers[avaliable_servers_index - 1].embedded_link, episode_link);
            }
            
            if (dubbed == 1)
            {
                episode_dub_position = episode_dub_position + strlen("server:\"");
            }
            else
            {
                episode_sub_position = episode_sub_position + strlen("server:\"");
            }


        }
    } 
    return 0;   
}


int get_episode_link(char slug[], char episode_number[], char* mode)
{
    struct MemoryStruct response_data;
    response_data.memory = malloc(1);
    response_data.size = 0;  

    response_length = 0;

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
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response_data);
  curl_easy_setopt(curl, CURLOPT_VERBOSE, 0);


  result = curl_easy_perform(curl);

  curl_easy_cleanup(curl);
  curl = NULL;

    response = NULL;
    response = realloc(response, response_data.size + 1);
    memcpy(response, response_data.memory, response_data.size);
    *(response + response_data.size) = '\0';
    response_length = response_data.size;

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

    if (parse_episode_servers_and_links(orientation, episode_dub_position, episode_sub_position, coincidence, mode) == 1)
    {
        free(response);
        free(response_data.memory);
        return 1;
    }
    else 
    {
        free(response);
        free(response_data.memory);
        return 0;
    }
}



int get_mp4upload_download_link(char cookies_jar[], char embedded_link[], char file_id[], int avaliable_servers_index)
{
    struct MemoryStruct header_data;
    header_data.memory = malloc(1);
    header_data.size = 0;

  char post_data[1000] = "op=download2&id=";
  strcat(post_data, file_id);
  strcat(post_data, "&rand=&referer=https%3A%2F%2Fwww.mp4upload.com%2F&method_free=Free+Download");

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
  curl_easy_setopt(curl, CURLOPT_HEADERDATA, &header_data);
  curl_easy_setopt(curl, CURLOPT_VERBOSE, 0);


  result = curl_easy_perform(curl);

    char* headers = malloc(header_data.size + 1);
    memcpy(headers, header_data.memory, header_data.size);
    *(headers + header_data.size) = '\0';

  // Search for link on http headers

  bool iterating = true;
  char *coincidence = headers;
  char substring[11];
  #ifdef _WIN32
    strcpy(substring, "Location: ");
  #else
    strcpy(substring, "location: ");
  #endif
  static char download_link[2000];
  while (iterating == true)
  {
    char *search = strstr(coincidence, substring);
    if ( search == NULL)
    {
        char* download_link_start = coincidence;
        if (strstr(coincidence, "\r\n") == NULL)
        {
            fprintf(stderr, "Fallo en la búsqueda del inicio del link de descarga");
            free(headers);
            free(header_data.memory);
            return 1;
        }
        else
        {
        char* download_link_end = strstr(coincidence, "\r\n");
        if (download_link_end != NULL)
        {
            int url_length = download_link_end - download_link_start;
            for (int i = 0; i < url_length; i++)
            {
                download_link[i] = *(download_link_start + i);
            }
            download_link[url_length] = '\0';
            iterating = false;
        }
        else
        {
            fprintf(stderr, "Fallo en la búsqueda del final del link de descarga");
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
  strcpy(avaliable_servers[avaliable_servers_index].download_link, download_link); 
    free(headers);
    free(header_data.memory);
  return 0;
}

int get_download_links()
{
    for (int i = 0; i < avaliable_servers_index; i++)
    {
        if (strcmp(avaliable_servers[i].name, "PDrain") == 0)
        {
            bool iterating = true;
            char *coincidence = &avaliable_servers[i].embedded_link[0];
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
            // Request to generate cookies
            CURLcode result;
            CURL *curl;

            curl = curl_easy_init();
            curl_easy_setopt(curl, CURLOPT_BUFFERSIZE, 102400L);
            curl_easy_setopt(curl, CURLOPT_URL, "https://www.mp4upload.com/j70hobym0b7k");
            curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
            curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0");
            curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 50L);
            curl_easy_setopt(curl, CURLOPT_COOKIEJAR, cookies_file_path);
            curl_easy_setopt(curl, CURLOPT_SSLVERSION, (long)CURL_SSLVERSION_TLSv1_2);
            curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, discard_body);
            curl_easy_setopt(curl, CURLOPT_VERBOSE, 0);

        
            result = curl_easy_perform(curl);

            curl_easy_cleanup(curl);
            curl = NULL;

            bool iterating = true;
            char *coincidence = avaliable_servers[i].embedded_link;

            while (iterating == true)
            {
                char *search = strstr(coincidence, "/");
                if ( search == NULL)
                {   
                    iterating = false;
                    char* file_id = coincidence;
                    get_mp4upload_download_link(cookies_file_path, avaliable_servers[i].embedded_link, file_id, i);
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

        for (int i = slug_index ; i + slug_length < strlen(last_episode_string); i++)
        {
            last_episode_string[i] = last_episode_string[i + slug_length + 1];
        }
        
        fclose(fptr);
    }
    // ADD SLUG TO BEGGINING OF FILE
    int last_episode_file_size;
    if ( last_episodes_file_size != 0)
    {
        last_episode_file_size = last_episodes_file_size * 5 * sizeof(char);
    }
    else
    {
        last_episode_file_size = 50 * sizeof(char);
    }
    char* last_episode_file = (char *)malloc(last_episode_file_size * sizeof(char));
    strcpy(last_episode_file, slug);

    strcat(last_episode_file, " ");
    strcat(last_episode_file, episode_number); 
    strcat(last_episode_file, "\n");
    strcat(last_episode_file, last_episode_string);
    fptr = fopen(last_episodes_file, "w");

    fprintf(fptr, "%s", last_episode_file);

    fclose(fptr);

    free(last_episode_file);
    free(last_episode_string);

    return 0;
}

int read_last_episode(char last_episodes_file[])
{
    FILE *fptr;
    fptr = fopen(last_episodes_file, "r");
    if (fptr == NULL)
    {
        fprintf(stderr, "Error abriendo archivo ~/.anime/.last_episodes");
        return 1;
    }
    fseek(fptr, 0, SEEK_END);
    long last_episodes_file_size = ftell(fptr);
    rewind(fptr);

    char *last_episodes_string = malloc((last_episodes_file_size + 1) * sizeof(char));
    if (last_episodes_string == NULL)
    {
        fprintf(stderr, "Falta de memoria");
        return 1;
    }
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
            if (episode_number_start_position != NULL)
            {
                for (int i = 0; i < episode_number_start_position - slug_start_position; i++ )
                {
                    slug[i] = *(slug_start_position + i);
                }

                slug[episode_number_start_position - slug_start_position] = '\0';
                avaliable_animes_count += 1;
                struct anime_data *temp = realloc(avaliable_animes, avaliable_animes_count * sizeof(struct anime_data));
                if (temp == NULL)
                {
                    fprintf(stderr, "Falta de memoria");
                    free(last_episodes_string);
                    return 1;
                }
                avaliable_animes = temp;
                strcpy(avaliable_animes[avaliable_animes_count - 1].slug, slug);
                for (int i = 0; i < episode_end_position - episode_number_start_position; i++ )
                {
                    episode_number[i] = *(episode_number_start_position + 1 + i);
                }

                episode_number[episode_end_position - episode_number_start_position] = '\0';
                strcpy(avaliable_animes[avaliable_animes_count - 1].last_episode, episode_number);

                slug_start_position = episode_end_position + 1;
            }
            else
            {
                iterating = false;
                free(last_episodes_string);
                return 0;
            }
        }
        else
        {
            iterating = false;
            free(last_episodes_string);
            return 0;
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
    read_last_episode(last_episodes_path);
    *max_selection = avaliable_animes_count - 1;

    for (int i = 0; i < avaliable_animes_count; i++)
    {
        *options = realloc(*options, ((i + 1) * sizeof(char*)));
        #ifdef _WIN32
            (*options)[i] = _strdup(avaliable_animes[i].slug);
        #else
            (*options)[i] = strdup(avaliable_animes[i].slug);
        #endif
    }

    return 0;
}

int mpv_mp4upload_playback(char cookies_file_flag[], char file_link[])
{
    char reproduce_command[1000];
    snprintf(reproduce_command, sizeof(reproduce_command), "mpv --really-quiet --tls-verify=no --http-header-fields=\"Referer: https://www.mp4upload.com/\"%s\"%s\"", cookies_file_flag, file_link);
    system(reproduce_command);
    refresh();
    return 0;
}

int mpv_pdrain_playback(char file_link[])
{
    char reproduce_command[100] ="mpv --really-quiet ";
    strcat(reproduce_command, file_link);
    system(reproduce_command);
    refresh();
    return 0;
}

int download_episode(char* url, char* slug, char* episode_number)
{
    CURL *curl;
    FILE *fp;
    CURLcode res;
    char* filepath = malloc(strlen(home) + 8 + strlen(slug) + 1 + strlen(episode_number) + 1 + 3 + 4 + 1);
    if ( dubbed == 0)
    {
        sprintf(filepath, "%s/.anime/%s_%s_%s.mp4", home, slug, episode_number, "SUB");
    }
    else
    {
        sprintf(filepath, "%s/.anime/%s_%s_%s.mp4", home, slug, episode_number, "DUB");
    }
    if (strstr(url, "pixeldrain.com") != NULL)
    {
        curl = curl_easy_init();
        if (curl) {
            fp = fopen(filepath,"wb");
            curl_easy_setopt(curl, CURLOPT_URL, url);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, download_callback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
            curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
            curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progress_callback);
            curl_easy_setopt(curl, CURLOPT_VERBOSE, 0);


            res = curl_easy_perform(curl);
            /* always cleanup */
            curl_easy_cleanup(curl);
            fclose(fp);
        }
        free(filepath);
        return 0;
    }
    else
    {
        CURLcode result;
        CURL *curl;
        struct curl_slist *slist1;

        slist1 = NULL;
        slist1 = curl_slist_append(slist1, "User-Agent: Mozilla/5.0");
        slist1 = curl_slist_append(slist1, "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8");
        slist1 = curl_slist_append(slist1, "Accept-Language: en-US,en;q=0.9");
        slist1 = curl_slist_append(slist1, "Accept-Encoding: gzip, deflate, br, zstd");
        slist1 = curl_slist_append(slist1, "Referer: https://www.mp4upload.com/");
        slist1 = curl_slist_append(slist1, "Sec-GPC: 1");
        slist1 = curl_slist_append(slist1, "Connection: keep-alive");
        slist1 = curl_slist_append(slist1, "Upgrade-Insecure-Requests: 1");
        slist1 = curl_slist_append(slist1, "Sec-Fetch-Dest: document");
        slist1 = curl_slist_append(slist1, "Sec-Fetch-Mode: navigate");
        slist1 = curl_slist_append(slist1, "Sec-Fetch-Site: same-site");
        slist1 = curl_slist_append(slist1, "Sec-Fetch-User: \?1");
        slist1 = curl_slist_append(slist1, "Priority: u=0, i");

        curl = curl_easy_init();
        fp = fopen(filepath,"wb");
        curl_easy_setopt(curl, CURLOPT_BUFFERSIZE, 102400L);
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, slist1);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "curl/8.20.0");
        curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 50L);
        curl_easy_setopt(curl, CURLOPT_COOKIEJAR, cookies_file_path);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        curl_easy_setopt(curl, CURLOPT_SSLVERSION, (long)CURL_SSLVERSION_TLSv1_2);
        curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, download_callback);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
        curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progress_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 0);


        result = curl_easy_perform(curl);

        curl_easy_cleanup(curl);
        curl = NULL;
        curl_slist_free_all(slist1);
        slist1 = NULL;
        fclose(fp);
        clear();
        printw("Search:");
        return 0;
    }
}

int check_pdrain_link(char download_link[])
{
    struct MemoryStruct response_data;
    response_data.memory = malloc(1);
    response_data.size = 0;  

    response_length = 0;
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
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response_data);
  curl_easy_setopt(curl, CURLOPT_VERBOSE, 0);



  result = curl_easy_perform(curl);

  curl_easy_cleanup(curl);
  curl = NULL;
    response = NULL;
    response = realloc(response, response_data.size + 1);
    memcpy(response, response_data.memory, response_data.size);
    *(response + response_data.size) = '\0';
    response_length = response_data.size;

  char *response_start = strstr(response, "success\":");
  char *first_f = strstr(response, "f");
  if (first_f == NULL || (response_start + strlen("success\":")) - first_f == 0 )
  {
    free(response);
    free(response_data.memory);
    return 1;
  } 
  else
  {
    free(response);
    free(response_data.memory);
    return 0;
  }
}

int menu_selected_episode_playback(char*** options, int selected_episode, int selected_anime, int **selection, int **max_selection, char **mode)
{
    char selected_episode_string[10];
    sprintf(selected_episode_string, "%i", selected_episode);
    if (get_episode_link(avaliable_animes[selected_anime].slug, selected_episode_string, *mode) == 1)
    {
        return 1;
    }
    get_download_links();
    bool reproduced = false;
    for (int i = 0; i < avaliable_servers_index; i++)
    {
        refresh();
        if (strcmp(avaliable_servers[i].name, "PDrain") == 0)
        {
            int link_status = check_pdrain_link(avaliable_servers[i].download_link);
            refresh();
            if (link_status == 1)
            {
                fprintf(stderr, "Fallo en link de Pdrain, intentando siguientes...");
            }
            else
            {
                if (download == 0)
                {
                    reproduced = true;
                    mpv_pdrain_playback(avaliable_servers[i].download_link);
                }
                else
                {
                    reproduced = true;
                    download_episode(avaliable_servers[i].download_link, avaliable_animes[selected_anime].slug, selected_episode_string);
                }
            }
        }
        else if (strcmp(avaliable_servers[i].name, "MP4Upload") == 0)
        {
            refresh();
            if (reproduced == false)
            {
                if (download == 0)
                {
                    reproduced = true;
                    char cookies_flag[1024];
                    snprintf(cookies_flag, sizeof(cookies_flag),
                    " --cookies-file=\"%s\" ",
                    cookies_file_path);
                    mpv_mp4upload_playback(cookies_flag, avaliable_servers[i].download_link);
                }
                else
                {
                    reproduced = true;
                    download_episode(avaliable_servers[i].download_link, avaliable_animes[selected_anime].slug, selected_episode_string);
                }
            }
        }
    }
    save_last_episode(last_episodes_path, avaliable_animes[selected_anime].slug, selected_episode_string);
    **mode = 'f';
    for (int i = 0; i < **max_selection + 1; i++)
    {
        free((*options)[i]);
    }
    free(*options);
    *options = NULL;
    *options = realloc(*options, 3 * sizeof(char*));

    #ifdef _WIN32
        (*options)[0] = _strdup("Continuar viendo");
        (*options)[1] = _strdup("Volver a inicio");
        (*options)[2] = _strdup("Salir");
    #else
        (*options)[0] = strdup("Continuar viendo");
        (*options)[1] = strdup("Volver a inicio");
        (*options)[2] = strdup("Salir");
    #endif

    **selection = 0;
    **max_selection = 2;
    refresh();
}

int manage_enter(int *selection, char*** options, char *mode, int *max_selection, int* selected_anime, WINDOW *search,WINDOW *menu, int *selected_episode, int max_episodes, char** search_string)
{
    if (*mode == 'a')
    {
        if (search_chars_count <= 0)
        {
            *selected_anime = *selection;

            int max_episodes;
            get_episodes(avaliable_animes[*selection].slug, &max_episodes);
            avaliable_animes[*selection].max_episode = max_episodes;
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

            for (int i = 0; i + atoi(avaliable_animes[*selection].last_episode) <= max_episodes; i++)
            {
                char episode_number[10];
                sprintf(episode_number, "%i", atoi(avaliable_animes[*selection].last_episode) + i);
                *options = realloc(*options, (i + 1) * sizeof(char*));
                #ifdef _WIN32
                    (*options)[i] = _strdup(episode_number);
                #else
                    (*options)[i] = strdup(episode_number);
                #endif
            }

            *max_selection = (max_episodes) - atoi(avaliable_animes[*selection].last_episode);
            *selection = 0;
            *mode = 'e';
        }
        else
        {
            search_animes(*search_string);
            free(*search_string);
            *search_string = NULL;
            search_chars_count = 0;

            for (int i = 0; i < *max_selection + 1; i++)
            {
                free((*options)[i]);
            }
            free(*options);
            *options = NULL;

            for (int i = 0; i < avaliable_animes_count; i++)
            {
                *options = realloc(*options, (i + 1) * sizeof(char*));
                #ifdef _WIN32
                    (*options)[i] = _strdup(avaliable_animes[i].slug);
                #else
                    (*options)[i] = strdup(avaliable_animes[i].slug);
                #endif
            }
            *selection = 0;
            *max_selection = avaliable_animes_count - 1;
            *mode = 'a';
        }
    }
    else if ( *mode == 'e')
    {
        if (search_chars_count == 0)
        {
            *selected_episode = atoi(*options[0]) + *selection;
        }
        else
        {
            *selected_episode = atoi(*search_string);
            free(*search_string);
            *search_string = NULL;
            search_chars_count = 0;
        }
        bool episodes_terminated = false;
        if (*selected_episode > max_episodes)
        {
            episodes_terminated = true;
            clear();
            printw("YA NO HAY MAS CAPITULOS DE ESTE ANIME, pulsa cualquier tecla para salir");
            refresh();
            getch();
        }
        *mode = 'f';
        if (episodes_terminated == true || menu_selected_episode_playback(options, *selected_episode, *selected_anime, &selection, &max_selection, &mode) == 1)
        {
            for (int i = 0; i < *max_selection + 1; i++)
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
        *mode = 'f';
    }
    else if ( *mode == 'f')
    {
        if (*selection == 0)
        {
            bool episodes_terminated = false;
            *mode = 'f';
            *selected_episode += 1;
            if (*selected_episode > max_episodes)
            {
                episodes_terminated = true;
                clear();
                printw("YA NO HAY MAS CAPITULOS DE ESTE ANIME, pulsa cualquier tecla para salir");
                refresh();
                getch();
            }
            if (episodes_terminated == true || menu_selected_episode_playback(options, *selected_episode, *selected_anime, &selection, &max_selection, &mode) == 1)
            {
                for (int i = 0; i < *max_selection + 1; i++)
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
                exit(0);                delwin(search);
                delwin(menu);
                endwin();
                exit(0);
            }
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

int manage_text(int input, char** search_string, WINDOW* search)
{
    if (isalpha(input) || input == ' ' || isdigit(input))
    {
        char *temp = realloc(*search_string, (search_chars_count + 1) * sizeof(char*));
        if (temp != NULL)
        {
            *search_string = temp;
        }
        (*search_string)[search_chars_count] = input;
        (*search_string)[search_chars_count + 1] = '\0';
        search_chars_count += 1;
    }
    else if (input == KEY_BACKSPACE || input == '\b' || input == 8 || input == 127)
    {
        if (search_chars_count > 0)
        {
            char* temp = realloc(*search_string, search_chars_count * sizeof(char*));
            if (temp != NULL)
            {
                fprintf(stderr, "Fallo de memoria");
                *search_string = temp;
            }
            search_chars_count -= 1;
            (*search_string)[search_chars_count] = '\0';
        }
    }
    wclear(search);
    if (*search_string != NULL)
    {
        wprintw(search, "Search:%s", *search_string);
    }
    else
    {
        wprintw(search, "Search:");
    }
    refresh();
    wrefresh(search);
    refresh();
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
    if (stdscr == NULL)
    {
        fprintf(stderr, "Initscr falló\n");
        exit(1);
    }

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
    read_last_episode(last_episodes_path);
    int selection = 0;
    int max_selection = avaliable_animes_count - 1;
    int selected_anime = 0;
    int selected_episode;
    char* search_string = NULL;

    char** options = NULL;

    for (int i = 0; i < avaliable_animes_count; i++)
    {
        options = realloc(options, ((i + 1) * sizeof(char*)));
        #ifdef _WIN32
            options[i] = _strdup(avaliable_animes[i].slug);
        #else
            options[i] = strdup(avaliable_animes[i].slug);
        #endif
    }

    draw_menu_options(options, selection, menu, max_selection);

    refresh();
    wrefresh(search);
    wrefresh(menu);
    refresh();

    bool menu_running = true;
    char mode = 'a'; // A de anime E de episodes y F de final
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
                manage_enter(&selection, &options, &mode, &max_selection, &selected_anime, search, menu, &selected_episode, avaliable_animes[selected_anime].max_episode, &search_string);
            break;  
            default:
                manage_text(input, &search_string, search);
            break;
        }
        draw_menu_options(options, selection, menu, max_selection);

        wrefresh(search);
        wrefresh(menu);
    }
    return 0;
}

int main(int argc, char* argv[])
{
    #ifdef _WIN32
        home = getenv("USERPROFILE");
        snprintf(cookies_file_path, sizeof(cookies_file_path),
         "%s\\.anime\\.cookies.txt",
         home);
        snprintf(last_episodes_path, sizeof(last_episodes_path),
         "%s\\.anime\\.last_episodes",
         home);

    #else
        home = getenv("HOME");
        snprintf(cookies_file_path, sizeof(cookies_file_path),
         "%s/.anime/.cookies.txt",
         home);
        snprintf(last_episodes_path, sizeof(last_episodes_path),
         "%s/.anime/.last_episodes",
         home);

    #endif
    if (argc > 1)
    {
        for (int i = 1; i < argc; i++)
        {
            if (strcmp(argv[i], "-d") == 0)
            {
                download = 1;
            }
            else if (strcmp(argv[i], "-D") == 0)
            {
                dubbed = 1;
            }
            else if (strcmp(argv[i], "-h") == 0)
            {
                printf("Opciones: \n -h Muestra este mensaje \n -D Activa el modo dub, para ver versiones dobladas (es posible que no haya version doblada, por defecto está desactivado) \n -d Activa el modo descargar (no stremea, guarda el archivo en %s/.anime, por defecto está desactivado)", home);
                exit(0);
            }
        }
    }
    else
    {
        char* config_file_path = malloc(strlen(home) + strlen("/.anime/config.txt") + 1);
        snprintf(config_file_path, strlen(home) + 19,"%s/.anime/config.txt", home);
        FILE* config_file;
        config_file = fopen(config_file_path, "r");
        
        if (config_file != NULL)
        {
            fseek(config_file, 0, SEEK_END);

            long config_file_size = ftell(config_file);
            rewind(config_file);

            char *config_file_string = malloc((config_file_size + 1) * sizeof(char));

            fread(config_file_string, sizeof(char), config_file_size, config_file);
            config_file_string[config_file_size] = '\0';

            fclose(config_file);

            if (strstr(config_file_string, "dubbed") != NULL)
            {
                dubbed = 1;
            }
            if (strstr(config_file_string, "download") != NULL)
            {
                download = 1;
            }
        }
        else
        {
            config_file = fopen(config_file_path, "a");
            fclose(config_file);
        }
        free(config_file_path);
    }
    startup_checks();
    menu_logic();
}
