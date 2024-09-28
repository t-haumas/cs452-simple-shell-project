#include <readline/readline.h>
#include <readline/history.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "../src/lab.h"

int nextJobID;

typedef struct job {
    int jobNum;
    pid_t pid;
    char* command;
} job;

typedef struct jobNode {
    job info;
    struct jobNode* next;
} jobNode;

jobNode* jobList;

jobNode* createJobNode(job newJob) {
    jobNode* newJobNode = (jobNode*)malloc(sizeof(jobNode));
    newJobNode->info = newJob;
    newJobNode->next = NULL;
    return newJobNode;
}

void append(jobNode** head, job newJob) {
    jobNode* previousNode = NULL;
    jobNode* currentNode = *head;
    while (currentNode != NULL) {
        previousNode = currentNode;
        currentNode = currentNode->next;
    }
    if (previousNode == NULL) {
        printf("making new head\n");
        *head = createJobNode(newJob);
    } else {
        previousNode->next = createJobNode(newJob);
    }
}

void printJob(job info) {
    printf("[%d] %d %s\n", info.jobNum, info.pid, info.command);
}


void printJobList(jobNode* head) {
    jobNode* currentNode = head;
    while (currentNode != NULL) {
        //printf("want to print\n");
        printJob(currentNode->info);
        currentNode = currentNode->next;
    }
}

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

void setUpChildProcessGroupAndForeground(pid_t id, struct shell* sh, bool isForeground) {
    setpgid(id, id);
    if (isForeground)
        tcsetpgrp(sh->shell_terminal, id);
}

int getLength(char** command) {
    int len = 0;
    while (command[len] != NULL) {
        len++;
    }
    return len;
}

bool getIsBackground(char** command) {
    int cmd_len = getLength(command);

    char* lastWord = command[cmd_len - 1];
    int wordLen = strlen(lastWord);
    if (wordLen < 1) {
        return false;
    }
    char lastChar = lastWord[wordLen - 1];
    if (lastChar == '&') {
        lastWord[wordLen - 1] = '\0';
        return true;
    }
    return false;
}



int main(int argc, char **argv)
{
    // printf("hello world\n");
    nextJobID = 0;
    jobList = NULL;

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
            // get is background
            bool isForeground = !getIsBackground(formatted);
            //printf("is foreground: %d\n", isForeground);
            // Fork and do command
            pid_t my_id = fork();
            if (my_id == -1)
            {
                // Fork failed
                fprintf(stderr, "Failed to start a new process.\n");
                exit(1);
            }
            else if (my_id == 0)
            {
                // Child process
                if (sh.shell_is_interactive) {
                    //fprintf(stdout, "interactive!\n");
                    pid_t child = getpid();
                    setUpChildProcessGroupAndForeground(child, &sh, isForeground);
                    signal(SIGINT, SIG_DFL);
                    signal(SIGQUIT, SIG_DFL);
                    signal(SIGTSTP, SIG_DFL);
                    signal(SIGTTIN, SIG_DFL);
                    signal(SIGTTOU, SIG_DFL);
                }
                char* cmdName = formatted[0];
                int error = execvp(cmdName, formatted);
                // if (error == -1)
                // {
                    fprintf(stderr, "An error occured during the child process.\n");
                    cmd_free(formatted);
                    freeUp(line);
                    prepareForExit(/*shell, */ prompt);
                    exit(1);// -1; // todo: should print that it wasn't a valid command?
                // } //todo: clean this area up.
                // return 0;
            }
            else
            {
                // Parent process
                if (sh.shell_is_interactive) {
                    setUpChildProcessGroupAndForeground(my_id, &sh, isForeground);

                    if (isForeground) {

                    waitpid(my_id, NULL, 0);
                     tcsetpgrp(sh.shell_terminal, getpgid(getpid())); //todo: probably split these functions and do error checking.
                    } else {
                        job newJob;
                        newJob.command = strdup(line);
                        newJob.jobNum = nextJobID++;
                        newJob.pid = my_id;
                        append(&jobList, newJob);
                        //printf("[%d] %d %s\n", nextJobID++, my_id, line); //todo: change this to print node(?)
                        printJobList(jobList);
                    }

                /* Restore the shell’s terminal modes.  */
                // tcgetattr(shell_terminal, &j->tmodes); // todo: is this necessary?
                // tcsetattr(shell_terminal, TCSADRAIN, &shell_tmodes);
                }
                else {
                    //todo: need to set child process group? Example seems like you don't need to.
                    waitpid(my_id, NULL, 0);

                }

                
                //printf("!! Done waiting!!\n");
            }
        }

        afterLineProcessed(formatted, line);
    }
    fprintf(stdout, "all done: %s\n", line);

    fprintf(stdout, "\n");
    freeUp(line);
    prepareForExit(/*shell,*/ prompt);

    return 0;
}
