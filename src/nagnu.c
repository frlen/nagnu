#include <curl/curl.h>
#include <curses.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include "nagnu.h"
#include "getconf.h"
#include "excludes.h"
#include "arguments.h"

#define MAX_BUF 3510720
#define MAXLINE 2000
#define HOST 0
#define SERVICE 1
#define CRITICAL 2
#define WARNING 3
#define UNKNOWN 4
#define TYPE 5
#define STATUS 6
#define OOK 7

char wr_buf[MAX_BUF];
int  wr_index;
char *path = "excludes";
int  num_strings = 0;
int  longest_string = 0;
int  svalue;
int  reset_vars = 0;
int  ypos = 0;
int  xpos = 0;
int  last_type = 0;
char *match;

struct hostprob {
    char *hostname;
    int  status;
    struct hostprob *nexthost;
    struct srvprob *srv;
};

struct srvprob {
    char *srvname;
    int  status;
    struct srvprob *nextsrv;
};

int main(int argc, char **argv)
{
    get_arguments(argc, argv);
    get_conf();

    initscr();
    while(true)
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

        init_pair(7, COLOR_BLACK, COLOR_GREEN);   // OK
        init_pair(3, COLOR_BLACK, COLOR_YELLOW);  // WARNING
        init_pair(2, COLOR_BLACK, COLOR_RED);     // CRITICAL
        init_pair(4, COLOR_BLACK, 5);             // UNKNOWN
        init_pair(5, COLOR_WHITE, COLOR_BLACK);   // BLACK
        init_pair(6, COLOR_BLACK, COLOR_WHITE);   // WHITE/BLACK
        init_pair(1, COLOR_WHITE, COLOR_RED);     // WHITE/RED

        count_strings();
        excludes_save = malloc(num_strings * sizeof(char*));
        if(excludes_save == 0)
        {
            printf("Out of memory\n");
            exit(-1);
        }
        int i;
        for (i = 0; i < num_strings; i++)
        {
            excludes_save[i] = malloc((longest_string+1) * sizeof(char));
            if(excludes_save[i] == 0)
            {
                printf("Out of memory\n");
                exit(-1);
            }
        }
        get_excludes();

        get_data();
        struct hostprob *hosthead = sort_data();

        if(hostisdown(&hosthead))
        {
            /* Make background red */
            bkgd(COLOR_PAIR(1));
        } else {
            /* Make background black */
            bkgd(COLOR_PAIR(5));

        }

        printlist(&hosthead);

        free_excludes();
        free_problems(hosthead);

        refresh();
        reset_vars = 1;

        sleep(svalue);
    }

    return 0;
}

hostisdown(struct hostprob **hosthead)
{

    struct hostprob *current = *hosthead;
    while(current->nexthost != NULL)
    {
        if(current->nexthost->status == 0)
        {
            return 1;
        }
        current = current->nexthost;
    }

    return 0;
}

int free_excludes()
{
    int i;
    for(i = 0; i < num_strings; i++)
    {
        free(excludes_save[i]);
    }
    free(excludes_save);

    return 0;
}

int free_problems(struct hostprob *hosthead)
{
    struct hostprob *current;

    while(hosthead->nexthost != NULL)
    {
        current = hosthead;
        hosthead = hosthead->nexthost;

        struct srvprob *currentsrv;
        while(hosthead->srv->nextsrv != NULL)
        {
            currentsrv = hosthead->srv;
            hosthead->srv = hosthead->srv->nextsrv;
            free(currentsrv);
        }

        free(current);

    }

    return 0;
}

int get_data()
{
    CURL *curl;
    CURLcode curl_res;
    curl = curl_easy_init();

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
            return 0;
        } else {
            print_error_box();
        }
    }
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

void print_error_box()
{
    char msg[]=" No connection to server! ";
    int row,col;
    getmaxyx(stdscr,row,col);
    attron(COLOR_PAIR(7));
    mvprintw(row/2,(col-strlen(msg))/2,"%s",msg);
    attroff(COLOR_PAIR(7));
}



struct hostprob *sort_data()
{
    int  i;
    int  counter = 0;
    int  type;
    int  status;
    char line[512];
    char *hostname;
    char *servicename;
    char *last_hostname = "";
    char *statusname;
    char *service;
    int excludes_counter = 0;

