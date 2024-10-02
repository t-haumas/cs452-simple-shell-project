#ifndef LAB_H
#define LAB_H
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>

#define lab_VERSION_MAJOR 1
#define lab_VERSION_MINOR 0
#define UNUSED(x) (void)x;

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct job {
        int jobNum;
        pid_t pid;
        char *command;
    } job;

    typedef struct jobNode {
        job info;
        struct jobNode *next;
    } jobNode;

    jobNode *jobList;

    struct shell {
        int shell_is_interactive;
        pid_t shell_pgid;
        struct termios shell_tmodes;
        int shell_terminal;
        char *prompt;
        bool exiting;
    };

    /**
     * @brief prints info about a job to the console in the following format:
     * [n] process-id command
     *
     * @param info the job to print
     */
    void printJob(job info);

    /**
     * @brief prints info about a job to the console in the following format:
     * [n] process-id Running command
     *
     * @param info the job to print
     */
    void printJobRunning(job info);

    /**
     * @brief prints info about a job to the console in the following format:
     * [n] Done command
     *
     * @param doneJob the job to print
     */
    void printDone(job doneJob);

    /**
     * @brief a helper function to free a pointer's allocated memory, and set
     * the pointer to NULL so freeing it again won't cause a problem.
     *
     * @param ptr a pointer to the pointer to free
     */
    void freeUp(void **ptr);

    /**
     * @brief loops through a provided list of jobs, and removes all jobs that
     * are discovered to be finished. If printAny is true, also prints to the
     * console any finished jobs while iterating. If printAll && printAny is
     * true, also prints to the console all jobs that are still running while
     * iterating.
     *
     * @param jobList the linked list of jobs to iterate through
     * @param printAny if finished jobs should be printed to the console
     * @param printAll if running jobs should be printed to the console
     */
    void reportAndManageFinishedJobs(jobNode **jobList, bool printAny, bool printAll);

    /**
     * @brief Set the shell prompt. This function will attempt to load a prompt
     * from the requested environment variable, if the environment variable is
     * not set a default prompt of "shell>" is returned.  This function calls
     * malloc internally and the caller must free the resulting string.
     *
     * @param env The environment variable
     * @return const char* The prompt
     */
    char *get_prompt(const char *env);

    /**
     * Changes the current working directory of the shell. Uses the linux system
     * call chdir. With no arguments the users home directory is used as the
     * directory to change to.
     *
     * @param dir The directory to change to
     * @return  On success, zero is returned.  On error, -1 is returned, and
     * errno is set to indicate the error.
     */
    int change_dir(char **dir);

    /**
     * @brief Convert line read from the user into to format that will work with
     * execvp. We limit the number of arguments to ARG_MAX loaded from sysconf.
     * This function allocates memory that must be reclaimed with the cmd_free
     * function.
     *
     * @param line The line to process
     *
     * @return The line read in a format suitable for exec
     */
    char **cmd_parse(char const *line);

    /**
     * @brief Free the line that was constructed with parse_cmd
     *
     * @param line the line to free
     */
    void cmd_free(char **line);

    /**
     * @brief Trim the whitespace from the start and end of a string.
     * For example "   ls -a   " becomes "ls -a". This function modifies
     * the argument line so that all printable chars are moved to the
     * front of the string
     *
     * @param line The line to trim
     * @return The new line with no whitespace
     */
    char *trim_white(char *line);

    /**
     * @brief Takes an argument list and checks if the first argument is a
     * built in command such as exit, cd, jobs, etc. If the command is a
     * built in command this function will handle the command and then return
     * true. If the first argument is NOT a built in command this function will
     * return false.
     *
     * @param sh The shell
     * @param argv The command to check
     * @return True if the command was a built in command
     */
    bool do_builtin(struct shell *sh, char **argv);

    /**
     * @brief Initialize the shell for use. Allocate all data structures
     * Grab control of the terminal and put the shell in its own
     * process group. NOTE: This function will block until the shell is
     * in its own program group. Attaching a debugger will always cause
     * this function to fail because the debugger maintains control of
     * the subprocess it is debugging.
     *
     * @param sh
     */
    void sh_init(struct shell *sh);

    /**
     * @brief Destroy shell. Free any allocated memory and resources and exit
     * normally.
     *
     * @param sh
     */
    void sh_destroy(struct shell *sh);

    /**
     * @brief Parse command line args from the user when the shell was launched
     *
     * @param argc Number of args
     * @param argv The arg array
     */
    void parse_args(int argc, char **argv);

    /**
     * @brief Returns a string name for this program, for use when printing
     * the version.
     */
    const char *getProgramName();

#ifdef __cplusplus
} // extern "C"
#endif

#endif
