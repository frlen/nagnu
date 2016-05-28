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

#define MAX_BUF 3510720
#define MAXLINE 2000

char wr_buf[MAX_BUF+2];
int wr_index;
char *match;
int ypos = 0;
int xpos = 0;
int reset_vars = 0;
int last_type = 0;
char *path = "excludes";
int num_strings = 0;
int longest_string = 0;
char **excludes_save;
extern char *cvalue;
int svalue;
char **errorss;
int errorsCounter = 0;

int main(int argc, char **argv)
{
    int i;
    get_arguments(argc, argv);
    get_conf();
    count_strings();
    excludes_save = malloc(num_strings * sizeof(char *));
    for (i = 0; i < num_strings; i++)
    {
        excludes_save[i] = malloc((longest_string+1) * sizeof(char));
    }
    get_excludes();

    initscr();
    while (true)
    {
        errorsCounter = 0;
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
        init_pair(4, COLOR_BLACK, 5);             // UNKNOWN
        init_pair(5, COLOR_WHITE, COLOR_BLACK);   // BLACK
        init_pair(6, COLOR_BLACK, COLOR_WHITE);   // WHITE/BLACK
        init_pair(7, COLOR_WHITE, COLOR_RED);     // WHITE/RED

        bkgd(COLOR_PAIR(5));

        get_data();
        refresh();
        reset_vars = 1;

        sleep(svalue);
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

  memcpy( (void *)&wr_buf[wr_index], ptr, (size_t)segsize );
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
  int i;
  
  if(curl) {
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 0);
    curl_easy_setopt(curl, CURLOPT_URL, server_address);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
    if(!strcmp(digest_auth, "yes")) {
        curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_DIGEST);
    } else {
        curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_ANY);
    }
    curl_easy_setopt(curl, CURLOPT_USERPWD, user_pwd);
    curl_res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if ( curl_res == 0 ) {
      service_problems();
      sort_data(host);
      for (i=0; i<=errorsCounter; i++) {
        free(errorss[i]);
      }
      free(errorss);
    } else {
      char msg_no_conn[]=" No connection to server! ";
      int row,col;
      getmaxyx(stdscr,row,col);
      attron(COLOR_PAIR(7));
      mvprintw(row/2,(col-strlen(msg_no_conn))/2,"%s",msg_no_conn);
      attroff(COLOR_PAIR(7));
    }

  }

  return 0;
}

char **service_problems()
{
  char  line[3500];
  int   counter = 0;
  char  status_hostdown[] = "DWN";
  char  status_hostunreachable[] = "UNR";
  char  status_warning[] = "WRN";
  char  status_critical[] = "CRI";
  char  status_unknown[] = "UNK";
  size_t i;

  errorss = malloc(sizeof(wr_buf));

  for(i=0; i <= (size_t)wr_buf; ++i)
  {
    if(wr_buf[i] != '\0')
    {
      line[counter] = wr_buf[i];
    } else {
      break;
    }
    if(line[counter] == '\n')
    {
      if((strcasestr(line, status_warning) || strcasestr(line, status_critical) || strcasestr(line, status_unknown) || strcasestr(line, status_hostdown) || strcasestr(line, status_hostunreachable)) && !strcasestr(line, "#comments")) {
        errorss[errorsCounter] = malloc(sizeof(char*)*counter+1);
        memset(errorss[errorsCounter], '\0', sizeof(char*)*counter+1);
        strcpy(errorss[errorsCounter], line);
        errorsCounter++;
      }
      memset(line, '\0', sizeof(line));
      counter = 0;
    } else {
      counter++;
    }
  }
  errorsCounter--;

  return errorss;
}

