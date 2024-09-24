#include <stdio.h>
#include <unistd.h>
#include "../src/lab.h"
#include <readline/readline.h>
#include <readline/history.h>

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

  char *line;
  using_history();
  while ((line = readline(get_prompt("MY_PROMPT"))))
  {
    // for echoing:
    //printf("%s\n", line);
    
    // process line
    add_history(line);
    free(line);
  }

  return 0;
}
