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
    if (newJobNode == NULL) {
        perror("Error allocating job node.");
        return NULL;
    }

    newJobNode->info = newJob;
    newJobNode->next = NULL;
    return newJobNode;
}

/**
 * @brief frees a linked list of jobs and any memory owned by the job structs.
 *
 * @param jobList a pointer to a jobNode serving as the head of the list.
 */
void freeList(jobNode *jobList)
{
    jobNode *current = jobList;
    jobNode *next;
    while (current != NULL)
    {
        next = current->next;
        freeUp((void **)&current->info.command);
        freeUp((void **)&current);
        current = next;
    }
}

/**
 * @brief adds an entry to the end of a jobNode linked list.
 *
 * @param head a pointer to a pointer to the head of the jobNode linked list.
 * @param newJob a job to add to the end of the linked list.
 * @return true if successful, false if an error occurred.
 */
bool append(jobNode **head, job newJob)
{
    jobNode *previousNode = NULL;
    jobNode *currentNode = *head;

    // Find the last element of the list, store it in previousNode
    while (currentNode != NULL)
    {
        previousNode = currentNode;
        currentNode = currentNode->next;
    }


    if (previousNode == NULL)
    {   // The list was empty, make the new node the head
        *head = createJobNode(newJob);
        return head != NULL;
    }
    else
    {   // Add the new node after the last node
        previousNode->next = createJobNode(newJob);
        return previousNode->next != NULL;
    }
}

/**
 * @brief helper/wrapper function that prepares for program exit by freeing
 * memory owned by the parameters.
 *
 * @param sh the shell struct to call sh_destroy on.
 * @param jobList the linked list of jobs to free.
 */
void prepareForExit(struct shell *sh, jobNode *jobList)
{
    sh_destroy(sh);
    freeList(jobList);
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
    add_history(line);
    freeUp((void **)&line);
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
 * the ampersand character from the argument command string array.
 *
 * @param command The array of strings representing the command.
 * @return true if the command should be run in the background, false if not.
 */
bool getIsBackground(char **command)
{
    // Get the last character of the command
    int cmd_len = getLength(command);
    char *lastWord = command[cmd_len - 1];
    int wordLen = strlen(lastWord);
    if (wordLen < 1)
    {
        return false;
    }
    char lastChar = lastWord[wordLen - 1];

    // If it is '&', remove it from the command string array and return true
    if (lastChar == '&')
    {
        lastWord[wordLen - 1] = '\0';
        // It's not enough to just remove the '&', now remove dangling whitespace and delete that entry if it's empty.
        command[cmd_len - 1] = trim_white(lastWord);
        freeUp((void **)&lastWord);
        if (strlen(command[cmd_len - 1]) == 0) {
            freeUp((void**)&command[cmd_len - 1]);
            command[cmd_len - 1] = NULL;
        }
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

/**
 * @brief a helper function for exiting the program early when something goes wrong.
 * 
 * @param cmd the parsed command to free
 * @param line the line read in from the user to free
 * @param sh the shell struct to destroy
 */
void exitEarly(char** cmd, char* line, struct shell* sh)
{
    reportAndManageFinishedJobs(&jobList, true, false);
    afterLineProcessed(cmd, line);
    prepareForExit(sh, jobList);
    exit(1);
}

int main(int argc, char **argv)
{
    // Initial setup
    jobList = NULL;
    parse_args(argc, argv);
    struct shell sh;

    sh_init(&sh);
    if (sh.exiting) {
        sh_destroy(&sh);
        exit(1);
    }

    char *line;
    using_history();

    // Main execution loop
    while ((line = readline(sh.prompt)))
    {
        // Get input
        char **formatted = cmd_parse(line);
        if (formatted == NULL) {
            exitEarly(formatted, line, &sh);
        }
        if (formatted[0] == NULL)
        {   // They inputted a blank line
            reportAndManageFinishedJobs(&jobList, true, false);
            afterLineProcessed(formatted, line);
            continue;
        }

        // Attempt to do builtin command
        bool was_builtin = do_builtin(&sh, formatted);
        if (was_builtin)
        {
            if (sh.exiting)
            {
                afterLineProcessed(formatted, line);
                prepareForExit(&sh, jobList);
                return 0;
            }
        }
        else
        {   // Command was not builtin
            // Check if command should be run in the background
            bool isForeground = !getIsBackground(formatted);

            if (formatted[0] == NULL)
            {   // They inputted a blank line
                reportAndManageFinishedJobs(&jobList, true, false);
                afterLineProcessed(formatted, line);
                continue;
            }

            // Fork and do command
            pid_t my_id = fork();
            if (my_id == -1)
            {
                // Fork failed
                perror("Error starting new process");
                exitEarly(formatted, line, &sh);
            }
            else if (my_id == 0)
            {
                // Child process
                if (sh.shell_is_interactive)
                {
                    pid_t child = getpid();
                    setUpChildProcessGroupAndForeground(child, &sh, isForeground);
                    signal(SIGINT, SIG_DFL);
                    signal(SIGQUIT, SIG_DFL);
                    signal(SIGTSTP, SIG_DFL);
                    signal(SIGTTIN, SIG_DFL);
                    signal(SIGTTOU, SIG_DFL);
                }
                char *cmdName = formatted[0];
                // Transform yourself into the new process and execute
                execvp(cmdName, formatted);

                perror("An error occured while executing the command");
                cmd_free(formatted);
                freeUp((void **)&line);
                prepareForExit(&sh, jobList);
                exit(1);
            }
            else
            {
                // Parent process
                if (sh.shell_is_interactive)
                {
                    setUpChildProcessGroupAndForeground(my_id, &sh, isForeground);

                    if (isForeground)
                    {
                        waitpid(my_id, NULL, 0); // The child is in the foreground, so wait for it to complete before continuing execution
                        pid_t processGroup = getpgid(getpid());
                        if (processGroup == (pid_t)-1)
                        {
                            perror("Error getting process group of parent process");
                        }
                        int result = tcsetpgrp(sh.shell_terminal, processGroup); // Regain control of the terminal
                        if (result == -1)
                        {
                            perror("Error setting terminal foreground process group");
                        }

                        // Restore the shell's terminal modes in case the child process messed it up.
                        tcsetattr(sh.shell_terminal, TCSADRAIN, &sh.shell_tmodes);
                    }
                    else
                    {
                        // Child is running in the background, so we make a new job entry for it
                        job newJob;
                        newJob.command = strdup(line);
                        newJob.jobNum = getHighestJobNumber(jobList) + 1;
                        newJob.pid = my_id;
                        bool successful = append(&jobList, newJob);
                        if (!successful) {
                            exitEarly(formatted, line, &sh);
                        }
                        printJob(newJob);
                    }
                    reportAndManageFinishedJobs(&jobList, true, false);
                }
                else
                {
                    // Parent is not running interactively.
                    reportAndManageFinishedJobs(&jobList, false, false);
                    waitpid(my_id, NULL, 0);
                }
            }
        }

        afterLineProcessed(formatted, line);
    }

    fprintf(stdout, "\n");
    freeUp((void **)&line);
    prepareForExit(&sh, jobList);

    return 0;
}
