
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

int getIdNorm(char* username){
  int id = -1; // no username found

  int key = ftok(".", 'a');
  int semid = semget(key, 0, 0);
  struct sembuf semlock = {1, -1, SEM_UNDO}; // 1st semaphore
  semop(semid, &semlock, 1);

  for(int i=0; i<MAX_USERS; i++){
    if(userIdTable[NORMAL_USER][i]==NULL)
      continue;
    if(!strcmp(userIdTable[NORMAL_USER][i], username)){
      id = i;
      break;
    }
  }

  semlock.sem_op = 1;
  semop(semid, &semlock, 1); // unlocking the semaphore

  return id;
}

int getIdJoint(char* keyword){
  int id = -1;

  int key = ftok(".", 'a');
  int semid = semget(key, 0, 0);
  struct sembuf semlock = {2, -1, SEM_UNDO}; // 2nd semaphore
  semop(semid, &semlock, 1);

  for(int i=0; i<MAX_USERS; i++){
    if(userIdTable[JOINT_USER][i]==NULL)
      continue;
    if(!strcmp(userIdTable[JOINT_USER][i], keyword)){
      id = i;
      break;
    }
  }

  semlock.sem_op = 1;
  semop(semid, &semlock, 1); // unlocking the semaphore

  return id;
}

int getIdAdmin(char* username){
  int id = -1;

  int key = ftok(".", 'a');
  int semid = semget(key, 0, 0);
  struct sembuf semlock = {3, -1, SEM_UNDO}; // 3rd semaphore
  semop(semid, &semlock, 1);

  for(int i=0; i<MAX_USERS; i++){
    if(userIdTable[ADMIN_USER][i]==NULL)
      continue;
    if(!strcmp(userIdTable[ADMIN_USER][i], username)){
      id = i;
      break;
    }
  }

  semlock.sem_op = 1;
  semop(semid, &semlock, 1); // unlocking the semaphore

  return id;
}




// Login Prompt
// returns 1 for registered user and 0 for not registered
int loginPrompt(int client_fd, char* username, char* password, char* keyword, int* type){
  bzero(username, MAX_CHAR_LEN);
  bzero(password, MAX_CHAR_LEN);

  write(client_fd, "Are you registered user?[Y/N]: ", strlen("Are you registered user?[Y/N]: "));
  char c[10];
  read(client_fd, c, 9);
  if(!strcmp(c, "N")){
    return 0;
  }

  write(client_fd, "Enter Username: ", strlen("Enter Username: "));
  read(client_fd, username, MAX_CHAR_LEN-1);
  write(client_fd, "Enter Password: ", strlen("Enter Password: "));
  read(client_fd, password, MAX_CHAR_LEN-1);
  write(client_fd, "Enter Account Access Type [Norm/Joint/Admin]: ", strlen("Enter Account Access Type [Norm/Joint/Admin]: "));
  char type_str[6];
  read(client_fd, type_str, 5);
  if(!strcmp(type_str, "Norm")){
    *type = NORMAL_USER;
  }
  else if(!strcmp(type_str, "Joint")){
    *type = JOINT_USER;

    write(client_fd, "Enter the unique keyword", strlen("Enter the unique keyword"));
    bzero(keyword, MAX_CHAR_LEN);
    read(client_fd, keyword, MAX_CHAR_LEN-1);
  }
  else if(!strcmp(type_str, "Admin")){
    *type = ADMIN_USER;
  }

  return 1;
}

// registration prompt
void registerPrompt(int client_fd, char* username, char* password){
  // todo : ask admin to create new account
  printf("incomplete\n");
}

void presentMenu(int client_fd, int type, char* rec){

  if(type==NORMAL_USER || type==JOINT_USER){
    char menu[150] = "Enter the number alloted for the operation you wish to perform\n1. Deposit\n2. Withdraw\n3. Balance enquiry\n4. Password Change\n5. View details\n50. Exit\n";

    write(client_fd, menu, 200);
    read(client_fd, rec, 10);
  }

  else{
    char menu[350] = "Enter the number alloted for the operation you wish to perform\n1. Add normal user\n2. Add joint user\n3. Add admin\n4. Delete Normal\n5. Delete Joint\n6. Delete admin\n7. Search normal\n8. Search joint\n9. Search admin\n10. Modify normal\n11. Modify joint\n12. Modify admin\n50. Exit\n";

    write(client_fd, menu, 350);
    read(client_fd, rec, 10);
  }
}

