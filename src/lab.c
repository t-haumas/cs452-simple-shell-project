#include "lab.h"

#include <errno.h>
#include <pwd.h>
#include <readline/history.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <wait.h>

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

void removeFromList(jobNode **jobList, jobNode *current, jobNode *previous, jobNode *next)
{
    // current will never be null.
    // current could be the first item, in which case previous would be null. In that case, set the jobList pointer to next.
    // next always may or may not be null.
    // printf("\n\n");
    // printJobList(*jobList);
    // printf("removing...\n");
    free(current->info.command);
    free(current);
    // printf("making next: %p\n", next);

    if (previous == NULL)
    {
        // printf("reset head\n");
        *jobList = next;
    }
    else
    {
        previous->next = next;
    }
    // printJobList(*jobList);
}

void reportAndManageFinishedJobs(jobNode **jobList, bool printAny, bool printAll)
{
    // printf("updating...%d\n", getpid());
    // printJobList(jobList);
    jobNode *previousNode = NULL;
    jobNode *currentNode = *jobList;
    // printf("pointer: %p\n", *jobList);
    // printJobList(*jobList);
    while (currentNode != NULL)
    {
        int doneWaiting = waitpid(currentNode->info.pid, NULL, WNOHANG);
        jobNode *nextNode = currentNode->next;
        if (doneWaiting != 0)
        {
            // printf("done!\n");
            if (printAny)
                printDone(currentNode->info);
            removeFromList(jobList, currentNode, previousNode, nextNode);
            // done waiting.
        } // else if (doneWaiting < 0) {
        //     fprintf(stderr, "Unable to check status of pid %d (%s).\n", currentNode->info.pid, strerror(errno));
        // }
        else
        {
            previousNode = currentNode;
            if (printAny && printAll)
                printJobRunning(currentNode->info);
        }
        currentNode = nextNode;
    }
}

// todo: delete this. for debugging only.
void printList(char **strArray)
{
    int idx = 0;
    printf("[");
    while (strArray[idx] != NULL)
    {
        printf("%s, ", strArray[idx]);
        idx++;
    }
    printf("]\n");
}

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

    char *mutableStr = strdup(constStr);

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
    {
        const char *homeDirectoryEnvVarContents = getenv("HOME");
        if (homeDirectoryEnvVarContents != NULL)
        {
            toDirectory = homeDirectoryEnvVarContents;
        }
        else
        {
            struct passwd *userEntry = getpwuid(getuid());
            errno = 0;
            if (userEntry == NULL)
            {
                if (errno == 0) {
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

    char *destroyableLine = strdup(line);
    char *trimmed = trim_white(destroyableLine);

    free(destroyableLine);

    const int maxArgCount = sysconf(_SC_ARG_MAX);
    char **arrayOfStrings = malloc(sizeof(char *) * maxArgCount + 1);

    char *currentToken = strtok(trimmed, delims);
    int currentTokenIndex = 0;
    while (currentToken != NULL && currentTokenIndex < maxArgCount)
    {
        arrayOfStrings[currentTokenIndex] = strdup(currentToken);
        currentTokenIndex++;
        currentToken = strtok(NULL, delims);
    }

    free(trimmed);
    arrayOfStrings[currentTokenIndex] = NULL;

    return arrayOfStrings;
}

void cmd_free(char **line)
{
    // printList(line);
    int strIdx = 0;
    while (line[strIdx] != NULL)
    {
        // printf("%d\n", strIdx);
        // printf("freeing '%s'\n", line[strIdx]);
        free(line[strIdx]);
        line[strIdx] = NULL;
        strIdx++;
        // printf("next: '%s'\n", line[strIdx + 1]);
    }

    free(line);
}

char *trim_white(char *line)
{
    char *trimmed = strdup(line);

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

    trimmed[endIdx] = '\0';
    char *trimmedRet = &trimmed[idx];

    char *final = strdup(trimmedRet);
    free(trimmed);
    return final;
}

bool do_builtin(struct shell *sh, char **argv)
{
    if (argv == NULL) {
        errno = EINVAL;
        perror("Invalid command string");
        return false;
    }

    if (argv[0] == NULL) {
        errno = EINVAL;
        perror("Invalid command string portion");
        return false;
    }

    char *cmd = argv[0];
    bool printAll = false;
    if (is(cmd, "jobs"))
    {
        printAll = true;
    }

    reportAndManageFinishedJobs(&jobList, true, printAll);

    if (printAll)
    {
        return true;
    }

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

    if (sh->shell_is_interactive)
    {
        while (tcgetpgrp(sh->shell_terminal) != (sh->shell_pgid = getpgrp()))
        {
            fprintf(stdout, "waiting\n");
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
            exit(1);
        }

        /* Grab control of the terminal.  */
        tcsetpgrp(sh->shell_terminal, sh->shell_pgid);

        /* Save default terminal attributes for shell.  */
        tcgetattr(sh->shell_terminal, &sh->shell_tmodes);
    }
    //todo: what about shell_pgid and shell_tmodes in the else case?
}

void sh_destroy(struct shell *sh)
{
    free(sh->prompt);
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
            exitAfterPrintingVersion = true;
            break;
        default:
            printf("Default\n");
            break;
        }
    }
}

const char *getProgramName()
{
    return "Simple Shell implemented by Thomas Ricks";
}
