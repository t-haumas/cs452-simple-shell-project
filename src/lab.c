#include "lab.h"

#include <string.h>
#include <stdio.h>

char *get_prompt(const char *env)
{
    const char* constStr = "> ";
    
    if (env != NULL)
    {
        const char *promptEnvVarContents = getenv("MY_PROMPT");
        if (promptEnvVarContents != NULL) {
            constStr = promptEnvVarContents;
        }
    }

    char* mutableStr = strdup(constStr);

    return mutableStr; //todo: can maybe simplify
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
    //Todo: implement this.
    char* trimmed;
    int length = strlen(line);

    trimmed = (char*)malloc((length + 1) * sizeof(char));

    if (trimmed == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        return line;
    }

    // Copy a string into the allocated memory
    strcpy(trimmed, line);
    return trimmed;
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

const char *getProgramName()
{
    return "Simple Shell implemented by Thomas Ricks";
}
