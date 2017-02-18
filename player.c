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

int open_master_player_fifo(int id){
  char name[20];
  snprintf(name, sizeof(name), "master_p%d", id);
  char path[30] = "/tmp/";
  strcat(path, name);
  int master_p = open(path, O_RDONLY);
  //add error checking
  return master_p;
}

int open_player_master_fifo(int id){
  char name[20];
  snprintf(name, sizeof(name), "p%d_master", id);
  char path2[30] = "/tmp/";
  strcat(path2, name);
  int p_master = open(path2, O_WRONLY);
  return p_master;
}

int main(int argc, char *argv[]){
  //  exit(0);
  if(argc != 2){
    fprintf(stderr, "player <player_id>\n");
    return EXIT_FAILURE;
  }
  int id = atoi(argv[1]);

  int pp_fifo = 0;
  
  int master_p = open_master_player_fifo(id);
  int p_master = open_player_master_fifo(id);
  POTATO_T * potato;
  POTATO_T p;
  potato = &p;

    
  fd_set rfds;
  FD_ZERO(&rfds);
  FD_SET(master_p, &rfds);
  int retval;
  
  retval = select(master_p+1, &rfds, NULL, NULL, NULL);
  if(retval == -1){
    perror("select()");
  }
  else if(retval){
    if(FD_ISSET(master_p, &rfds)){
      if(read(master_p, potato, sizeof(*potato)) == sizeof(*potato)){
	printf("total_hops = %d\n", potato->total_hops);
	printf("hops_count = %d\n", potato->hop_count);
	printf("id = %d\n", id);
      }
    }	
  }

  
  /*  
  //open player_player pipe
  int a = id-1;
  if(a < 0){
    a = players - 1;
  }
  int p_pl = open_player_player_fifo();
  int pl_p = open_player_player_fifo();
  int p_pr = open_player_player_fifo();
  int pr_p = open_player_player_fifo();
  */

  
  return EXIT_SUCCESS;

}
