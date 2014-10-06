char *cvalue = NULL;
int ch;

int get_arguments(int argc, char **argv)
{
    while ((ch = getopt(argc, argv, "c:")) != -1)
    {
        switch (ch)
        {
            case 'c':
                cvalue = optarg;
                break;
        }
    }

    return 0;
}

