#include <stdio.h>
#include <string.h>
#include "nagnu.h"

char *fileName = "nagnu.conf";
char user[256];
char passwd[256];
char server_address[256];
char user_pwd[256];

int getConf()
{
  int state;
  char confPath[256];
  char *possiblePaths[3];
  possiblePaths[0] = "./";
  possiblePaths[1] = "/etc/";
  possiblePaths[2] = "/usr/local/etc/";

  for (int i = 0; i < 3; i++)
  {
    state = lookForConf(possiblePaths[i]);
    if (state == 0)
    {
      strcpy(confPath,possiblePaths[i]);
      strcat(confPath,fileName);
      break;
    }
  }

  readConf(confPath);


  return 0;

}

int readConf(char path[])
{

  char c[256];
  int skipLine = 0;
  char storeInput[256];
  const char *uri = "uri";
  const char *username = "username";
  const char *password = "password";
  int i = 0;
  int isUri = 0, isUsername = 0, isPassword = 0;

  FILE *fp;
  fp = fopen(path, "r");

  while ((*c = getc(fp)) != EOF) 
  {

    if (skipLine == 1 && !strcmp(c, "\n"))
    {
      skipLine = 0;
    }

    if (skipLine == 1)
    {
      continue;
    }

    if (!strcmp(c, "#"))
    {
      skipLine = 1;
      continue;
    } 

    if (strcmp(c, " "))
    {
      if (strcmp(c, "\n"))
      {
        storeInput[i] = *c;
        i++;
      } else {
        if (isUri == 1)
        {
          strcpy(server_address, storeInput);
          memset(storeInput, '\0', sizeof(storeInput));
          i = 0;
          isUri = 0;
        }
        
        if (isUsername == 1)
        {
          //user = storeInput;
          strcpy(user, storeInput);
          memset(storeInput, '\0', sizeof(storeInput));
          i = 0;
          isUsername = 0;
        }
        
        if (isPassword == 1)
        {
          strcpy(passwd, storeInput);
          memset(storeInput, '\0', sizeof(storeInput));
          i = 0;
          isPassword = 0;
        }
        
      }

    }
  
    if (!strcmp(c, " ")) 
    {
      if (!strcmp(storeInput,uri))
      {
        i = 0;
        isUri = 1;
        memset(storeInput, '\0', sizeof(storeInput));
        continue;
      }

      if (!strcmp(storeInput,username))
      {
        i = 0;
        isUsername = 1;
        memset(storeInput, '\0', sizeof(storeInput));
        continue;
      }

      if (!strcmp(storeInput,password))
      {
        i = 0;
        isPassword = 1;
        memset(storeInput, '\0', sizeof(storeInput));
        continue;
      }

    }

  }
  fclose(fp);
  strcat(server_address, "/cgi-bin/status.cgi?host=all");
  strcpy(user_pwd, user);
  strcat(user_pwd,":");
  strcat(user_pwd, passwd);

  return 0;

}

int lookForConf(char path[])
{
  char fullPath[256];

  strcpy(fullPath,path);
  strcat(fullPath,fileName);

  FILE *fp;
  if (!(fp = fopen(fullPath, "r")))
  {
    printf("Can't find config file.");
    return 1;
  } else {
    fclose(fp);
    return 0;
  }
  

}
