#include "lab.h"

#include <errno.h>
#include <pwd.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <wait.h>
#include <readline/history.h>

void printJob(job info)
{
    printf("[%d] %d %s\n", info.jobNum, info.pid, info.command);
}

void printJobRunning(job info)
{
    printf("[%d] %d Running %s\n", info.jobNum, info.pid, info.command);
}

void printDone(job doneJob)
{
    printf("[%d] Done %s\n", doneJob.jobNum, doneJob.command);
}

void freeUp(void **ptr)
{
    free(*ptr);
    ptr = NULL;
}

/**
 * @brief removes an element from a linked list of jobs.
 *
 * @param jobList a pointer to a pointer to the head of the job linked list.
 * @param current the jobNode to remove from the list. Should never be NULL.
 * @param previous the jobNode before current in the list. If current is the
 * first element of the list, previous should be NULL.
 * @param next the jobNode after current in the list. If current is the last
 * element of the list, next should be NULL.
 */
void removeFromList(jobNode **jobList, jobNode *current, jobNode *previous, jobNode *next)
{
    // Current will never be null.
    // Current could be the first item, in which case previous would be null.
    // Next always may or may not be null.
    freeUp((void **)&current->info.command);
    freeUp((void **)&current);

    if (previous == NULL)
    {
        *jobList = next;
    }
    else
    {
        previous->next = next;
    }
}

void reportAndManageFinishedJobs(jobNode **jobList, bool printAny, bool printAll)
{
    if (jobList == NULL) // This shouldn't happen.
    {
        errno = EINVAL;
        perror("Error while reporting and managing jobs");
        return;
    }

    jobNode *previousNode = NULL;
    jobNode *currentNode = *jobList;
    while (currentNode != NULL) // Iterate through the whole list
    {
        jobNode *nextNode = currentNode->next;
        int doneWaiting = waitpid(currentNode->info.pid, NULL, WNOHANG); // Check if still waiting for the job to complete.
        if (doneWaiting != 0)
        {   // Job finished
            if (printAny)
                printDone(currentNode->info);
            removeFromList(jobList, currentNode, previousNode, nextNode);
        }
        else
        {   // Job still running
            previousNode = currentNode;
            if (printAny && printAll)
                printJobRunning(currentNode->info);
        }
        currentNode = nextNode;
    }
}

/**
 * @brief a helper function for testing if 2 strings are equivalent.
 *
 * @param one the first string
 * @param two another string
 * @return true if the strings are equivalent, false if not.
 */
bool is(char *one, char *two)
{
    return strcmp(one, two) == 0;
}

char *get_prompt(const char *env)
{
    const char *constStr = "shell>";

    if (env != NULL)
    {
        const char *promptEnvVarContents = getenv("MY_PROMPT");
        if (promptEnvVarContents != NULL)
        {
            constStr = promptEnvVarContents;
        }
    }

    char *mutableStr = strdup(constStr); // Duplicate to protect the contents of the const char*s, it is not good to give direct access to the environment variable contents.

    return mutableStr;
}

int change_dir(char **dir)
{
    if (dir == NULL)
    {
        errno = EINVAL;
        perror("Error in change_dir");
        return -1;
    }

    char *toDir = dir[1];

    const char *toDirectory;
    if (toDir == NULL)
    {   // No argument was provided
        const char *homeDirectoryEnvVarContents = getenv("HOME");
        if (homeDirectoryEnvVarContents != NULL)
        {
            toDirectory = homeDirectoryEnvVarContents;
        }
        else
        {   // Environment variable didn't work, now trying to get user passwd entry.
            errno = 0;
            struct passwd *userEntry = getpwuid(getuid());
            if (userEntry == NULL)
            {
                if (errno == 0)
                {
                    errno = ENOENT;
                }
                perror("Unable to get user passwd entry to find home directory");
                return -1;
            }
            else
            {
                toDirectory = userEntry->pw_dir;
            }
        }
    }
    else
    {
        toDirectory = toDir;
    }

    int result = chdir(toDirectory);
    if (result == 0)
    {
        return 0;
    }
    else
    {
        perror("Unable to change directory");
        return -1;
    }
}

char **cmd_parse(char const *line)
{
    const char *delims = " ";

    // Trim white
    char *destroyableLine = strdup(line);
    char *trimmed = trim_white(destroyableLine);
    freeUp((void **)&destroyableLine);

    // Allocate array
    const int maxArgCount = sysconf(_SC_ARG_MAX);
    char **arrayOfStrings = malloc(sizeof(char *) * maxArgCount + 1);
    if (arrayOfStrings == NULL) {
        perror("Error when parsing command");
        return NULL;
    }

    // Split trimmed string into tokens by spaces
    char *currentToken = strtok(trimmed, delims);
    int currentTokenIndex = 0;
    while (currentToken != NULL && currentTokenIndex < maxArgCount)
    {
        arrayOfStrings[currentTokenIndex] = strdup(currentToken); // Duplicate all the tokens for cleaner memory management
        currentTokenIndex++;
        currentToken = strtok(NULL, delims);
    }

    freeUp((void **)&trimmed); // Free all of original string, which has been broken up by strtok
    arrayOfStrings[currentTokenIndex] = NULL; // Put an end cap on the array

    return arrayOfStrings;
}

