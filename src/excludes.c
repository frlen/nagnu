#include <stdio.h>
#include <string.h>
#include "excludes.h"
#include "nagnu.h"

extern char *evalue;

void count_strings()
{
    int string_length = 0;
    char c[512];
    FILE *fp = fopen(evalue, "r");

    if(fp)
    {
        while ((*c = getc(fp)) != EOF)
        {
            if(strcmp(c,"\n"))
            {
                string_length++;
                if(string_length > longest_string)
                {
                    longest_string = string_length;
                }
            } else {
                num_strings++;
                string_length = 0;
            }
        }
        fclose(fp);
    }
}

char **get_excludes()
{
    int i = 0;
    char c[1080];
    FILE *fp = fopen(evalue, "r");
    char store_input[512];
    int num_strings_cpy = num_strings-1;
    memset(store_input, '\0', sizeof(store_input));
    if(fp)
    {
        while ((*c = getc(fp)) != EOF)
        {
            if(strcmp(c, "\n"))
            {
                store_input[i] = *c;
                i++;
            } else {
                strcpy(excludes_save[num_strings_cpy], store_input);
                memset(store_input, '\0', sizeof(store_input));
                --num_strings_cpy;
                i = 0;
                if(num_strings_cpy == -1)
                {
                    break;
                }
            }
        }
    }
    return excludes_save;
}
