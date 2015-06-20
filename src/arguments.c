#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "arguments.h"

char *cvalue = NULL;
char *evalue = NULL;
int svalue = 10;
int ch;

int get_arguments(int argc, char **argv)
{
    while ((ch = getopt(argc, argv, "c:e:s:")) != -1)
    {
        switch (ch)
        {
            // Path to the configuration file
            case 'c':
                cvalue = optarg;
                break;
            // Path to the excludes file
            case 'e':
                evalue = optarg;
                break;
            // Update interval in seconds
            case 's':
                svalue = strtoul(optarg, 0, 0);
                break;
        }
    }

    return 0;
}