void sort_data(char hostar[])
{
  char status_hostdown[] = "DWN";
  char status_hostunreachable[] = "UNR";
  char status_warning[] = "WRN";
  char status_critical[] = "CRI";
  char status_unknown[] = "UNK";
  char *hostname;
  char *service_name;
  int  host_state = 0;
  int  service_state;
  int  print_host = 0;
  int  type;
  char hits[250];
  char service_state_name[20];
  int  exclude_counter = 0;
  int  is_exclude = 0;
  int  i;
  int  service = 0;
  char last_hostname[250];
  char **hostsdown;
  int  hostdown_counter = 0;
  int  hostcounter;

  memset(last_hostname, '\0', 250);
  hostsdown = malloc(1);

  // Determine wether a host is down, and if so, change the background to red.
  for(i=0; i <= errorsCounter; i++)
  {
    if(errorss[i] == '\0')
    {
        break;
    }

    if(strcasestr(errorss[i], status_hostdown) || strcasestr(errorss[i], status_hostunreachable))
    {

      // Check for hostname in excludes.
      type = 0;
      exclude_counter=0;
      hostname = match_string(errorss[i], type);
      while(exclude_counter < num_strings)
      {
        if(!strcmp(hostname, excludes_save[exclude_counter]))
        {
          is_exclude = 1;
          break;
        }
        ++exclude_counter;
      }

      // Move on to the next host if this one is in excludes.
      if(is_exclude == 1)
      {
        is_exclude = 0;
        free(hostname);
        continue;
      }

      //Host is down, change the background color and add to hostsdown.
      bkgd(COLOR_PAIR(7));

      hostsdown = realloc(hostsdown, (strlen(hostname)+1)+hostdown_counter * sizeof(hostsdown));
      hostsdown[hostdown_counter] = malloc(strlen(hostname)+1);
      strcpy(hostsdown[hostdown_counter], hostname);
      hostdown_counter++;

    }
  }

  for(i=0; i <= errorsCounter; i++)
  {
    if(errorss[i] == '\0')
    {
        break;
    }

    if(strcasestr(errorss[i], status_hostdown) || strcasestr(errorss[i], status_hostunreachable) || strcasestr(errorss[i], status_warning) || strcasestr(errorss[i], status_critical) || strcasestr(errorss[i], status_unknown))
    {
      host_state = 0;
      if(strcasestr(errorss[i], status_hostdown) || strcasestr(errorss[i], status_hostunreachable))
      {
        type = 0;
      } else {
        type = 100;
      }

      hostname = match_string(errorss[i], type);
      if(!strcmp(hostname, last_hostname))
      {
        continue;
      }

      // Check for hostname in excludes.
      exclude_counter=0;
      while(exclude_counter < num_strings)
      {
        if(!strcmp(excludes_save[exclude_counter], hostname))
        {
          is_exclude = 1;
          break;
        }
        ++exclude_counter;
      }

      // Move on to the next host if this one is in excludes.
      if(is_exclude == 1)
      {
        is_exclude = 0;
        free(hostname);
        continue;
      }

      // Move on if the host is down
      if(hostdown_counter > 0)
      {
        for(hostcounter=0; hostcounter < hostdown_counter; hostcounter++)
        {
          if(strcasestr(hostsdown[hostcounter], hostname))
          {
            host_state = 2;
          }
        }
      }

      if(strcasestr(errorss[i], status_hostdown))
      {
        host_state = 2;
        print_object(hostname, host_state, type);
      }
      if(strcasestr(errorss[i], status_hostunreachable))
      {
        host_state = 3;
        print_object(hostname, host_state, type);
      }

      if(host_state < 1)
      {
        for(service = 0; service <= errorsCounter; ++service)
        {
          sprintf(hits, "value='%s'/>", hostname);
          if(strcasestr(errorss[service], hits))
          {
            if((strcasestr(errorss[service], status_warning) || strcasestr(errorss[service], status_critical) || strcasestr(errorss[service], status_unknown)) && !strcasestr(errorss[service], "#comments"))
            {
              type = 1;
              service_name = match_string(errorss[service], type);
              exclude_counter = 0;
              while(exclude_counter < num_strings)
              {
                if(strcasestr(service_name, excludes_save[exclude_counter]))
                {
                  is_exclude = 1;
                  break;
                }
                ++exclude_counter;
              }
              if(is_exclude == 1)
              {
                is_exclude = 0;
                print_host = 0;
                free(service_name);
                continue;
              }
              if(print_host == 0)
              {
                type = 0;
                print_object(hostname, host_state, type);
                strcpy(last_hostname, hostname);
                print_host = 1;
              }
              type = 1;
              if(strcasestr(errorss[service], status_warning)) {
                service_state = 1;
                strcpy(service_state_name, "WARNING");
              } else if(strcasestr(errorss[service], status_critical)) {
                service_state = 2;
                strcpy(service_state_name, "CRITICAL");
              } else if(strcasestr(errorss[service], status_unknown)) {
                service_state = 3;
                strcpy(service_state_name, "UNKNOWN");
              }

              print_object(service_state_name, service_state, type);

              attron(A_BOLD);
              printw(" %s\n", service_name);
              attroff(A_BOLD);
              free(service_name);
            }
          }
        }
        print_host = 0;
      }
      free(hostname);
    }
  }


  if(hostdown_counter > 0) {
    for(hostcounter=0; hostcounter < hostdown_counter; hostcounter++)
    {
      free(hostsdown[hostcounter]);
    }
  }
  free(hostsdown);

}

char *match_string(char line[], int type)
{
  regex_t regex;
  int typeregex;
  regmatch_t pmatch[8];
  size_t nmatch;
  char *pattern[200];
  char *match;
  if(type == 0) {
    nmatch = 3;
    *pattern = "name='host' value='\\(.*\\)'/>";
  } else if(type == 100) {
    nmatch = 3;
    *pattern = "name='host' value='\\(.*\\)'/><postfield";
  } else {
    nmatch = 3;

    if(strcmp(cgi_version_new, "true"))
    {
      *pattern = "<TD ALIGN=LEFT valign=center CLASS='\\(.*\\)'><A HREF='extinfo.cgi?type=2&host=\\(.*\\)&service=\\(.*\\)'>\\(.*\\)</A></TD>"; /* Nagios < 3.5 */
    } else {
      *pattern = "<postfield name='service' value='\\(.*\\)'/>"; /* Nagios >= 3.5 */
    }
  }
  match = malloc(100);

  typeregex = regcomp(&regex, *pattern, 0);
  if( typeregex ) {
    fprintf(stderr, "Could not compile regex\n");
    exit(1);
  }
  typeregex = regexec(&regex, line, nmatch, pmatch, 0);
  if( !typeregex ){
    if((type == 0) || (type == 100 )) {
      sprintf(match, "%.*s", (int)(pmatch[1].rm_eo - pmatch[1].rm_so), &line[pmatch[1].rm_so]);
    } else {
      sprintf(match, "%.*s", (int)(pmatch[1].rm_eo - pmatch[1].rm_so), &line[pmatch[1].rm_so]);
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

    //If a host is down, make sure it SHOWS!
    if (state == 3)
    {
      attroff(COLOR_PAIR(state));
      attron(COLOR_PAIR(6));
      mvprintw(ypos, xpos, " %s ", object);
      attroff(COLOR_PAIR(6));
    } else {
      mvprintw(ypos, xpos, " %s ", object);
      attroff(COLOR_PAIR(state));
    }
    last_type = 0;
    if (state == 3)
    {
      ++ypos;
    }
  }
  attroff(COLOR_PAIR(state));
  ++ypos;
  return 0;
}
