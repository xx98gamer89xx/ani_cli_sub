#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <curl/curl.h>
#include "cJSON.h"

struct anime_data
{
    char slug[40];
    char id[10];
};

struct server_data
{
    char name[20];
    char embedded_link[2000];
    char download_link[2000];
};

char response[10000000] = "";
struct anime_data *avaliable_animes = NULL;
struct server_data *avaliable_servers = NULL;

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

void parse_array(cJSON *array)
{
  cJSON *slug;
  cJSON *id;
  cJSON *item = array ? array->child : 0;
  int i = 0;
  while (item)
  {
        slug = cJSON_GetObjectItem(item, "slug");
        id = cJSON_GetObjectItem(item, "id");
        item=item->next;
        avaliable_animes = realloc(avaliable_animes, (i + 1) * sizeof(struct anime_data));
        strcpy(avaliable_animes[i].slug, slug->valuestring);
        strcpy(avaliable_animes[i].id, id->valuestring);
        printf("\n ID: %s \n SLUG: %s", avaliable_animes[i].id, avaliable_animes[i].slug);
        i++;
  }
}

char* search(char search[])
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
    parse_array(json_response);
    cJSON_Delete(json_response);
}

char* get_episodes(char slug[])
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
  return episodes_number;
}

int parse_episode_servers_and_links(int orientation, char* episode_dub_position, char* episode_sub_position, int substring_length, char* server)
{
    printf("\n PARSEADOR \n");
    char* last_sub_dub_position;
    int avaliable_servers_number = 0;
    if (orientation > 0)
    {
        last_sub_dub_position = episode_dub_position;
    }
    else
    {
        last_sub_dub_position = episode_sub_position;
    }

    bool iterating = true;
    while (iterating == true)
    {
        char *search = strstr(server, "server:\"");
        if ( search == NULL || last_sub_dub_position - search < 0)
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
                avaliable_servers_number += 1;
                avaliable_servers = realloc(avaliable_servers, avaliable_servers_number * sizeof(struct server_data));
                printf("NOMBRE DEL SERVIDOR: %s \n", server_name);
                strcpy(avaliable_servers[avaliable_servers_number - 1].name, server_name);
                char *url_start = strstr(search, "url:\"");
                char *url_end = strstr(url_start + strlen("url:\""), "\"");
                int url_lenght = url_end - (url_start + strlen("url:\""));
                char episode_link[1000];
                for (int i = 0; i < url_lenght; i++)
                {
                    episode_link[i] = *(url_start + strlen("url:\"") + i);
                }
                episode_link[url_lenght] = '\0';
                printf("LINK DE EPISODIO: %i", url_lenght);
                strcpy(avaliable_servers[avaliable_servers_number - 1].embedded_link, episode_link);
            }
            server = search + strlen("server:\"");
        }
    } 
    return 0;   
}

int get_episode_link(char slug[], char episode_number[])
{
    response_length = 0;
    response[0] = '\0';
    strcpy(response, "");

    char episode_page_link[100] = "https://animeav1.com/media/";
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

    int orientation = episode_dub_position - episode_sub_position;
    

    parse_episode_servers_and_links(orientation, episode_dub_position, episode_sub_position, strlen(substring), coincidence);
}



int get_mp4upload_download_link(char cookies_jar[], char embedded_link[], char file_id[], int avaliable_servers_index)
{
  printf("MP$ UPLOAD REQUEST");
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

  printf("MP$ UPLOAD REQUEST");

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
        printf("EMPIEZP: %s", download_link_start);
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
            printf("HOLA");
            download_link[url_length] = '\0';
            printf("ADIOS");
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
  printf("NUMERO DE SERVIDORSE %i", avaliable_servers_index);
  strcpy(avaliable_servers[avaliable_servers_index].download_link, download_link); 

    printf("\n DOWNLOAD LINK: %s \n", download_link);

  return 0;
}

int get_download_links(int avaliable_servers_index)
{
    printf("INICIADO GET DOWNLOAD LINKS");
    if (strcmp(avaliable_servers[avaliable_servers_index].name, "PDrain") == 0)
    {
        printf("PDRAIN");
        bool iterating = true;
        char *coincidence = &avaliable_servers[avaliable_servers_index].embedded_link[0];
        printf("COINCIDENCE: %s", coincidence);
        char substring[2] = "/";

        while (iterating == true)
            {
            char *search = strstr(coincidence, substring);
            if ( search == NULL)
            {
                iterating = false;
                char final_link[10000] = "https://pixeldrain.com/api/file/";
                char* file_id = coincidence + 1;
                strcat(final_link, file_id);
                printf("%s", final_link);
                strcpy(avaliable_servers[avaliable_servers_index].download_link, final_link);
            }
            else
            {
                coincidence = search + strlen(substring);
            }

        } 
    }
    else if (strcmp(avaliable_servers[avaliable_servers_index].name, "MP4Upload") == 0)
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
        char *coincidence = avaliable_servers[avaliable_servers_index].embedded_link;

        

        while (iterating == true)
        {
            char *search = strstr(coincidence, "/");
            if ( search == NULL)
            {   
                iterating = false;
                char* file_id = coincidence;
                get_mp4upload_download_link("/home/donar/.anime/.cookies.txt", avaliable_servers[avaliable_servers_index].embedded_link, file_id, avaliable_servers_index);
            }
            else
            {
                coincidence = search + strlen("/");
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

char* read_last_episode(char last_episodes_file[], char slug[])
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

    char* slug_start_position = strstr(&last_episodes_string[0], slug);
    char* slug_end_position = strstr(slug_start_position, "\n");

    char* episode_number_start_position = strstr(slug_start_position + strlen(slug), " ") + 1;

    char* episode_number = NULL;
    int episode_number_length = slug_end_position - episode_number_start_position;

    int j = 0;

    for (int i = episode_number_start_position - last_episodes_string; i < (episode_number_start_position - last_episodes_string) + episode_number_length; i++)
    {
        episode_number = realloc(episode_number, (j + 1) * sizeof(char));
        episode_number[j] = *(last_episodes_string + i);
        j += 1;
    }

    episode_number = realloc(episode_number, (j + 1) * sizeof(char));
    episode_number[j] = '\0';

    free(last_episodes_string);
    printf("NUMERO DE EPISODIO: %s", episode_number);
    return episode_number;
}

int mpv_mp4upload_playback(char cookies_file_flag[], char file_link[])
{
    char reproduce_command[1000] = "mpv --really-quiet --tls-verify=no --http-header-fields='Referer: https://www.mp4upload.com/' ";
    strcat(reproduce_command, cookies_file_flag);
    strcat(reproduce_command, file_link);
    system(reproduce_command);
    return 0;
}

int main()
{
    search("vinland saga");
    int selected_anime_index = 0;
    char *total_episodes_number = get_episodes(avaliable_animes[selected_anime_index].slug);
    char selected_episode[] = "1";
    get_episode_link(avaliable_animes[selected_anime_index].slug, selected_episode);
    get_download_links(0);
    mpv_mp4upload_playback(" --cookies-file=/home/donar/.anime/.cookies.txt ", avaliable_servers[0].download_link);
    return 0;
}