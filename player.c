//ECE650 hw2 Bingyu Lan

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
  char path[30];
  snprintf(path, sizeof(path), "/tmp/master_p%d", id);
  int master_p;
  if((master_p = open(path, O_RDONLY)) == -1){
    perror("open()");
    exit(EXIT_FAILURE);
  }
  //add error checking
  return master_p;
}

int open_player_master_fifo(int id){
  char path[30];
  snprintf(path, sizeof(path), "/tmp/p%d_master", id);
  int p_master;
  if((p_master = open(path, O_WRONLY)) == -1){
    perror("open()");
    exit(EXIT_FAILURE);
  }
  return p_master;
}

int open_leftright_player(int a, int b, int w_r){
  int p_p;
  char path[30];
  snprintf(path, sizeof(path), "/tmp/p%d_p%d", a, b);
  if(w_r == 0){
    if((p_p = open(path, O_WRONLY)) == -1){
      perror("open()");
      exit(EXIT_FAILURE);
    }
  }
  else{
    if((p_p = open(path, O_RDONLY)) == -1){
      perror("open()");
      exit(EXIT_FAILURE);
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
  }      
  return;
}

void send_end_to_master(POTATO_T * potato, int p_master){
  if(write(p_master, potato, sizeof(*potato)) != sizeof(*potato)){
    fprintf(stderr, "write to file error\n");
  }      
}

int maxfd(int a, int b){
  if(a > b){
    return a;
  }
  else{
    return b;
  }
}

int wait_for_start(int id, int master_p, int p_master, int * p_p){
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
  return players;
}

void send_potato_to_next(POTATO_T * potato, int * p_p, int id, int players){
  //  srand((unsigned int) time(NULL));
  int random = rand() % 2;
  int sendto;
  int nextid;
  if(random){
    sendto = p_p[0];
    if(id == players-1){
      nextid = 0;
    }
    else{
      nextid = id + 1;
    }
  }
  else{
    sendto = p_p[2];
    if(id == 0){
      nextid = players - 1;
    }
    else{
      nextid = id - 1;
    }
  }
  if(write(sendto, potato, sizeof(*potato)) != sizeof(*potato)){
    fprintf(stderr, "write to file error\n");
  }
  printf("Sending potato to %d\n", nextid);
  return;
}

void pass_potato(int id, int master_p, int p_master, int * p_p, int players){
  fd_set rfds;
  int retval;
  int rdfifo[3] = {master_p, p_p[1], p_p[3]};
  int max = maxfd(p_p[1], p_p[3]);
  POTATO_T * potato;
  POTATO_T p;
  potato = &p;
  int pass = 1;
  do{
    FD_ZERO(&rfds);
    FD_SET(master_p, &rfds);
    FD_SET(p_p[1], &rfds);
    FD_SET(p_p[3], &rfds);
    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    retval = select(max+1, &rfds, NULL, NULL, &tv);
    if(retval == -1){
      perror("select()");
    }
    else if(retval){
      for(int i = 0; i < 3; i++){
	if(FD_ISSET(rdfifo[i], &rfds)){
	  if(read(rdfifo[i], potato, sizeof(*potato)) == sizeof(*potato)){
	    if(potato->msg_type){
	      pass = 0;
	      break;
	    }
	    else{
	      potato->hop_trace[potato->hop_count] = id;
	      potato->hop_count++;
	      if(potato->hop_count == potato->total_hops){
		printf("I'm it\n");
		send_end_to_master(potato, p_master);
	      }
	      else{
		send_potato_to_next(potato, p_p, id, players);
		break;
	      }
	    }
	  }
	}	
      }
    }
    else{
      pass = 0;
    }
  }while(pass);
  return;
  //  printf("%d stop\n", id);
}

void close_fifo(int master_p, int p_master, int * p_p){
  close(master_p);
  close(p_master);
  for(int i = 0; i < 3; i++){
    close(p_p[i]);
  }
  return;
}

int main(int argc, char *argv[]){
  //  exit(0);

  if(argc != 2){
    fprintf(stderr, "player <player_id>\n");
    return EXIT_FAILURE;
  }
  int id = atoi(argv[1]);

  //open fifos and store fds
  int master_p = open_master_player_fifo(id);
  int p_master = open_player_master_fifo(id);
  int * p_p = malloc(4 * sizeof(*p_p));

  //wait for the start signal and send ready signal to master
  int players = wait_for_start(id, master_p, p_master, p_p);

  srand((unsigned int) time(NULL)+10*id);
  //wait for potato and pass it to next
  pass_potato(id, master_p, p_master, p_p, players);
  // close_fifo(master_p, p_master, p_p);
  free(p_p);
  
  return EXIT_SUCCESS;

}
