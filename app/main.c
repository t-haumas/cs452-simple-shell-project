#include <stdio.h>
#include <unistd.h>  

int main(int argc, char** argv)
{
  printf("hello world\n");
  
  int c;
  while ((c = getopt(argc, argv, "abc:")) != -1) {
    switch (c) {
      case 'a':
        fprintf(stdout, "You said a\n");
        break;
      case 'b':
        fprintf(stdout, "You said b.\n");
        break;
      case 'c':
        fprintf(stdout, "You said c\n");
        break;
      default:
        printf("Default\n");
        break;
    }
  }


  return 0;
}