    struct hostprob *hosthead;
    hosthead = malloc(sizeof(struct hostprob));
    if(hosthead == 0)
    {
        printf("Out of memory\n");
        exit(-1);
    }
    hosthead->nexthost = NULL;

    memset(line, '\0', 512);


    for(i=0; i<=wr_index; i++) {
        if(wr_buf[i] != '\n') {
            line[counter] = wr_buf[i];
            counter++;
        } else {
            if(strcasestr(line, "anchor")) {
                type = get_type_or_status(line, TYPE);

                hostname = match_string(line, type);
                free(match);
                if(excluded(hostname)) {
                    memset(line, '\0', 512);
                    last_hostname = hostname;
                    counter = 0;
                    continue;
                }
                if(strcmp(hostname, last_hostname) && (type == HOST)) {
                    service = "NULL";
                    addhost(&hosthead, hostname, type, service);
                }
                if(type == SERVICE) {
                    status = get_type_or_status(line, STATUS);
                    service = match_string(line, -1);
                    free(match);
                    if(excluded(service)) {
                        memset(line, '\0', 512);
                        last_hostname = hostname;
                        counter = 0;
                        continue;
                    }
                    if(!findhostinlist(&hosthead, hostname)) {
                        addhost(&hosthead, hostname, type, service);
                    }
                    addsrv(&hosthead, hostname, status, service);
                }
                last_hostname = hostname;
            }
            memset(line, '\0', 512);
            counter = 0;
        }
    }

    return hosthead;

}

int excluded(char *hostname)
{
    int exclude_counter = 0;
    int value = 0;
    while(exclude_counter < num_strings)
    {
        if(!strcmp(hostname, excludes_save[exclude_counter]))
        {
            value = 1;
            break;
        }
        exclude_counter++;
    }

    return value;
}

int findhostinlist(struct hostprob **hosthead, char *hostname)
{
    int value;
    struct hostprob *current = *hosthead;
    while(current->nexthost != NULL) {

        if(!strcmp(current->nexthost->hostname, hostname)) {
            value = 1;
            break;
        } else {
            value = 0;
            current = current->nexthost;
        }
    }

    return value;
}

int addhost(struct hostprob **hosthead, char *hostname,
            int type, char *service)
{
    struct hostprob *current = *hosthead;
    while (current->nexthost != NULL) {
        current = current->nexthost;
    }
    current->nexthost = malloc(sizeof(struct hostprob));
    if(current->nexthost == 0)
    {
        printf("Out of memory\n");
        exit(-1);
    }
    current->nexthost->hostname = hostname;
    current->nexthost->status = type;
    current->nexthost->srv = NULL;
    current->nexthost->nexthost = NULL;

    return 0;
}

int addsrv(struct hostprob **hosthead, char *hostname,
           int status, char *service)
{
    struct hostprob *current = *hosthead;
    while(strcmp(current->nexthost->hostname,hostname))  {
        current = current->nexthost;
    }

    /* Create new struct for service problem */
    if(current->nexthost->srv == NULL) {
        current->nexthost->srv = malloc(sizeof(struct srvprob));
        if(current->nexthost->srv == 0)
        {
            printf("Out of memory\n");
            exit(-1);
        }
        current->nexthost->srv->srvname = service;
        current->nexthost->srv->status  = status;
        current->nexthost->srv->nextsrv = NULL;
    } else {
        struct srvprob *currentsrv = current->nexthost->srv;
        while(currentsrv->nextsrv != NULL) {
            currentsrv = currentsrv->nextsrv;
        }
        currentsrv->nextsrv = malloc(sizeof(struct srvprob));
        if(currentsrv->nextsrv == 0)
        {
            printf("Out of memory\n");
            exit(-1);
        }
        currentsrv->nextsrv->srvname = service;
        currentsrv->nextsrv->status  = status;
        currentsrv->nextsrv->nextsrv = NULL;
    }

    return 0;
}

