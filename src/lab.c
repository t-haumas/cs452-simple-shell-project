#include "lab.h"

char *get_prompt(const char *env)
{
    return 0;
}

int change_dir(char **dir)
{
    return 0;
}

char **cmd_parse(char const *line)
{
    return 0;
}

void cmd_free(char **line)
{
}

char *trim_white(char *line)
{
    return 0;
}

bool do_builtin(struct shell* sh, char **argv)
{
    return false;
}

void sh_init(struct shell* sh)
{
}

void sh_destroy(struct shell* sh)
{
}

void parse_args(int argc, char **argv)
{
}

const char *getPromptName()
{
    return "Simple Shell implemented by Thomas Ricks";
}
