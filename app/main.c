#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <readline/readline.h>
#include <readline/history.h>

#include "../src/lab.h"

/**
 * @brief creates a jobNode from a job in order to add it to a jobNode linked list.
 * 
 * @param newJob the job to create a jobNode from
 * @return a jobNode containing the info from newJob, ready to be added to a
 * jobNode linked list.
 */
jobNode *createJobNode(job newJob)
{
    jobNode *newJobNode = (jobNode *)malloc(sizeof(jobNode));
    newJobNode->info = newJob;
    newJobNode->next = NULL;
    return newJobNode;
}

/**
 * @brief adds an entry to the end of a jobNode linked list.
 * 
 * @param head a pointer to a pointer to the head of the jobNode linked list.
 * @param newJob a job to add to the end of the linked list.
 */
void append(jobNode **head, job newJob)
{
    // printf("\n\n");
    // printJobList(*head);
    // printf("appending...\n");
    jobNode *previousNode = NULL;
    jobNode *currentNode = *head;
    while (currentNode != NULL)
    {
        previousNode = currentNode;
        currentNode = currentNode->next;
    }
    if (previousNode == NULL)
    {
        // printf("making new head\n");
        *head = createJobNode(newJob);
    }
    else
    {
        previousNode->next = createJobNode(newJob);
    }
    // printJobList(*head);
}

/**
 * @brief helper/wrapper function that prepares for program exit. Right now,
 * it just calls sh_destroy.
 * 
 * @param sh the shell struct to call sh_destroy on.
 */
void prepareForExit(struct shell* sh)
{
    sh_destroy(sh);
}

/**
 * @brief helper function to free the provided pointers, and save the user's
 * input to the history.
 * 
 * @param strArray the array of strings created with cmd_parse that represents
 * the command the user is running.
 * @param line the line returned by readline, which is the raw string the user
 * typed in.
 */
void afterLineProcessed(char **strArray, char *line)
{
    cmd_free(strArray);
    add_history(line); // todo: should we add the whole line or just t5he trimmed?
    freeUp(line);
}

/**
 * @brief Sets the specified process's process group to its own group, and
 * if isForeground is true, grabs control of the console.
 * 
 * @param id the process ID.
 * @param sh the shell struct containing info on the file descriptor associated
 * with the terminal to take control of.
 * @param isForeground whether or not the process with pid id should run in the
 * foreground and take control of the console.
 */
void setUpChildProcessGroupAndForeground(pid_t id, struct shell *sh, bool isForeground)
{
    setpgid(id, id);
    if (isForeground)
        tcsetpgrp(sh->shell_terminal, id);
}

/**
 * @brief returns the number of tokens in a command.
 * 
 * @param command an array of strings representing a command to the shell.
 * @return the length of the command array.
 */
int getLength(char **command)
{
    int len = 0;
    while (command[len] != NULL)
    {
        len++;
    }
    return len;
}

/**
 * @brief checks if the command's last character is an ampersand ('&'),
 * and should be run in the background. If true, this function also deletes
 * the ampersand character by replacing it with the null character ('\0').
 * 
 * @param command The array of strings representing the command.
 * @return true if the command should be run in the background, false if not.
 */
bool getIsBackground(char **command)
{
    int cmd_len = getLength(command);

    char *lastWord = command[cmd_len - 1];
    int wordLen = strlen(lastWord);
    if (wordLen < 1)
    {
        return false;
    }
    char lastChar = lastWord[wordLen - 1];
    if (lastChar == '&')
    {
        lastWord[wordLen - 1] = '\0';
        return true;
    }
    return false;
}

/**
 * @brief returns the highest job number of all jobs in the given jobList.
 * Note: this does not care if the jobs in the list are running or done.
 * It returns the max job number of all of them.
 * 
 * @param jobList the linked list of jobs to check.
 * @return the highest job number of any job in the list.
 */
