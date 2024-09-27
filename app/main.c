#include <readline/readline.h>
#include <readline/history.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "../src/lab.h"

void freeUp(char *strX)
{
    free(strX);
    strX = NULL;
} // meaningless comment

void prepareForExit(/*shell, */ char *prompt)
{
    // Free shell object.
    freeUp(prompt);
}

void afterLineProcessed(char **strArray, char *line)
{
    cmd_free(strArray);
    add_history(line); //todo: should we add the whole line or just t5he trimmed?
    //printf("%dAdded history: %s\n", getpid(), line);
    freeUp(line);
}



int main(int argc, char **argv)
{
    // printf("hello world\n");

    int c;
    while ((c = getopt(argc, argv, "v")) != -1)
    {
        switch (c)
        {
        case 'v':
            fprintf(stdout, "%s Version %d.%d\n", getProgramName(), lab_VERSION_MAJOR, lab_VERSION_MINOR);
            return 0;
            break;
        // case 'b':
        //   fprintf(stdout, "You said b.\n");
        //   break;
        // case 'c':
        //   fprintf(stdout, "You said c\n");
        //   break;
        default:
            printf("Default\n");
            break;
        }
    }

    // Main execution loop

    // Allocate shell object

    char *line;
    char *prompt = get_prompt("MY_PROMPT");
    using_history();

    struct shell sh;
    sh_init(&sh);
    while ((line = readline(prompt)))
    {
        char **formatted = cmd_parse(line);
        if (formatted[0] == NULL) {
            afterLineProcessed(formatted, line);
            continue;
        }
        bool was_builtin = do_builtin(&sh, formatted);
        if (was_builtin)
        {

            if (sh.exiting)
            {
                afterLineProcessed(formatted, line);
                prepareForExit(/*shell, */ prompt);
                return 0;
            }
        }
        else
        {
            // Fork and do command
            pid_t my_id = fork();
            if (my_id == -1)
            {
                // Fork failed
                fprintf(stderr, "Failed to start a new process.\n");
            }
            else if (my_id == 0)
            {
                // Child process
                char* cmdName = formatted[0];
                int error = execvp(cmdName, formatted);
                if (error == -1)
                {
                    fprintf(stderr, "An error occured during the child process.\n");
                    cmd_free(formatted);
                    freeUp(line);
                    prepareForExit(/*shell, */ prompt);
                    return -1; // todo: should print that it wasn't a valid command?
                }
                return 0;
            }
            else
            {
                // Parent process
                waitpid(my_id, NULL, 0);
                // printf("!! Done waiting!!\n");
            }
        }

        afterLineProcessed(formatted, line);
    }
    fprintf(stdout, "\n");
    freeUp(line);
    prepareForExit(/*shell,*/ prompt);

    return 0;
}