// thread function
void *server_to_client(void* arg){

  int client_fd = *((int*)arg); // client file descriptor

  char username[MAX_CHAR_LEN], password[MAX_CHAR_LEN], keyword[MAX_CHAR_LEN];
  int accountType, userId;

  int success_login = 0, trials = 0;
  while(!success_login && trials<3){
    int registered = loginPrompt(client_fd, username, password, keyword, &accountType); // no synchronization required


    if(!registered){
      registerPrompt(client_fd, username, password);
    }

    // get id of the user

    if(accountType==NORMAL_USER){
      userId = getIdNorm(username);
    }
    else if(accountType==JOINT_USER){
      userId = getIdJoint(keyword);
    }
    else if(accountType==ADMIN_USER){
      userId = getIdAdmin(username);
    }

    // authenticate user password
    if(userId!=-1){
      if(accountType==NORMAL_USER){
        struct Data dt = searchNorm(userId);
        if(strcmp(dt.user.password, password)) // not equal
          userId = -1;
      }
      else if(accountType==JOINT_USER){
        struct JointData jdt = searchJoint(userId);
        if(!strcmp(jdt.user1.username, username)){
          if(strcmp(jdt.user1.password, password))
            userId = -1;
        }
        else if(!strcmp(jdt.user2.username, username)){
          if(strcmp(jdt.user2.password, password))
            userId = -1;
        }
      }
      else if(accountType==ADMIN_USER){
        struct Data dt = searchAdmin(userId);
        if(strcmp(dt.user.password, password))
          userId = -1;
      }
    }


    if(userId==-1){
      write(client_fd, "No such username", strlen("No such username"));
      char ack[11];
      read(client_fd, ack, 10);
      trials++;
    }
    else{
      write(client_fd, "successful", strlen("successful"));
      char ack[11];
      read(client_fd, ack, 10);
      success_login=1;
    }

  }

  int Exit = 0; // bool to exit menu
  if(trials>=3){
    write(client_fd, "exhausted all attempts, closing connection", strlen("exhausted all attempts, closing connection"));
    char ack[11];
    read(client_fd, ack, 10);
    Exit = 1;
  }

  while(!Exit){
    char rec[12];
    bzero(rec, 12);
    presentMenu(client_fd, accountType, rec);
    if(!strcmp(rec, "50")){
      Exit = 1;
      write(client_fd, "You are exiting", strlen("You are exiting"));
      char ack[11];
      read(client_fd, ack, 10);
      break;
    }
    if(accountType==NORMAL_USER){
      if(!strcmp(rec, "1")){
        long int amt;
        char amtstr[10];bzero(amtstr, 10);
        write(client_fd, "Enter Amt to be deposited?", strlen("Enter amt to be deposited?"));
        read(client_fd, amtstr, 9);
        amt = atol(amtstr);
        depositAmtNorm(userId, amt);
        write(client_fd, "Amount Deposited\nEnter ACK to proceed", strlen("Amount Deposited\nEnter ACK to proceed"));
        char ack[11];
        read(client_fd, ack, 10);
      }
      else if(!strcmp(rec, "2")){
        long int amt;
        char amtstr[10];bzero(amtstr, 10);
        write(client_fd, "Enter Amt to withdraw?", strlen("Enter amt to withdraw?"));
        read(client_fd, amtstr, 9);
        amt = atol(amtstr);
        int ret = withdrawAmtNorm(userId, amt);
        if(ret==-1){
          write(client_fd, "Invalid op. The amount to be withdrawn is more than available\nEnter ACK to proceed", strlen("Invalid op. The amount to be withdrawn is more than available\nEnter ACK to proceed"));
        }
        else{
          write(client_fd, "Amount withdrawn\nEnter ACK to proceed", strlen("Amount withdrawn\nEnter ACK to proceed"));
        }
        char ack[11];
        read(client_fd, ack, 10);
      }
      else if(!strcmp(rec, "3")){
        long int amt = balanceEnquiry(userId, NORMAL_USER);
        char amtstr[50];bzero(amtstr, 50);
        sprintf(amtstr, "Balance Amount: %ld\nEnter ACK to proceed\n", amt);
        write(client_fd, amtstr, strlen(amtstr));
        char ack[11];
        read(client_fd, ack, 10);
      }
      else if(!strcmp(rec, "4")){
        write(client_fd, "Enter new password", strlen("Enter new password"));
        read(client_fd, password, MAX_CHAR_LEN-1);
        passwordChangeNorm(userId, password);
        write(client_fd, "Password changed\nEnter ACK to proceed", strlen("Password changed\nEnter ACK to proceed"));
        char ack[11];
        read(client_fd, ack, 10);
      }
      else if(!strcmp(rec, "5")){
        struct Data dt = viewDetailsNorm(userId);
        char details[120];
        sprintf(details, "User id:%d\nUsername:%s\nBalance amt:%ld\nEnter ACK to proceed\n", dt.id, dt.user.username, dt.balance);
        write(client_fd, details, strlen(details));
        char ack[11];
        read(client_fd, ack, 10);
      }
    }
    else if(accountType==JOINT_USER){
      if(!strcmp(rec, "1")){
        long int amt;
        char amtstr[10];bzero(amtstr, 10);
        write(client_fd, "Enter Amt to be deposited?", strlen("Enter amt to be deposited?"));
        read(client_fd, amtstr, 9);
        amt = atol(amtstr);
        depositAmtJoint(userId, amt);
        write(client_fd, "Amount Deposited\nEnter ACK to proceed", strlen("Amount Deposited\nEnter ACK to proceed"));
        char ack[11];
        read(client_fd, ack, 10);
      }
      else if(!strcmp(rec, "2")){
        long int amt;
        char amtstr[10];bzero(amtstr, 10);
        write(client_fd, "Enter Amt to withdraw?", strlen("Enter amt to withdraw?"));
        read(client_fd, amtstr, 9);
        amt = atol(amtstr);
        int ret = withdrawAmtJoint(userId, amt);
        if(ret==-1){
          write(client_fd, "Invalid op. The amount to be withdrawn is more than available\nEnter ACK to proceed", strlen("Invalid op. The amount to be withdrawn is more than available\nEnter ACK to proceed"));
        }
        else{
          write(client_fd, "Amount withdrawn\nEnter ACK to proceed", strlen("Amount withdrawn\nEnter ACK to proceed"));
        }
        char ack[11];
        read(client_fd, ack, 10);
      }
      else if(!strcmp(rec, "3")){
        long int amt = balanceEnquiry(userId, JOINT_USER);
        char amtstr[50];
        sprintf(amtstr, "Balance amount: %ld\nEnter ACK to proceed\n", amt);
        write(client_fd, amtstr, strlen(amtstr));
        char ack[11];
        read(client_fd, ack, 10);
      }
      else if(!strcmp(rec, "4")){
        write(client_fd, "Enter new password", strlen("Enter new password"));
        bzero(password, MAX_CHAR_LEN); // replacing old password
        read(client_fd, password, MAX_CHAR_LEN-1);
        passwordChangeJoint(userId, password, username);
        write(client_fd, "Password changed\nEnter ACK to proceed", strlen("Password changed\nEnter ACK to proceed"));
        char ack[11];
        read(client_fd, ack, 10);
      }
      else if(!strcmp(rec, "5")){
        struct JointData jdt = viewDetailsJoint(userId);
        char details[200];
        sprintf(details, "Joint User id:%d\nUsername1:%s, Username2:%s\nBalance amt:%ld\nEnter ACK to proceed\n", jdt.id, jdt.user1.username, jdt.user2.username, jdt.balance);
        write(client_fd, details, strlen(details));
        char ack[11];
        read(client_fd, ack, 10);
      }
    }
    else if(accountType==ADMIN_USER){
      if(!strcmp(rec, "1")){
        char new_username[MAX_CHAR_LEN], new_password[MAX_CHAR_LEN];
        bzero(new_username, MAX_CHAR_LEN);
        bzero(new_password, MAX_CHAR_LEN);
        write(client_fd, "Enter new username", strlen("Enter new username"));
        read(client_fd, new_username, MAX_CHAR_LEN-1);
        write(client_fd, "Enter new password", strlen("Enter new password"));
        read(client_fd, new_password, MAX_CHAR_LEN-1);
        addNormal(new_username, new_password);
        write(client_fd, "New normal a/c created\nEnter ACK to proceed", strlen("New normal a/c created\nEnter ACK to proceed"));
        char ack[11];
        read(client_fd, ack, 10);
      }
      else if(!strcmp(rec, "2")){
        char new_keyword[MAX_CHAR_LEN], new_username1[MAX_CHAR_LEN], new_password1[MAX_CHAR_LEN], new_username2[MAX_CHAR_LEN], new_password2[MAX_CHAR_LEN];
        bzero(new_keyword, MAX_CHAR_LEN);
        bzero(new_username1, MAX_CHAR_LEN);bzero(new_password1, MAX_CHAR_LEN); bzero(new_username2, MAX_CHAR_LEN);bzero(new_password2, MAX_CHAR_LEN);
        write(client_fd, "Enter new keyword", strlen("Enter new keyword"));
        read(client_fd, new_keyword, MAX_CHAR_LEN-1);
        write(client_fd, "Enter new username1", strlen("Enter new username1"));
        read(client_fd, new_username1, MAX_CHAR_LEN-1);
        write(client_fd, "Enter new username2", strlen("Enter new username2"));
        read(client_fd, new_username2, MAX_CHAR_LEN-1);
        write(client_fd, "Enter new password1", strlen("Enter new password1"));
        read(client_fd, new_password1, MAX_CHAR_LEN-1);
        write(client_fd, "Enter new password2", strlen("Enter new password2"));
        read(client_fd, new_password2, MAX_CHAR_LEN-1);
        addJoint(new_keyword, new_username1, new_password1, new_username2, new_password2);
        write(client_fd, "new joint a/c created\nEnter ACK to proceed", strlen("new joint a/c created\nEnter ACK to proceed"));
        char ack[11];
        read(client_fd, ack, 10);
      }
      else if(!strcmp(rec, "3")){
        char new_username[MAX_CHAR_LEN], new_password[MAX_CHAR_LEN];
        bzero(new_username, MAX_CHAR_LEN); bzero(new_password, MAX_CHAR_LEN);
        write(client_fd, "Enter new username", strlen("Enter new username"));
        read(client_fd, new_username, MAX_CHAR_LEN-1);
        write(client_fd, "Enter new password", strlen("Enter new password"));
        read(client_fd, new_password, MAX_CHAR_LEN-1);
        addAdmin(new_username, new_password);
        write(client_fd, "new admin a/c created\nEnter ACK to proceed", strlen("new admin a/c created\nEnter ACK to proceed"));
        char ack[11];
        read(client_fd, ack, 10);
      }
      else if(!strcmp(rec, "4")){
        char new_username[MAX_CHAR_LEN];
        bzero(new_username, MAX_CHAR_LEN);
        write(client_fd, "Enter the username to be deleted", strlen("Enter the username to be deleted"));
        read(client_fd, new_username, MAX_CHAR_LEN-1);
        int norm_id = getIdNorm(new_username);
        if(norm_id==-1){
          write(client_fd, "No such a/c found\nEnter ACK to proceed", strlen("No such a/c found\nEnter ACK to proceed"));
        }
        else{
          deleteNorm(norm_id);
          write(client_fd, "Normal a/c deleted\nEnter ACK to proceed", strlen("Normal a/c deleted\nEnter ACK to proceed"));
        }
        char ack[11];
        read(client_fd, ack, 10);
      }
      else if(!strcmp(rec, "5")){
        char new_keyword[MAX_CHAR_LEN];bzero(new_keyword, MAX_CHAR_LEN);
        write(client_fd, "Enter the keyword of a/c to be deleted", strlen("Enter the keyword of the a/c to be deleted"));
        read(client_fd, new_keyword, MAX_CHAR_LEN-1);
        int joint_id = getIdJoint(new_keyword);
        if(joint_id==-1){
          write(client_fd, "No such a/c found\nEnter ACK to proceed", strlen("No such a/c found\nEnter ACK to proceed"));
        }
        else {
          deleteJoint(joint_id);
          write(client_fd, "Joint a/c deleted\nEnter ACK to proceed", strlen("Joint a/c deleted\nEnter ACK to proceed"));
        }
        char ack[11];
        read(client_fd, ack, 10);
      }
      else if(!strcmp(rec, "6")){
        char new_username[MAX_CHAR_LEN]; bzero(new_username, MAX_CHAR_LEN);
        write(client_fd, "Enter the username of the a/c to be deleted", strlen("Enter the username of the a/c to be deleted"));
        read(client_fd, new_username, MAX_CHAR_LEN-1);
        int admin_id = getIdAdmin(new_username);
        if(admin_id==-1){
          write(client_fd, "No such a/c found\nEnter ACK to proceed", strlen("No such a/c found\nEnter ACK to proceed"));
        }
        else {
          deleteAdmin(admin_id);
          write(client_fd, "Admin a/c deleted\nEnter ACK to proceed", strlen("Admin a/c deleted\nEnter ACK to proceed"));
        }
        char ack[11];
        read(client_fd, ack, 10);
      }
      else if(!strcmp(rec, "7")){
        char new_username[MAX_CHAR_LEN]; bzero(new_username, MAX_CHAR_LEN);
        write(client_fd, "Enter the username of the a/c to be searched", strlen("Enter the username of the a/c to be searched"));
        read(client_fd, new_username, MAX_CHAR_LEN-1);
        int norm_id = getIdNorm(new_username);
        if(norm_id==-1){
          write(client_fd, "No such a/c found\nEnter ACK to proceed", strlen("No such a/c found\nEnter ACK to proceed"));
        }
        else{
          struct Data dt = searchNorm(norm_id);
          char details[200];
          sprintf(details, "User id:%d\nUsername: %s\nBalance amt: %ld\nEnter ACK to proceed\n", dt.id, dt.user.username, dt.balance);
          write(client_fd, details, strlen(details));
        }
        char ack[11];
        read(client_fd, ack, 10);
      }
      else if(!strcmp(rec, "8")){
        char new_keyword[MAX_CHAR_LEN];bzero(new_keyword, MAX_CHAR_LEN);
        write(client_fd, "Enter the keyword of the a/c to be searched", strlen("Enter the keyword of the a/c to be searched"));
        read(client_fd, new_keyword, MAX_CHAR_LEN-1);
        int joint_id = getIdJoint(new_keyword);
        if(joint_id==-1){
          write(client_fd, "No such a/c found\nEnter ACK to proceed", strlen("No such a/c found\nEnter ACK to proceed"));
        }
        else{
          struct JointData jdt = searchJoint(joint_id);
          char details[200];
          sprintf(details, "User id:%d\nKeyword: %s\nUsername1: %s, Username2: %s\nBalance amt: %ld\nEnter ACK to proceed\n", jdt.id, jdt.keyword, jdt.user1.username, jdt.user2.username, jdt.balance);
          write(client_fd, details, strlen(details));
        }
        char ack[11];
        read(client_fd, ack, 10);
      }
      else if(!strcmp(rec, "9")){
        char new_username[MAX_CHAR_LEN];bzero(new_username, MAX_CHAR_LEN);
        write(client_fd, "Enter the username of the a/c to be searched", strlen("Enter the username of the a/c to be searched"));
        read(client_fd, new_username, MAX_CHAR_LEN-1);
        int admin_id = getIdAdmin(new_username);
        if(admin_id==-1){
          write(client_fd, "No such a/c found\nEnter ACK to proceed", strlen("No such a/c found\nEnter ACK to proceed"));
        }
        else{
          struct Data dt = searchAdmin(admin_id);
          char details[120];
          sprintf(details, "User id:%d\nUsername: %s\nBalance amt: %ld\nEnter ACK to proceed\n", dt.id, dt.user.username, dt.balance);
          write(client_fd, details, strlen(details));
        }
        char ack[11];
        read(client_fd, ack, 10);
      }
      else if(!strcmp(rec, "10")){
        char new_username[MAX_CHAR_LEN], new_password[MAX_CHAR_LEN], old_username[MAX_CHAR_LEN];
        bzero(new_username, MAX_CHAR_LEN);bzero(new_password, MAX_CHAR_LEN);bzero(old_username, MAX_CHAR_LEN);
        write(client_fd, "Enter the old username", strlen("Enter the old username"));
        read(client_fd, old_username, MAX_CHAR_LEN-1);
        write(client_fd, "Enter new username", strlen("Enter new username"));
        read(client_fd, new_username, MAX_CHAR_LEN-1);
        write(client_fd, "Enter new password", strlen("Enter new password"));
        read(client_fd, new_password, MAX_CHAR_LEN-1);
        int norm_id = getIdNorm(old_username);
        if(norm_id==-1){
          write(client_fd, "No such a/c found\nEnter ACK to proceed", strlen("No such a/c found\nEnter ACK to proceed"));
        }
        else{
          modifyNorm(norm_id, new_username, new_password);
          write(client_fd, "Normal a/c is modified\nEnter ACK to proceed", strlen("Normal a/c is modified\nEnter ACK to proceed"));
        }
        char ack[11];
        read(client_fd, ack, 10);
      }
      else if(!strcmp(rec, "11")){
        char old_keyword[MAX_CHAR_LEN], new_keyword[MAX_CHAR_LEN], new_username1[MAX_CHAR_LEN], new_password1[MAX_CHAR_LEN], new_username2[MAX_CHAR_LEN], new_password2[MAX_CHAR_LEN];
        bzero(old_keyword, MAX_CHAR_LEN);bzero(new_keyword, MAX_CHAR_LEN); bzero(new_username1, MAX_CHAR_LEN); bzero(new_password1, MAX_CHAR_LEN); bzero(new_username2, MAX_CHAR_LEN);
        bzero(new_password2, MAX_CHAR_LEN);
        write(client_fd, "Enter the old keyword of the a/c", strlen("Enter the old keyword of the a/c"));
        read(client_fd, old_keyword, MAX_CHAR_LEN-1);
        write(client_fd, "Enter new keyword", strlen("Enter new keyword"));
        read(client_fd, new_keyword, MAX_CHAR_LEN-1);
        write(client_fd, "Enter new username1", strlen("Enter new username1"));
        read(client_fd, new_username1, MAX_CHAR_LEN-1);
        write(client_fd, "Enter new username2", strlen("Enter new username2"));
        read(client_fd, new_username2, MAX_CHAR_LEN-1);
        write(client_fd, "Enter new password1", strlen("Enter new password1"));
        read(client_fd, new_password1, MAX_CHAR_LEN-1);
        write(client_fd, "Enter new password2", strlen("Enter new password2"));
        read(client_fd, new_password2, MAX_CHAR_LEN-1);
        int joint_id = getIdJoint(old_keyword);
        if(joint_id==-1){
          write(client_fd, "No such a/c found\nEnter ACK to proceed", strlen("No such a/c found\nEnter ACK to proceed"));
        }
        else{
          modifyJoint(joint_id, new_keyword, new_username1, new_password1, new_username2, new_password2);
          write(client_fd, "Joint a/c modified\nEnter ACK to proceed", strlen("Joint a/c modified\nEnter ACK to proceed"));
        }
        char ack[11];
        read(client_fd, ack, 10);
      }
      else if(!strcmp(rec, "12")){
        char new_username[MAX_CHAR_LEN], new_password[MAX_CHAR_LEN], old_username[MAX_CHAR_LEN];
        bzero(new_username, MAX_CHAR_LEN); bzero(new_password, MAX_CHAR_LEN); bzero(old_username, MAX_CHAR_LEN);
        write(client_fd, "Enter the old username", strlen("Enter the old username"));
        read(client_fd, old_username, MAX_CHAR_LEN-1);
        write(client_fd, "Enter new username", strlen("Enter new username"));
        read(client_fd, new_username, MAX_CHAR_LEN-1);
        write(client_fd, "Enter new password", strlen("Enter new password"));
        read(client_fd, new_password, MAX_CHAR_LEN-1);
        int admin_id = getIdAdmin(old_username);
        if(admin_id==-1){
          write(client_fd, "No such a/c found\nEnter ACK to proceed", strlen("No such a/c found\nEnter ACK to proceed"));
        }
        else{
          modifyAdmin(admin_id, new_username, new_password);
          write(client_fd, "Admin a/c modified\nEnter ACK to proceed", strlen("Admin a/c modified\nEnter ACK to proceed"));
        }
        char ack[11];
        read(client_fd, ack, 10);
      }
    }
  }

  close(client_fd);


  pthread_exit(NULL);
}
