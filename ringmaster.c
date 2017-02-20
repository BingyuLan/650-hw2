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

void create_master_to_players_fifo(int players, char ** master_pn, char ** pn_master){
  //create fifo
  for(int i = 0; i < players; i++){
    char path[30];
    snprintf(path, sizeof(path), "/tmp/master_p%d", i);
    master_pn[i] = malloc(30 * sizeof(*master_pn[i]));
    strcpy(master_pn[i], path);
    //make a fifo
    if(mkfifo(path, S_IRUSR | S_IWUSR) != 0){
      perror("mkfifo() error");
    }
    //printf("%s\n", name);
  }
  for(int i = 0; i < players; i++){
    char path[30];
    snprintf(path, sizeof(path), "/tmp/p%d_master", i);
    pn_master[i] = malloc(30 * sizeof(*pn_master[i]));
    strcpy(pn_master[i], path);
    if(mkfifo(path, S_IRUSR | S_IWUSR) != 0){
      perror("mkfifo() error");
    }
    //printf("%s\n", name);
  }
}

void create_players_to_players_fifo(int players, char ** pn_pn){
  int count = 0;
  for(int i = 0; i < players; i++){
    int a = i-1;
    for(int j = 0; j < 2; j++){
      if(j == 1 && players == 2){
	break;
      }
      if(a < 0){
	a = players - 1;
      }
      if(a == players){
	a = 0;
      }
      char path[30];
      snprintf(path, sizeof(path), "/tmp/p%d_p%d", i, a); 
      pn_pn[count] = malloc(30 * sizeof(*pn_pn[count]));
      strcpy(pn_pn[count], path);
      count++;
      if(mkfifo(path, S_IRWXG | S_IRWXU | S_IRWXO) != 0){
	perror("mkfifo() error");
      }
      a = i+1;
    }
  }
  return;
}

void wait_for_end(int * fdread, int players){
  POTATO_T * potato;
  POTATO_T p;
  potato = &p;
  int loop = 1;
  int retval;
  do{
    fd_set rfds;
    FD_ZERO(&rfds);
    for(int i = 0; i < players; i++){
      FD_SET(fdread[i], &rfds);
    }
    retval = select(fdread[players-1]+1, &rfds, NULL, NULL, NULL);
    if(retval == -1){
      perror("select()");
    }
    else if(retval){
      for(int i = 0; i < players; i++){
	if(FD_ISSET(fdread[i], &rfds)){
	  if(read(fdread[i], potato, sizeof(*potato)) == sizeof(*potato)){
	    printf("Trace of potato:\n");
	    for(int i = 0; i < potato->total_hops-1; i++){
	      printf("%lu,", potato->hop_trace[i]);
	    }
	    printf("%lu\n", potato->hop_trace[potato->total_hops-1]);
	    loop = 0;
	  }
	}
      }
    }
  }while(loop);  
  return;
}

void send_end_signal(int * fdwrite, int players){
  POTATO_T * potato;
  POTATO_T p;
  potato = &p;
  potato->msg_type = 1; //1 means end of game
  for(int i = 0; i < players; i++){
    if(write(fdwrite[i], potato, sizeof(*potato)) != sizeof(*potato)){
      fprintf(stderr, "write to file error\n");
    }      
  }
  return;
}

void wait_for_ready_signal(int players, int * fdread){
  int ready[1];
  int retval;
  int count = 0;
  do{
    fd_set rfds;
    FD_ZERO(&rfds);
    for(int i = 0; i < players; i++){
      FD_SET(fdread[i], &rfds);
    }
    //  int retval;
    retval = select(fdread[players-1]+1, &rfds, NULL, NULL, NULL);
    if(retval == -1){
      perror("select()");
    }
    else if(retval){
      for(int i = 0; i < players; i++){
	if(FD_ISSET(fdread[i], &rfds)){
	  if(read(fdread[i], ready, sizeof(*ready)) == sizeof(*ready)){
	    printf("Player %d is ready to play\n", ready[0]);
	    count++;
	  }
	}
      }
    }
    if(count == players){
      break;
    }
  }while(retval);
  return;

}

void send_first_potato(int players, int * fdwrite, int hops){
  POTATO_T * potato;
  POTATO_T p;
  potato = &p;
  potato->msg_type = 0; //0 means popato
  potato->total_hops = hops;
  potato->hop_count = 0;

  srand((unsigned int) time(NULL));
  int random = rand() % players;
  printf("All players present, sending potato to player %d\n", random);
  
  if(write(fdwrite[random], potato, sizeof(*potato)) != sizeof(*potato)){
    fprintf(stderr, "write to file error\n");
  }      
  return;
}

int main(int argc, char *argv[]){
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
  printf("Potato Ringmaster\nPlayers = %d\nHops = %d\n", players, hops);

  //create fifos
  char ** master_pn = malloc((players) * sizeof(*master_pn));
  char ** pn_master = malloc((players) * sizeof(*pn_master));
  char ** pn_pn = malloc((2 * players) * sizeof(*pn_pn));  
  create_master_to_players_fifo(players, master_pn, pn_master);
  create_players_to_players_fifo(players, pn_pn);

  //open fifos
  int fdwrite[players];
  int fdread[players];
  
  for(int i = 0; i < players; i++){
    fdwrite[i] = open(master_pn[i], O_WRONLY);
    fdread[i] = open(pn_master[i], O_RDONLY);
    //add error checking
  }

  //send init msg to players
  int * playerpointer = & players;
  for(int i = 0; i < players; i++){
    if(write(fdwrite[i], playerpointer, sizeof(*playerpointer)) != sizeof(*playerpointer)){
      fprintf(stderr, "write to file error\n");
      return EXIT_FAILURE;
    }      
  }
  
  //wait for players to be ready
  wait_for_ready_signal(players, fdread);

  if(hops){
    //send out potato
    send_first_potato(players, fdwrite, hops);
  
    //wait for end
    wait_for_end(fdread, players);
  }

  //send end signal
  send_end_signal(fdwrite, players);

  //delete fifo
  for(int i = 0; i < players; i++){
    unlink(master_pn[i]);
    unlink(pn_master[i]);
  }
  for(int i = 0; i < 2*players; i++){
    unlink(pn_pn[i]);
  }
  
  
  return EXIT_SUCCESS;
  
}


