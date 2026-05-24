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
    char embedded_link[100];
    char download_link[100];
};

char response[100000] = "";
struct anime_data *avaliable_animes = NULL;
struct anime_data *avaliable_servers = NULL;

size_t callback(void *ptr, size_t size, size_t nmemb, void *userdata)
{
    strncat(response, ptr, size * nmemb);
    return size * nmemb;
}

static int dstderr_callback(CURL *handle, curl_infotype type, char *data, size_t size, void *userptr)
{
    (void)handle;
    (void)userptr;

    strcat(response, data, size);

    return 0;
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
  int episode_number_digits = episode_string_front - episode_string_back;
  static char episode_number[100];
  for (int i = 0; i < episode_number_digits; i++)
  {
    episode_number[i] = *(episode_string_back + i);
  }
  printf("%s", episode_number);
  return episode_number;
}

int get_episode_link(char slug[], char episode_number[])
{
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

    int avaliable_servers_number = 0;

    char* episode_sub_position = strstr(coincidence, "SUB");
    char* episode_dub_position = strstr(coincidence, "DUB");

    int orientation = episode_dub_position - episode_sub_position;
    
    char avaliable_servers

    parse_episode_servers_and_link(orientation, episode_dub_position, episode_sub_position);
}

int parse_episode_servers_and_links(int orientation, char* episode_dub_position, char* episode_sub_position)
{
    if (orientation > 0)
    {
        last_sub_dub_position = episode_dub_position;
    }
    else
    {
        last_sub_dub_position = episode_sub_position;
    }

    bool iterating = true;
    char *server = coincidence;
    while (iterating == true)
    {
        char *search = strstr(server, "server:\"");
        if ( search == NULL || last_sub_dub_position - search < 0)
        {
            iterating = false;
        }
        else
        {
            char server_name[20];
            server_characters_number = strstr(search, "\"") - search;
            for (int i = 0; i < server_characters_number; i++)
            {
                server_name[i] = search + i;
            }
            if (server_name == "PDrain" || server_name == "MP4Upload")
            {
                avaliable_servers_number += 1;
                avaliable_servers = realloc(avaliable_servers, avaliable_servers_number * sizeof(struct server_data));

                strcpy(avaliable_servers[avaliable_servers_number - 1].name, server_name);
                char *url_start = strstr(search, "url:\"");
                char *url_end = strstr(search, "\"");
                int url_lenght = url_end - url_start;
                char episode_link[100];
                for (int i = 0; i < url_lenght; i++)
                {
                    episode_link[i] = url_start + i;
                }
                strcpy(avaliable_servers[avaliable_servers_number - 1].embedded_link, episode_link);

            }
            server = search + strlen(substring);
        }
    } 
    return 0;   
}

char* get_mp4upload_download_link(char download_dir[], char embedded_link[], char file_id[])
{
  char post_data[] = "op=download2&";
  strcat(post_data, file_id);
  strcat(post_data, "&rand=&referer=https%3A%2F%2Fwww.mp4upload.com%2F&method_free=Free+Download");

  strcat(download_dir, "/.cookies.txt");

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
  curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE_LARGE, (curl_off_t)103);
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, slist1);
  curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0");
  curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 50L);
  curl_easy_setopt(curl, CURLOPT_COOKIEFILE, download_dir);
  curl_easy_setopt(curl, CURLOPT_SSLVERSION, (long)CURL_SSLVERSION_TLSv1_2);
  curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);
  curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, debug_callback);

  result = curl_easy_perform(curl);

  // Search for link on stderror data

  bool iterating = true;
  char *coincidence = &response[0];
  char substring[11] = "location: ";
  char download_link[200];

  while (iterating == true)
  {
    char *search = strstr(coincidence, substring);
    if ( search == NULL)
    {
        char* download_link_start = search;
        char* download_link_end = strstr(search, '\r');
        int url_length = download_link_end - download_link_start;
        for (int i = 0; i < url_length; i++)
        {
            download_link[i] = download_link_start + i;
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

  return &download_link[0];
}

int get_download_links(int avaliable_servers_index)
{
    if (avaliable_servers[avaliable_servers_index].name == "PDrain")
    {
        bool iterating = true;
        char *coincidence = &avaliable_servers[avaliable_servers_index].embedded_link[0];
        char substring[2] = "/";

            while (iterating == true)
            {
            char *search = strstr(coincidence, substring);
            if ( search == NULL)
            {
                iterating = false;
                char final_link[30] = "https://pixeldrain.com/api/file/";
                file_id = coincidence + 1;
                strcat(final_link, file_id);
                strcpy(avaliable_servers[avaliable_servers_index].download_link, final_link);
            }
            else
            {
                coincidence = search + strlen(substring);
            }

        } 
    }
    else if (avaliable_servers[avaliable_servers_index].name == "MP4Upload")
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
        curl_easy_setopt(curl, CURLOPT_COOKIEJAR, "/home/donar/.anime/.cookies.txt");
        curl_easy_setopt(curl, CURLOPT_SSLVERSION, (long)CURL_SSLVERSION_TLSv1_2);
        curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);
    
        result = curl_easy_perform(curl);

        curl_easy_cleanup(curl);
        curl = NULL;

        bool iterating = true;
        char *coincidence = &avaliable_servers[avaliable_servers_index].embedded_link[0];
        char substring[2] = "/";

        while (iterating == true)
        {
            char *search = strstr(coincidence, substring);
            if ( search == NULL)
            {
                iterating = false;
                file_id = coincidence + 1;
                download_link = get_mp4upload_download_link("/home/donar/.anime", embedded_link, file_id);
                strcpy(avaliable_servers[avaliable_servers_index].download_link, file_id);
            }
            else
            {
                coincidence = search + strlen(substring);
            }
        }
        
    }
}

int mpv_playback(char download_dir[], char file_link[])
{
    char cookies_file_flag[1000] = "--cookies-file=";
    strcat(cookies_file_flag, download_dir);
    strcat(cookies_file_flag, "/.cookies.txt' ");
    char reproduce_command[1000] = "mpv --really-quiet --tls-verify=no --http-header-fields='Referer: https://www.mp4upload.com/' ";
    strcat(reproduce_command, cookies_file_flag);
    strcat(reproduce_command, file_link);
    system(reproduce_command);
    return 0;
}

int main()
{
    get_episode_link("vinland-saga", "1");
    
    return 0;
}