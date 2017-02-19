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

int open_leftright_player(int a, int b, int w_r){
  int p_p;
  char path[30];
  snprintf(path, sizeof(path), "/tmp/p%d_p%d", a, b);
  if(w_r == 0){
    if((p_p = open(path, O_WRONLY)) == -1){
      perror("open()");
    }
  }
  else{
    if((p_p = open(path, O_RDONLY)) == -1){
      perror("open()");
    }
  }
  return p_p;
}
  
void open_player_player_fifo(int id, int players, int * p_p){
  int count = 0;
  int a = id;
  int b = id + 1;
  if(b == players){
    b = id - 1;
    p_p[3] = open_leftright_player(b, a, 1);//1 means RD
    p_p[2] = open_leftright_player(a, b, 0);//0 means WR
    b = 0;
    p_p[0] = open_leftright_player(a, b, 0);//0 means WR
    p_p[1] = open_leftright_player(b, a, 1);//1 means RD
  }
  else{
    p_p[0] = open_leftright_player(a, b, 0);//0 means WR
    p_p[1] = open_leftright_player(b, a, 1);//1 means RD
    b = id - 1;
    if(b < 0){
      b = players - 1;
    }
    p_p[3] = open_leftright_player(b, a, 1);//1 means RD
    p_p[2] = open_leftright_player(a, b, 0);//0 means WR
  }
}

void send_init_to_master(int id, int p_master){
  int ready[1];
  ready[0] = id;
  if(write(p_master, ready, sizeof(*ready)) != sizeof(*ready)){
    fprintf(stderr, "write to file error\n");
    return;
  }      
}

int main(int argc, char *argv[]){
  //  exit(0);
  if(argc != 2){
    fprintf(stderr, "player <player_id>\n");
    return EXIT_FAILURE;
  }
  int id = atoi(argv[1]);
  
  //int pp_fifo = 0;
  
  int master_p = open_master_player_fifo(id);
  int p_master = open_player_master_fifo(id);
  int * p_p = malloc(4 * sizeof(*p_p));
  
  
  int players;
  int * playerpointer = &players;
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
      if(read(master_p, playerpointer, sizeof(*playerpointer)) != sizeof(*playerpointer)){
	perror("read()");
      }
      else{
	open_player_player_fifo(id, players, p_p);
	printf("Connected as player %d out of %d total players\n", id, players);
	send_init_to_master(id, p_master);

      }
    }
  }
  /*
  if(id == 0){
  char name[20];
  snprintf(name, sizeof(name), "p%d_p%d", id, id+1);
  char path[30] = "/tmp/";
  strcat(path, name);
  printf("2:%s\n", path);
  if(open(path, O_WRONLY) == -1){
    perror("open()");
  }
  else{
    printf("open%d%d\n", id, id+1);
  }
  }
  if(id == 1){
  char name[20];
  snprintf(name, sizeof(name), "p%d_p%d", id-1, id);
  char path[30] = "/tmp/";
  strcat(path, name);
  printf("2:%s\n", path);
  if(open(path, O_RDONLY) == -1){
    perror("open()");
  }
  else{
    printf("open%d%d\n", id-1, id);
  }
  }
  */
  
  
  
  

  /*    
  POTATO_T * potato;
  POTATO_T p;
  potato = &p;
  
  //fd_set rfds;
  FD_ZERO(&rfds);
  FD_SET(master_p, &rfds);
  
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
  
  */ 
    
  
  return EXIT_SUCCESS;

}