/* Determine wheter it is a host or service problem */
int get_type_or_status(char line[], int typeorstatus)
{
    char st_hostdwn[] = "DWN";
    char st_hostunr[] = "UNR";
    char st_srvcrit[] = "CRI";
    char st_srvwarn[] = "WRN";
    char st_srvunkn[] = "UNK";

    if(typeorstatus == TYPE) {
        int type;
        if(strcasestr(line, st_hostdwn) || strcasestr(line, st_hostunr)) {
            type = HOST;
        } else if(strcasestr(line, st_srvcrit) || strcasestr(line, st_srvwarn)
               || strcasestr(line, st_srvunkn)) {
            type = SERVICE;
        } else {
            type = 3;
        }
        return type;
    } else {
        int status;
        if(strcasestr(line, st_srvcrit)) {
            status = CRITICAL;
        } else if (strcasestr(line, st_srvwarn)) {
            status = WARNING;
        } else {
            status = UNKNOWN;
        }
        return status;
    }
    return 1;
}

char * match_string(char line[], int type)
{
  regex_t regex;
  int typeregex;
  regmatch_t pmatch[8];
  size_t nmatch;
  char *pattern[200];
  nmatch = 3;
  if(type == HOST) {
    *pattern = "name='host' value='\\(.*\\)'/>";
  } else if(type == SERVICE) {
    *pattern = "name='host' value='\\(.*\\)'/><postfield";
  } else {
    *pattern = "<postfield name='service' value='\\(.*\\)'/>";
  }

  char *match = malloc(100);
  if(match == 0)
  {
      printf("Out of memory\n");
      exit(-1);
  }

  typeregex = regcomp(&regex, *pattern, 0);
  if( typeregex ) {
    fprintf(stderr, "Could not compile regex\n");
    exit(1);
  }
  typeregex = regexec(&regex, line, nmatch, pmatch, 0);
  if( !typeregex ){
    if((type == 0) || (type == 100 )) {
      sprintf(match, "%.*s", (int)(pmatch[1].rm_eo - pmatch[1].rm_so),
                      &line[pmatch[1].rm_so]);
    } else {
      sprintf(match, "%.*s", (int)(pmatch[1].rm_eo - pmatch[1].rm_so),
                      &line[pmatch[1].rm_so]);
    }
    regfree(&regex);
  } else {
    printf("No match!");
  }

  return match;
}

void printlist(struct hostprob **head) {
    int status;
    int srvstatus;
    char *hostname;
    char *srvname;
    struct hostprob *current = *head;

    while (current->nexthost != NULL) {
        hostname = current->nexthost->hostname;
        if(current->nexthost->status == HOST) {
            status = HOST;
            print_object(status, hostname, 0);
        } else {
            struct srvprob *currentsrv = current->nexthost->srv;
            status = OOK;
            srvstatus = currentsrv->status;
            srvname   = currentsrv->srvname;
            print_object(status, hostname, 0);
            print_object(srvstatus, srvname, 1);
            attron(A_BOLD);
            printw(" %s\n", srvname);
            attroff(A_BOLD);

            while (currentsrv->nextsrv != NULL) {
                currentsrv = currentsrv->nextsrv;
                srvstatus  = currentsrv->status;
                srvname = currentsrv->srvname;
                print_object(srvstatus, srvname, 1);
                attron(A_BOLD);
                printw(" %s\n", srvname);
                attroff(A_BOLD);
            }
        }
        if(current->nexthost != NULL) {
            current = current->nexthost;
        } else {
            break;
        }
    }
}

void print_object(int state, char *object, int type)
{
  char *statename;

  if(state == WARNING) {
      statename = "WARNING";
  }
  if(state == UNKNOWN) {
      statename = "UNKNOWN";
  }
  if(state == CRITICAL) {
      statename = "CRITICAL";
  }
  if (reset_vars == 1)
  {
    xpos = 0;
    ypos = 0;
    reset_vars = 0;
  }

  if (LINES <= ypos)
  {
    xpos = xpos+50;
    ypos = 0;
  }

  attron(COLOR_PAIR(state));
  if (type == 1)
  {
    ++ypos;
    if(last_type == 1)
    {
        --ypos;
    }
    mvprintw(ypos-1, xpos+2, " %s ", statename);
    attroff(COLOR_PAIR(state));
    last_type = 1;
  } else {

    //If a host is down, make sure it SHOWS!
    if (state == 0)
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
    if (state == 0)
    {
      ++ypos;
    }
  }
  attroff(COLOR_PAIR(state));
  ++ypos;
}
