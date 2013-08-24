//em0 test
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdlib.h>
#include <curl/curl.h>
#include <regex.h>
#include <unistd.h>
#include "nagnu.h"
#include "getconf.c"


#define MAX_BUF 1755360
#define MAXLINE 1000

char wr_buf[MAX_BUF+2];
int wr_index;
char *match;


int main()
{

  getConf();
  return 0;

  printf("\n");
  get_data();

  return 0;
}


size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream)
{
  int segsize = size * nmemb;

  if( wr_index + segsize > MAX_BUF ) {
    * (int *)stream = 1;
    return 0;
  }

  *memcpy( (void *)&wr_buf[wr_index], ptr, (size_t)segsize );
  wr_index += segsize;
  wr_buf[wr_index] = 0;

  return segsize;
}




int get_data()
{
  CURL *curl;
  CURLcode curl_res;
  curl = curl_easy_init();
  char host[5] = "FALSE";
  
  if(curl) {
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 0);
    curl_easy_setopt(curl, CURLOPT_URL, server_address);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
    curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_ANY);
    curl_easy_setopt(curl, CURLOPT_USERPWD, user_pwd);
    curl_res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if ( curl_res == 0 ) {
      sort_data(host);
    } else {
      printf("Curl failed");
    }

  }
  
  return 0;
}

int service_problems() {

  char  line[2500];
  int   counter = 0;
  int   errorsCounter = 0;
  char  statuswarning[] = "statusBGWARNING'";
  char  statuscritical[] = "statusBGCRITICAL'";
  char  statusunknown[] = "statusBGUNKNOWN'";
  char  errorss[5000][500];
  int   i = 0;


  while(wr_buf != '\0') {
    line[counter] = wr_buf[i];
    ++i;
    ++counter;
    if(wr_buf[i] == '\n') {

      if((strcasestr(line, statuswarning) || strcasestr(line, statuscritical) || strcasestr(line, statusunknown)) && !strcasestr(line, "#comments")) {
        strcpy(errorss[errorsCounter], line);
        ++errorsCounter;
      }
    counter = 0;
    memset( line, '\0', sizeof(line) );
    }
    if(wr_buf[i] == '\0') {
      break;
    }
  }
  printf("Nr of error lines: %d\n", errorsCounter);
  return 0;
}



void sort_data(char host[]) {
 
  char statushostdown[] = "'statusHOSTDOWN'><A HREF='extinfo.cgi?type=1";
  char statusEven[] = "'statusEven'><A HREF='extinfo.cgi?type=1";
  char statusOdd[] = "'statusOdd'><A HREF='extinfo.cgi?type=1";
  char *hostname = malloc(sizeof(char) * 50);
  char *servicename = malloc(sizeof(char) * 100);
  char statuswarning[] = "statusBGWARNING'";
  char statuscritical[] = "statusBGCRITICAL'";
  char statusunknown[] = "statusBGUNKNOWN'";
  char hostLine[2500];
  char serviceLine[2500 + 1];
  int  hostCounter = 0;
  int  serviceCounter = 0;
  int  hostState = 0;
  int  serviceState;
  int  printHost = 0;
  int  type;
  char hits[250];
  char serviceStateName[20];
  
  for(size_t i=0; i <= (size_t)wr_buf; ++i) {
    hostState = 0;
    hostLine[hostCounter] = wr_buf[i];
    ++hostCounter;
    if(wr_buf[i] == '\n') {

        if(strcasestr(hostLine, statushostdown) || strcasestr(hostLine, statusEven) || strcasestr(hostLine, statusOdd)) {
          type = 0;
          hostname = match_string(hostLine, type);
          if(strcasestr(hostLine, statushostdown)) {
            hostState = 2;
            print_object(hostname, hostState, type);
            printf("\n\n");
          }
          
          if(hostState < 1) {
            for(int service = 0; service <= (size_t)wr_buf; ++service) {
              serviceLine[serviceCounter] = wr_buf[service];
              ++serviceCounter;
              if(wr_buf[service] == '\n') {
  
                sprintf(hits, "extinfo.cgi?type=2&host=%s&service=", hostname);
                if(strcasestr(serviceLine, hits)) {
                  if((strcasestr(serviceLine, statuswarning) || strcasestr(serviceLine, statuscritical) || strcasestr(serviceLine, statusunknown)) && !strcasestr(serviceLine, "#comments")) {
                    if(printHost == 0) {
                      type = 0;
                      print_object(hostname, hostState, type);
                      printf("\n");
                      printHost = 1;
                    }
                    type = 1;
                    if(strcasestr(serviceLine, statuswarning)) {
                      serviceState = 1;
                      strcpy(serviceStateName, "WARNING");
                    } else if(strcasestr(serviceLine, statuscritical)) {
                      serviceState = 2;
                      strcpy(serviceStateName, "CRITICAL");
                    } else if(strcasestr(serviceLine, statusunknown)) {
                      serviceState = 3;
                      strcpy(serviceStateName, "UNKNOWN");
                    }
                    servicename = match_string(serviceLine, type);
                    print_object(serviceStateName, serviceState, type);
                    printf(" %s\n", servicename);
                  }
                }
                serviceCounter = 0; 
                memset( serviceLine, '\0', sizeof(serviceLine) );
              } else if(wr_buf[service] == '\0') {
                break;
              }
            }
          }
          if(printHost == 1) {
            printf("\n");
          }
          printHost = 0;

        }
      
      hostCounter = 0;
      memset( hostLine, '\0', sizeof(hostLine) );
    } else if (wr_buf[i] == '\0') {
      free(servicename);
      free(hostname);
      break;
    }
  }

  return;
}






char * match_string(char line[], int type)
{
  regex_t regex;
  int typeregex;
  regmatch_t pmatch[8];
  size_t nmatch;
  char *pattern[20];
  if(type == 0) {
    nmatch = 3;
    *pattern = ".*host=\\(.*\\)' ";
  } else {
    nmatch = 8;
    *pattern = "<TD ALIGN=LEFT valign=center CLASS='\\(.*\\)'><A HREF='extinfo.cgi?type=2&host=\\(.*\\)&service=\\(.*\\)'>\\(.*\\)</A></TD>";
  }
  char *match = malloc(sizeof(char) * 100);

  typeregex = regcomp(&regex, *pattern, 0);
  if( typeregex ) { 
    fprintf(stderr, "Could not compile regex\n");
    exit(1);
   }
  typeregex = regexec(&regex, line, nmatch, pmatch, 0);
  if( !typeregex ){
    if(type == 0) {
      sprintf(match, "%.*s", (int)(pmatch[1].rm_eo - pmatch[1].rm_so), &line[pmatch[1].rm_so]);
    } else {
      sprintf(match, "%.*s", (int)(pmatch[4].rm_eo - pmatch[4].rm_so), &line[pmatch[4].rm_so]);
    }
    regfree(&regex);
  }

  return match;

}








int print_object(char *object, int state, int type)
{

  if(type == 1) {
    printf("  ");
  }

  if(state == 0) {
    printf("\033[42m");
  } else if(state == 1) {
    printf("\033[43m");
  } else if(state == 2) {
    printf("\033[41m");
  } else if(state == 3) {
    printf("\033[45m");
  }
  printf(" %s \033[0m", object);
  free(match);
  return 0;
}