void cmd_free(char **line)
{
    int strIdx = 0;
    while (line[strIdx] != NULL)
    {
        freeUp((void **)&line[strIdx]);
        strIdx++;
    }

    freeUp((void **)&line);
}

char *trim_white(char *line)
{
    char *trimmed = strdup(line); // Let's not operate on the original string they passed in. Instead, return a modified copy.

    if (trimmed == NULL)
    {
        return trimmed;
    }

    if (strlen(trimmed) == 0)
    {
        return trimmed;
    }

    int idx = 0;
    while (idx < (int)strlen(trimmed) && trimmed[idx] == ' ')
    {
        idx++;
    }

    int endIdx = strlen(trimmed);
    while (endIdx > 0 && trimmed[endIdx - 1] == ' ')
    {
        endIdx--;
    }

    trimmed[endIdx] = '\0'; // End the string at the first trailing space
    char *trimmedRet = &trimmed[idx]; // Start the string after the first leading space

    // Duplicate the output string and free the modified one for clean memory management
    char *final = strdup(trimmedRet);
    freeUp((void **)&trimmed);

    return final;
}

bool do_builtin(struct shell *sh, char **argv)
{
    if (argv == NULL)
    {
        errno = EINVAL;
        perror("Invalid command string");
        return false;
    }

    if (argv[0] == NULL)
    {
        errno = EINVAL;
        perror("Invalid command string portion");
        return false;
    }

    char *cmd = argv[0];

    // If it is the jobs command, print all jobs and exit.
    bool printAll = false;
    if (is(cmd, "jobs"))
    {
        printAll = true;
    }

    reportAndManageFinishedJobs(&jobList, true, printAll); // Still need to report finished jobs and manage the list even if it wasn't the jobs command.

    if (printAll)
    {
        return true;
    }

    // If it was not the jobs command, continue execution here.
    if (is(cmd, "cd"))
    {
        change_dir(argv);
        return true;
    }
    else if (is(cmd, "history"))
    {
        HIST_ENTRY **allHistory = history_list();
        if (allHistory)
        {
            int idx = 0;
            while (allHistory[idx] != NULL)
            {
                printf("\t- %s\n", allHistory[idx]->line);
                idx++;
            }
        }
        return true;
    }
    else if (is(cmd, "exit"))
    {
        sh->exiting = true;
        return true;
    }
    else
    {
        return false;
    }
}

void sh_init(struct shell *sh)
{
    sh->exiting = false;
    sh->prompt = get_prompt("MY_PROMPT");
    sh->shell_terminal = STDIN_FILENO;
    sh->shell_is_interactive = isatty(sh->shell_terminal);

    if (sh->shell_is_interactive) // This will always be true if we are running on stdin and stdout.
    {
        while (tcgetpgrp(sh->shell_terminal) != (sh->shell_pgid = getpgrp()))
        {
            kill(-sh->shell_pgid, SIGTTIN);
        }

        signal(SIGINT, SIG_IGN);
        signal(SIGQUIT, SIG_IGN);
        signal(SIGTSTP, SIG_IGN);
        signal(SIGTTIN, SIG_IGN);
        signal(SIGTTOU, SIG_IGN);
        signal(SIGCHLD, SIG_IGN);

        sh->shell_pgid = getpid();
        if (setpgid(sh->shell_pgid, sh->shell_pgid) < 0)
        {
            perror("Couldn't put the shell in its own process group");
            sh->exiting = true;
            return;
        }

        // Grab control of the terminal
        tcsetpgrp(sh->shell_terminal, sh->shell_pgid);

        // Save default terminal attributes for shell
        tcgetattr(sh->shell_terminal, &sh->shell_tmodes);
    }
}

void sh_destroy(struct shell *sh)
{
    freeUp((void **)&sh->prompt);
}

void parse_args(int argc, char **argv)
{
    int c;
    while ((c = getopt(argc, argv, "v")) != -1)
    {
        switch (c)
        {
        case 'v':
            fprintf(stdout, "%s Version %d.%d\n", getProgramName(), lab_VERSION_MAJOR, lab_VERSION_MINOR);
            exit(0);
            break;
        default:
            break;
        }
    }
}

const char *getProgramName()
{
    return "Simple Shell implemented by Thomas Ricks";
}
