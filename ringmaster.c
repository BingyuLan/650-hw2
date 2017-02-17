#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include "potato.h"


int main(int argc, char *argv[]){
  //  exit(0);
  //validate command line arguments
  if(argc != 3){
    fprintf(stderr, "ringmaster <number_of_players> <number_of_hops>\n");
    return EXIT_FAILURE;
  }
  int players = atoi(argv[1]);
  int hops = atoi(argv[2]);
  if(players <=1 || hops < 0 || hops > 512){
    fprintf(stderr, "number_of_players > 1, 0 <= number_of_hops <= 512\n");
    return EXIT_FAILURE;
  }

  //create fifo

  for(int i = 0; i < players; i++){
    char name[20];
    snprintf(name, sizeof(name), "master_p%d", i);
    char path[30] = "/tmp/";
    strcat(path, name);
    //make a fifo
    if(mkfifo(path, S_IRUSR | S_IWUSR) != 0){
      perror("mkfifo() error");
    }
    //printf("%s\n", name);

    snprintf(name, sizeof(name), "p%d_master", i);
    char path2[30] = "/tmp/";
    strcat(path2, name);
    if(mkfifo(path2, S_IRUSR | S_IWUSR) != 0){
      perror("mkfifo() error");
    }
    //printf("%s\n", name);
  }

  for(int i = 0; i < players; i++){
    int a = i-1;
    for(int j = 0; j < 2; j++){
      if(a < 0){
	a = players - 1;
      }
      if(a == players){
	a = 0;
      }
      char name[20];
      snprintf(name, sizeof(name), "p%d_p%d", i, a);
      //printf("%s\n", name);
      char path[30] = "/tmp/";
      strcat(path, name);
      if(mkfifo(path, S_IRUSR | S_IWUSR) != 0){
	perror("mkfifo() error");
      }
      a = i+1;
    }
  }

  //open fifo
  for(int i = 0; i < players; i++){
    char name[20];
    snprintf(name, sizeof(name), "master_p%d", i);
    char path[30] = "/tmp/";
    strcat(path, name);
    

  
}

