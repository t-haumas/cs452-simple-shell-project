#include <stdio.h>
#include <unistd.h>
#include "../src/lab.h"

int main(int argc, char** argv)
{
  printf("hello world\n");
  
  int c;
  while ((c = getopt(argc, argv, "v")) != -1) {
    switch (c) {
      case 'v':
        fprintf(stdout, "You said you want the version\n");
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


  return 0;
}
