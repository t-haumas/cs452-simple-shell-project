#include "lab.h"

#include <errno.h> //todo: need this?
#include <pwd.h>
#include <readline/history.h>
#include <string.h>
#include <stdio.h>

bool is(char* one, char* two) {
    return strcmp(one, two) == 0;
}

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
    if (dir == NULL) {
        fprintf(stderr, "Trying to change directory, but passed in no arg array.\n"); //todo: beautify this.
        //todo: set errno
        return -1;
    }

    char* toDir = *dir;

    const char *toDirectory;
    if (toDir == NULL) {
        //fprintf(stdout, "nullHOME\n"); //todo: remove this, for debugging only.
        const char *homeDirectoryEnvVarContents = getenv("HOME");
        if (homeDirectoryEnvVarContents != NULL)
        {
            toDirectory = homeDirectoryEnvVarContents;
        }
        else
        {
            struct passwd *userEntry = getpwuid(getuid());
            if (userEntry == NULL)
            {
                fprintf(stderr, "Unable to change directory due to inability to find passwd user entry.\n");
                return -1;
            }
            else
            {
                toDirectory = userEntry->pw_dir;
            }
        }
    } else {
        // if (*toDir == NULL) {
        //     fprintf(stderr, "Directory is null somehow!\n"); //todo: what to do about this?
        // }
        toDirectory = toDir;
        //fprintf(stdout, "%s\n", toDirectory); //todo: remove this, for debugging only.
    }
    int result = chdir(toDirectory);
    if (result == 0)
    {
        //printf("Did it! to: %s\n", toDirectory); // todo: remove, for debugging only.
        return 0;
    }
    else
    {
        fprintf(stderr, "Unable to change directory to \"%s\".\n", toDirectory);
        //todo: set errno
        return -1;
    }
}

char **cmd_parse(char const *line)
{
    const char* delims = " ";

    char *trimmed = trim_white(line); //todo: maybe make a copy first?
    int numTokens = sysconf(_SC_ARG_MAX); //todo: make sure this is right.
    char** arrayOfStrings = malloc(sizeof(char*) * numTokens + 1);

    char* currentToken = strtok(trimmed, delims);
    int currentTokenIndex = 0;
    while (currentToken != NULL) {
        arrayOfStrings[currentTokenIndex] = currentToken;
        currentTokenIndex++;
        currentToken = strtok(NULL, delims);
    }

    arrayOfStrings[currentTokenIndex] = NULL;

    /* for debugging only*/
    // char* a = arrayOfStrings[0];
    // int idx = 0;
    // printf("[");
    // while (a != NULL) {
    //     printf("%s, ", a);
    //     idx++;
    //     a = arrayOfStrings[idx];
    // }
    // printf("]\n");

    //todo: delte the above section

    return arrayOfStrings;
}

void cmd_free(char **line)
{
    // int strIdx = 0;
    // printf("%s\n", line[1]);
    // while (line[strIdx] != NULL) {
    //     printf("%d\n", strIdx);
    //     printf("freeing '%s'\n", line[strIdx]);
    //     free(line[strIdx]);
    //     strIdx++;
    //     printf("%d\n\n", strIdx);
    //     printf("next: '%s'\n", line[strIdx]);
    // }

    // char* a = line[0];
    // int idx = 0;
    // while (a != NULL) {
    //     printf("freeing %s\n", a);
    //     free(line[idx]);
    //     idx++;
    //     a = line[idx];
    // }
    // printf("]\n");

    free(line[0]); //todo: why is this working? try printing the list after each step.
    free(line);
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
    char* cmd = argv[0];
    if (is(cmd, "cd")) {
        change_dir(&argv[1]);
        return true;
    } else if (is(cmd, "history")) {
        HIST_ENTRY** allHistory = history_list();
        if (allHistory) {
            int idx = 0;
            while (allHistory[idx] != NULL) {
                printf("\t- %s\n", allHistory[idx]->line);
                idx++;
            }
        }
        return true;
    } else if (is(cmd, "exit")) {
        sh->exiting = true;
        return true;
    } else {
        return false;
    }
}

void sh_init(struct shell* sh)
{
    sh->exiting = false;
    //todo: more.
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
