// Client program: testing - ./a.out 127.0.0.1 9800
#include<stdio.h>
#include<unistd.h>
#include<fcntl.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<stdlib.h>
#include<netinet/in.h>
#include<string.h>
#include<arpa/inet.h>

void receiveMessage(int sockfd, char* buf, size_t size){
  bzero(buf, size);
  read(sockfd, buf, size);
  printf("%s\n", buf);
}



void login(int sockfd){
  char login_buf[100];
  receiveMessage(sockfd, login_buf, 100);

  char c[4];
  scanf("%s", c);
  write(sockfd, c, 3);

  receiveMessage(sockfd, login_buf, 100);

  char username[12], password[12];

  scanf("%s", username);
  write(sockfd, username, strlen(username));

  receiveMessage(sockfd, login_buf, 100);

  scanf("%s", password);
  write(sockfd, password, strlen(password));

  receiveMessage(sockfd, login_buf, 100);

  char type[15];
  scanf("%s", type);
  write(sockfd, type, strlen(type));

  if(!strcmp(type, "Joint")){
    receiveMessage(sockfd, login_buf, 100);
    char keyword[12];
    scanf("%s", keyword);
    write(sockfd, keyword, strlen(keyword));
  }
}



int main(int argc, char* argv[]){


  char* ipaddr = argv[1];
  int portnum = atoi(argv[2]);

  struct sockaddr_in serv;
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);

  serv.sin_family = AF_INET;
  serv.sin_addr.s_addr = inet_addr(ipaddr);
  serv.sin_port = htons(portnum);

  int ret = connect(sockfd, (struct sockaddr*)&serv, sizeof(serv));

  if(ret==-1)
    perror("");

  int login_attempts = 0;
  while(login_attempts<3){
    login(sockfd);
    char buf[100];
    bzero(buf, 100);
    receiveMessage(sockfd, buf, 100);
    if(!strcmp(buf, "No such username")){
      write(sockfd, "ACK", 5);
      login_attempts++;
    }
    else if(!strcmp(buf, "successful")){
      write(sockfd, "ACK", 5);
      break;
    }
    // write(sockfd, "ACK", 5);
  }
  int Exit = 0;

  if(login_attempts>=3){
    char buf[100];
    receiveMessage(sockfd, buf, 100);
    write(sockfd, "ACK", 5);
    Exit = 1;
  }

  while(!Exit){
    char receive_buf[400];
    bzero(receive_buf, 400);
    receiveMessage(sockfd, receive_buf, 400);
    if(!strcmp(receive_buf, "You are exiting")){
      write(sockfd, "ACK", 5);
      Exit = 1;
      break;
    }
    char send_buf[300];
    bzero(send_buf, 300);
    scanf(" %[^\n]", send_buf);
    write(sockfd, send_buf, strlen(send_buf));
  }


  return 0;
}
