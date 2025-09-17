#include <stdio.h>
#include <stdlib.h>

int main() { 
  printf("Type a character: ");
  int character = getchar();
  printf("\nYou typed: %c\n", character);
  printf("Putting the character back and reading again: \n");

  ungetc(character, stdin);


  size_t limit = 1024;
  char *buffer = malloc(limit);

  size_t n = getline(&buffer, &limit, stdin);
  if(n == -1){
    fprintf(stderr, "failed to read character");
    exit(-1);
  }

  printf("You typed %s", buffer);
}
