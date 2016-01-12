#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "getconf.h"

char *file_name = "nagnu.conf";
char user[256];
char passwd[256];
char server_address[256];
char user_pwd[256];
char cgi_version_new[256];
extern char *cvalue;

int get_conf()
{
  int state = 1;
  int i = 0;
  int path_num;
  char conf_path[256];
  char *possible_paths[3];
  possible_paths[0] = "./";
  possible_paths[1] = "/etc/";
  possible_paths[2] = "/usr/local/etc/";

  if(cvalue != NULL)
  {
    state = look_for_conf(cvalue, 1);
  } else {
    for (i = 0; i < 3; i++)
    {
      state = look_for_conf(possible_paths[i], 0);
      if(state == 0)
      {
        path_num = i;
        break;
      }
    }
  }

  if ((state == 0) && (cvalue == NULL))
  {
    strcpy(conf_path,possible_paths[path_num]);
    strcat(conf_path,file_name);
  } else if ((state == 0) && (cvalue != NULL)) {
    strcpy(conf_path,cvalue);
  } else {
    printf("Could not find nagnu.conf, exiting.\n");
    exit(2);
  }

  read_conf(conf_path);

  return 0;
}

int read_conf(char path[])
{
  int c;
  int skip_line = 0;
  char store_input[512];
  memset(store_input, '\0', sizeof(store_input));
  const char *server = "server";
  const char *username = "username";
  const char *password = "password";
  const char *newcgi = "newcgi";
  int i = 0;
  int is_server = 0, is_username = 0, is_password = 0, is_newcgi = 0;

  FILE *fp;
  fp = fopen(path, "r");

  while ((c = getc(fp)) != EOF)
  {
    if (skip_line == 1 && c == '\n')
    {
      skip_line = 0;
    }

    if (skip_line == 1)
    {
      continue;
    }

    if (c == '#')
    {
      skip_line = 1;
      continue;
    }

    if (c != ' ')
    {
      if (c != '\n')
      {
        store_input[i] = c;
        i++;
      } else {
        if (is_server == 1)
        {
          strcpy(server_address, store_input);
          memset(store_input, '\0', sizeof(store_input));
          i = 0;
          is_server = 0;
        }

        if (is_username == 1)
        {
          strcpy(user, store_input);
          memset(store_input, '\0', sizeof(store_input));
          i = 0;
          is_username = 0;
        }

        if (is_password == 1)
        {
          strcpy(passwd, store_input);
          memset(store_input, '\0', sizeof(store_input));
          i = 0;
          is_password = 0;
        }
        if (is_newcgi == 1)
        {
          strcpy(cgi_version_new, store_input);
          memset(store_input, '\0', sizeof(store_input));
          i = 0;
          is_newcgi = 0;
        }
      }
    }

    if (c == ' ')
    {
      if (!strcmp(store_input,server))
      {
        i = 0;
        is_server = 1;
        memset(store_input, '\0', sizeof(store_input));
        continue;
      }

      if (!strcmp(store_input,username))
      {
        i = 0;
        is_username = 1;
        memset(store_input, '\0', sizeof(store_input));
        continue;
      }

      if (!strcmp(store_input,password))
      {
        i = 0;
        is_password = 1;
        memset(store_input, '\0', sizeof(store_input));
        continue;
      }
      if (!strcmp(store_input,newcgi))
      {
        i = 0;
        is_newcgi = 1;
        memset(store_input, '\0', sizeof(store_input));
        continue;
      }
    }
  }

  fclose(fp);
  strcat(server_address, "/cgi-bin/statuswml.cgi?style=uprobs");
  strcpy(user_pwd, user);
  strcat(user_pwd,":");
  strcat(user_pwd, passwd);

  return 0;

}

int look_for_conf(char path[], int cvalue)
{
  char full_path[256];

  strcpy(full_path,path);
  if (cvalue != 1)
  {
    strcat(full_path,file_name);
  }

  FILE *fp;
  if (!(fp = fopen(full_path, "r")))
  {
    return 1;
  } else {
    fclose(fp);
    return 0;
  }
}
