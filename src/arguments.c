char *cvalue = NULL;
char *evalue = NULL;
int ch;

int get_arguments(int argc, char **argv)
{
    while ((ch = getopt(argc, argv, "c:e:")) != -1)
    {
        switch (ch)
        {
            case 'c':
                cvalue = optarg;
                break;
            case 'e':
                evalue = optarg;
                break;
        }
    }

    return 0;
}

