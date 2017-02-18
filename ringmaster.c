#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include "potato.h"

void create_master_to_players_fifo(int players, char ** pipename1){
  //create fifo

  for(int i = 0; i < players; i++){
    char name[20];
    snprintf(name, sizeof(name), "master_p%d", i);
    char path[30] = "/tmp/";
    strcat(path, name);
    unlink(path);
    pipename1[i] = malloc(30 * sizeof(*pipename1[i]));
    strcpy(pipename1[i], path);
    //make a fifo
    if(mkfifo(path, S_IRUSR | S_IWUSR) != 0){
      perror("mkfifo() error");
    }
    //printf("%s\n", name);

    snprintf(name, sizeof(name), "p%d_master", i);
    char path2[30] = "/tmp/";
    strcat(path2, name);
    unlink(path2);
    if(mkfifo(path2, S_IRUSR | S_IWUSR) != 0){
      perror("mkfifo() error");
    }
    //printf("%s\n", name);
  }

}

void create_players_to_players_fifo(int players){
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
      unlink(path);
      if(mkfifo(path, S_IRUSR | S_IWUSR) != 0){
	perror("mkfifo() error");
      }
      a = i+1;
    }
  }

}


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

  char ** pipename1 = malloc((2*players) * sizeof(*pipename1));
  
  
  create_master_to_players_fifo(players, pipename1);
  create_players_to_players_fifo(players);
  for(int i = 0; i < players; i++){
    printf("%s\n", pipename1[i]);
  }

  //open fifo
  int fdwrite[players];
  int fdread[players];
  for(int i = 0; i < players; i++){
    char name[20];
    snprintf(name, sizeof(name), "master_p%d", i);
    char path[30] = "/tmp/";
    strcat(path, name);
    fdwrite[i] = open(path, O_WRONLY);
    //add error checking
    snprintf(name, sizeof(name), "p%d_master", i);
    char path2[30] = "/tmp/";
    strcat(path2, name);
    fdread[i] = open(path2, O_RDONLY);
  }


  POTATO_T * potato;
  POTATO_T p;
  potato = &p;
  potato->msg_type = 0; //0 means popato
  potato->total_hops = hops;
  potato->hop_count = 0;

  srand((unsigned int) time(NULL));
  int random = rand() % players;
  printf("first_potato_to : %d\n", random);
  
  if(write(fdwrite[random], potato, sizeof(*potato)) != sizeof(*potato)){
    fprintf(stderr, "write to file error\n");
    return EXIT_FAILURE;
  }      
  else{
    printf("pass potato success\n");
  }
  
  
  return EXIT_SUCCESS;
  
}


