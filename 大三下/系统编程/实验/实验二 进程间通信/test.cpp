static int ensure_tree(void)
{
    return ensure_dir(CHAT_USER_HOME) == 0 &&
           ensure_dir(CHAT_APP_DIR) == 0 &&
           ensure_dir(CHAT_DATA_DIR) == 0 &&
           ensure_dir(CHAT_SERVER_FIFO_DIR) == 0 &&
           ensure_dir(CHAT_CLIENT_FIFO_DIR) == 0;
}

static int valid_username(const char *name)
{
    size_t i;

    if (name[0] == '\0')
        return 0;

    for (i = 0; name[i] != '\0'; i++)
    {
        if (!isalnum((unsigned char)name[i]) && name[i] != '_' && name[i] != '-')
            return 0;
    }

    return 1;
}
int main()
{
    if (argc != 3)
    {
        fprintf(stderr, "Usage: %s username password\n", argv[0]);
        return EXIT_FAILURE;
    }
    if (!valid_username(argv[1]))
    {
        fprintf(stderr, "Username may contain only letters, digits, '_' and '-'\n");
        return EXIT_FAILURE;
    }
}