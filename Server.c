// compile : make
// Server program : testing - ./Server 9800
#include"declarations.h"
#include<stdio.h>
#include<unistd.h>
#include<fcntl.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<stdlib.h>
#include<netinet/in.h>
#include<string.h>
#include<pthread.h>
#include<sys/ipc.h>
#include<sys/shm.h>
#include<sys/sem.h>

extern char** userIdTable[3];

int main(int argc, char* argv[]){

  // Setting up the server
  int portnum = atoi(argv[1]);
  struct sockaddr_in serv, cli;

  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if(sockfd<0){
    perror("");
  }

  serv.sin_family = AF_INET;
  serv.sin_addr.s_addr = INADDR_ANY;
  serv.sin_port = htons(portnum);

  int ret = bind(sockfd, (struct sockaddr*)&serv, sizeof(serv));
  if(ret<0){
    perror("");
  }
  ret = listen(sockfd, MAX_USERS);
  if(ret<0){
    perror("");
  }

  printf("Server is ready to listen to client requests. To close the server press CTRL+C\n");

  pthread_t tid;



  // instantiating semaphores
  union semun semarg0, semarg1, semarg2, semarg3, semarg4;
  int key = ftok(".", 'a');
  int semid = semget(key, 4, IPC_CREAT|0744);
  semarg0.val = 1;//binary
  semarg1.val = 1; // Normal ops
  semarg2.val = 1; // Joint ops
  semarg3.val = 1; // Admin ops
  semarg4.val = 1; // exclusively for modifications
  semctl(semid, 0, SETVAL, semarg0); // 0th is binary
  semctl(semid, 1, SETVAL, semarg1); // 1st is binary
  semctl(semid, 2, SETVAL, semarg2); // 2nd is binary
  semctl(semid, 3, SETVAL, semarg3); // 3rd is binary
  semctl(semid, 4, SETVAL, semarg4); // 4th is for modification ops


  // initializing tables
  // index is the id for userIdTable
  for (int idx=0; idx<3; idx++){
    userIdTable[idx] = (char**)malloc(MAX_USERS*sizeof(char*));
    // for(int i=0; i<MAX_USERS; i++)
    //   userIdTable[idx][i] = (char*)malloc(MAX_CHAR_LEN*sizeof(char));
  }

  // Create Databases
  int Norm_database_fd = open("Normal_account.txt", O_CREAT, 0744);
  int Joint_database_fd = open("Joint_account.txt", O_CREAT, 0744);
  int Admin_database_fd = open("Admin_account.txt", O_CREAT, 0744);


  // Adding Bank owner to admin Database
  char owner_username[MAX_CHAR_LEN] = "Bankowner";
  char owner_password[MAX_CHAR_LEN] = "b@nkowner";
  addAdmin(owner_username, owner_password);
  close(Norm_database_fd);
  close(Joint_database_fd);
  close(Admin_database_fd);


  while(1){
    socklen_t clilen = sizeof(cli);
    int clifd = accept(sockfd, (struct sockaddr*)&cli, &clilen);

    // concurrent server
    if(clifd<0){
      perror("");
    }

    pthread_create(&tid, NULL, &server_to_client, (void*)&clifd);
  }

  close(sockfd);
  return 0;
}
