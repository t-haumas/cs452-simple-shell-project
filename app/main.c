#include <readline/readline.h>
#include <readline/history.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "../src/lab.h"


void freeUp(char* strX) {
  free(strX);
  strX = NULL;
} // meaningless comment

void prepareForExit(/*shell, */ char* prompt) {
    // Free shell object.
  freeUp(prompt);
}

void afterLineProcessed(char** strArray, char* line) {
  cmd_free(strArray);
  add_history(line);
  freeUp(line);
}

int main(int argc, char** argv)
{
  //printf("hello world\n");
  
  int c;
  while ((c = getopt(argc, argv, "v")) != -1) {
    switch (c) {
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

  char* line;
  char* prompt = get_prompt("MY_PROMPT");
  using_history();
  
  struct shell sh;
  sh_init(&sh);
  while ((line = readline(prompt)))
  {

    char** formatted = cmd_parse(line);
    bool was_builtin = do_builtin(&sh, formatted);
    if (sh.exiting) {
        afterLineProcessed(formatted, line);
        prepareForExit(/*shell, */ prompt);
        return 0;
    }

    afterLineProcessed(formatted, line);
  }
  fprintf(stdout, "\n");
  freeUp(line);
  prepareForExit(/*shell,*/ prompt);

  return 0;
}
