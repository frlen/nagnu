#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdlib.h>
#include <curl/curl.h>
#include <regex.h>
#include <unistd.h>
#include <curses.h>
#include "nagnu.h"
#include "getconf.h"
#include "excludes.h"
#include "arguments.h"

#define MAX_BUF 1755360
#define MAXLINE 1000

char wr_buf[MAX_BUF+2];
int wr_index;
char *match;
int ypos = 0;
int xpos = 0;
int reset_vars = 0;
int first_run = 0;
int last_type = 0;
char *path = "excludes";
int num_strings = 0;
int longest_string = 0;
char **get_excludes();
char **excludes_save;
extern char *cvalue;

int main(int argc, char **argv)
{
    if (first_run == 0)
    {
        get_arguments(argc, argv);
        get_conf();
        count_strings();
        excludes_save = malloc(num_strings * sizeof(char *));
        for (int i = 0; i < num_strings; i++)
        {
            excludes_save[i] = malloc((longest_string+1) * sizeof(char));
        }
        get_excludes();
    }

    initscr();
    while (true)
    {
        wr_index = 0;
        clear();
        if(has_colors() == FALSE)
        { 
            endwin();
            printf("Your terminal does not support color\n");
            return 1;
        }
        start_color();
        curs_set(0);

        init_pair(1, COLOR_BLACK, COLOR_GREEN);   // OK
        init_pair(2, COLOR_BLACK, COLOR_YELLOW);  // WARNING
        init_pair(3, COLOR_BLACK, COLOR_RED);     // CRITICAL
        init_pair(4, COLOR_BLACK, 5);   					// UNKNOWN

        get_data();
        refresh();

        reset_vars = 1;
        first_run = 1;

        sleep(10);
    }

    endwin();
    return 0;
}

size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream)
{
  int segsize = size * nmemb;

  if( wr_index + segsize > MAX_BUF ) {
    * (int *)stream = 1;
    return 0;
  }

  (void) *memcpy( (void *)&wr_buf[wr_index], ptr, (size_t)segsize );
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

int service_problems() 
{
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

      if((strcasestr(line, statuswarning) || strcasestr(line, statuscritical)	|| strcasestr(line, statusunknown)) && !strcasestr(line, "#comments")) {
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

void sort_data(char hostar[]) 
{
  char statushostdown[] = "'statusHOSTDOWN'><A HREF='extinfo.cgi?type=1";
  char statusEven[] = "'statusEven'><A HREF='extinfo.cgi?type=1";
  char statusOdd[] = "'statusOdd'><A HREF='extinfo.cgi?type=1";
  char statuswarning[] = "statusBGWARNING'";
  char statuscritical[] = "statusBGCRITICAL'";
  char statusunknown[] = "statusBGUNKNOWN'";
  char *hostname = malloc(sizeof(char) * 50);
  char *servicename = malloc(sizeof(char) * 100);
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
  int exclude_counter = 0;
  int is_exclude = 0;

  for(size_t i=0; i <= (size_t)wr_buf; ++i) 
  {
    if(wr_buf[i] == '\0')
    {
        break;
    }
    hostState = 0;
    if(wr_buf[i] == '\n') 
    {
        if(strcasestr(hostLine, statushostdown) || strcasestr(hostLine, statusEven) || strcasestr(hostLine, statusOdd)) 
        {
          type = 0;
          hostname = match_string(hostLine, type);
          if(strcasestr(hostLine, statushostdown)) 
          {
            hostState = 2;
            print_object(hostname, hostState, type);
            printf("\n\n");
          }
          
          if(hostState < 1) 
          {
            for(int service = 0; service <= (size_t)wr_buf; ++service) 
            {
              serviceLine[serviceCounter] = wr_buf[service];
              ++serviceCounter;
              if(wr_buf[service] == '\n') 
              {

                sprintf(hits, "extinfo.cgi?type=2&host=%s&service=", hostname);
                if(strcasestr(serviceLine, hits)) 
                {
                  
                  if((strcasestr(serviceLine, statuswarning) || strcasestr(serviceLine, statuscritical) || strcasestr(serviceLine, statusunknown)) && !strcasestr(serviceLine, "#comments")) 
                  {
                    type = 1;
                    servicename = match_string(serviceLine, type);
                    exclude_counter = 0;
                    while(exclude_counter < num_strings) 
                    {
                      if(strcasestr(servicename, excludes_save[exclude_counter])) 
                      {
                        is_exclude = 1;
                        break;
                      }
                      ++exclude_counter;
                    }
                    if(is_exclude == 1) 
                    {
                      is_exclude = 0;
                      printHost = 0;
                      serviceCounter = 0;
                      memset( serviceLine, '\0', sizeof(serviceLine));
                      continue;
                    }

                    if(printHost == 0) 
                    {
                      type = 0;
                      print_object(hostname, hostState, type);
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

                    print_object(serviceStateName, serviceState, type);
                    attron(A_BOLD);
                    printw(" %s\n", servicename);
                    attroff(A_BOLD);
                  }
                }
                serviceCounter = 0;
                memset( serviceLine, '\0', sizeof(serviceLine) );
              } else if(wr_buf[service] == '\0') {
                break;
              }
            }
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
    hostLine[hostCounter] = wr_buf[i];
    ++hostCounter;

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

    /* Switch the pattern rows below for nagios 3.5 or later */

    *pattern = "<TD ALIGN=LEFT valign=center CLASS='\\(.*\\)'><A HREF='extinfo.cgi?type=2&host=\\(.*\\)&service=\\(.*\\)'>\\(.*\\)</A></TD>"; /* Nagios < 3.5 */
    //*pattern = "<td align='left' valign=center class='\\(.*\\)'><a href='extinfo.cgi?type=2&host=\\(.*\\)&service=\\(.*\\)'>\\(.*\\)</a></td></tr>"; /* Nagios >= 3.5 */
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
  } else {
    printf("No match!");
  }

  return match;
}

int print_object(char *object, int state, int type)
{
  if (reset_vars == 1)
  {
    xpos = 0;
    ypos = 0;
    reset_vars = 0;
  }

  ++state;
  if (LINES <= ypos)
  {
    xpos = xpos+50;
    ypos = 0;
  }

  attron(COLOR_PAIR(state));
  if (type == 1)
  {
    ++ypos;
    if (last_type == 1)
    {
      --ypos;
    }
    mvprintw(ypos-1, xpos+2, " %s ", object);
    attroff(COLOR_PAIR(state));
    last_type = 1;
  } else {
    mvprintw(ypos, xpos, " %s ", object);
    attroff(COLOR_PAIR(state));
    last_type = 0;
  }
  attroff(COLOR_PAIR(state));
  ++ypos;
  free(match);
  return 0;
}