int getHighestJobNumber(jobNode *jobList)
{
    int highestNumber = 0;
    jobNode *currentNode = jobList;
    while (currentNode != NULL)
    {
        int currentJobNumber = currentNode->info.jobNum;
        if (currentJobNumber > highestNumber)
        {
            highestNumber = currentJobNumber;
        }
        currentNode = currentNode->next;
    }

    return highestNumber;
}

int main(int argc, char **argv)
{
    exitAfterPrintingVersion = false;
    jobList = NULL;

    parse_args(argc, argv);

    if (exitAfterPrintingVersion) {
        return 0;
    }

    struct shell sh;
    sh_init(&sh);

    char *line;
    using_history();

    // Main execution loop
    while ((line = readline(sh.prompt)))
    {
        char **formatted = cmd_parse(line);
        if (formatted[0] == NULL)
        {
            reportAndManageFinishedJobs(&jobList, true, false);
            afterLineProcessed(formatted, line);
            continue;
        }
        bool was_builtin = do_builtin(&sh, formatted);
        if (was_builtin)
        {

            if (sh.exiting)
            {
                afterLineProcessed(formatted, line);
                prepareForExit(&sh);
                return 0;
            }
        }
        else
        {
            // Get is background
            bool isForeground = !getIsBackground(formatted);

            // Fork and do command
            pid_t my_id = fork();
            if (my_id == -1)
            {
                // Fork failed
                reportAndManageFinishedJobs(&jobList, true, false);
                fprintf(stderr, "Failed to start a new process.\n");
                exit(1);
            }
            else if (my_id == 0)
            {
                // Child process
                // printf("Child, my parent is %d.\n", getppid()); //todo: remove this.
                if (sh.shell_is_interactive)
                {
                    // fprintf(stdout, "interactive!\n");
                    pid_t child = getpid();
                    setUpChildProcessGroupAndForeground(child, &sh, isForeground);
                    signal(SIGINT, SIG_DFL);
                    signal(SIGQUIT, SIG_DFL);
                    signal(SIGTSTP, SIG_DFL);
                    signal(SIGTTIN, SIG_DFL);
                    signal(SIGTTOU, SIG_DFL);
                }
                char *cmdName = formatted[0];
                execvp(cmdName, formatted);

                fprintf(stderr, "An error occured during the child process.\n");
                cmd_free(formatted);
                freeUp(line);
                prepareForExit(&sh);
                exit(1); // todo: should print that it wasn't a valid command?
            }
            else
            {
                // Parent process
                if (sh.shell_is_interactive)
                {
                    setUpChildProcessGroupAndForeground(my_id, &sh, isForeground);

                    if (isForeground)
                    {

                        waitpid(my_id, NULL, 0);
                        tcsetpgrp(sh.shell_terminal, getpgid(getpid())); // todo: probably split these functions and do error checking.
                    }
                    else
                    {
                        job newJob;
                        newJob.command = strdup(line);
                        newJob.jobNum = getHighestJobNumber(jobList) + 1;
                        newJob.pid = my_id;
                        append(&jobList, newJob);
                        printJob(newJob);
                    }
                    reportAndManageFinishedJobs(&jobList, true, false);

                    /* Restore the shell’s terminal modes.  */
                    // tcgetattr(shell_terminal, &j->tmodes); // todo: is this necessary?
                    // tcsetattr(shell_terminal, TCSADRAIN, &shell_tmodes);
                }
                else
                {
                    reportAndManageFinishedJobs(&jobList, false, false);
                    // todo: need to set child process group? Example seems like you don't need to.
                    printf("strange block\n");
                    waitpid(my_id, NULL, 0); //todo: maybe delete this else block and the if check if we find out that the parent is always interactive.
                }
            }
        }

        afterLineProcessed(formatted, line);
    }
    // todo: dealloc jobs list [if need to free before exiting], same for if they type exit.

    fprintf(stdout, "\n");
    freeUp(line);
    prepareForExit(&sh);

    return 0;
}
