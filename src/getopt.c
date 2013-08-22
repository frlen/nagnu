#include <stdio.h>
#include <string.h>
#include "nagnu.h"

char *fileName = "nagnu.conf";
char address[];
char servertype[];
char username[];
char password[];
char *server_address[];
char *user_pwd[];

int getOpt()
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

  FILE *fp;
  fp = fopen(path, "r");
  fprintf(fp, "testar\n");
  fclose(fp);
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
    printf("Hittade inte %s i %s\n", fileName, path);
    return 1;
  } else {
    fclose(fp);
    return 0;
  }
  

}
