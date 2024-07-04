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

// ------ Search related operations --------------
long int balanceEnquiry(int id, int type){
  long int bal = -1;
  if(type==NORMAL_USER){
    struct Data dt = searchNorm(id);
    bal = dt.balance;
  }
  else if(type==JOINT_USER){
    struct JointData jdt = searchJoint(id);
    bal = jdt.balance;
  }

  return bal;
}


struct Data viewDetailsNorm(int id){
  return searchNorm(id);
}

struct JointData viewDetailsJoint(int id){
  // search ops will take care of synchronizations if needed
  return searchJoint(id);
}

// ---------- Modify related operations -----------
int passwordChangeNorm(int id, char* password){
  // 4th semaphore (exclusive to modification operation)
  int key = ftok(".", 'a');
  int semid = semget(key, 0, 0);
  struct sembuf semlock = {4, -1, SEM_UNDO}; // 4th semaphore
  semop(semid, &semlock, 1);

  struct Data dt = searchNorm(id);
  strncpy(dt.user.password, password, MAX_CHAR_LEN-1); // only password is changed


  // write lock the database record space
  int norm_fd = open("Normal_account.txt", O_RDWR);

  struct flock recordlock;
  recordlock.l_type = F_WRLCK;
  recordlock.l_whence = SEEK_SET;
  recordlock.l_start = id*sizeof(struct Data);
  recordlock.l_len = sizeof(struct Data);
  recordlock.l_pid = getpid();
  fcntl(norm_fd, F_SETLKW, &recordlock);

  lseek(norm_fd, recordlock.l_start, SEEK_SET);

  write(norm_fd, &dt, sizeof(struct Data));

  // unlock the database record space
  recordlock.l_type = F_UNLCK;
  fcntl(norm_fd, F_SETLK, &recordlock);

  // unlock 4th semaphore (exclusive to modification operation)
  semlock.sem_op = 1;
  semop(semid, &semlock, 1);

  return 0;
}

int passwordChangeJoint(int id, char* password, char* cur_username){

  // 4th semaphore (exclusive to modification operation)
  int key = ftok(".", 'a');
  int semid = semget(key, 0, 0);
  struct sembuf semlock = {4, -1, SEM_UNDO}; // 4th semaphore
  semop(semid, &semlock, 1);

  struct JointData jdt = searchJoint(id);
  if(!strcmp(jdt.user1.username, cur_username)){
    strncpy(jdt.user1.password, password, MAX_CHAR_LEN-1);
  }
  else if(!strcmp(jdt.user2.username, cur_username)){
    strncpy(jdt.user2.password, password, MAX_CHAR_LEN-1);
  }


  // write lock the database record space
  int joint_fd = open("Joint_account.txt", O_RDWR);

  struct flock recordlock;
  recordlock.l_type = F_WRLCK;
  recordlock.l_whence = SEEK_SET;
  recordlock.l_start = id*sizeof(struct JointData);
  recordlock.l_len = sizeof(struct JointData);
  recordlock.l_pid = getpid();
  fcntl(joint_fd, F_SETLKW, &recordlock);

  lseek(joint_fd, recordlock.l_start, SEEK_SET);

  write(joint_fd, &jdt, sizeof(struct JointData));

  // unlock the database record space
  recordlock.l_type = F_UNLCK;
  fcntl(joint_fd, F_SETLK, &recordlock);

  // unlock 4th semaphore (exclusive to modification operation)
  semlock.sem_op = 1;
  semop(semid, &semlock, 1);

  return 0;
}

int depositAmtNorm(int id, long int amt){
  // 4th semaphore (exclusive to modification operation)
  int key = ftok(".", 'a');
  int semid = semget(key, 0, 0);
  struct sembuf semlock = {4, -1, SEM_UNDO}; // 4th semaphore
  semop(semid, &semlock, 1);

  struct Data dt = searchNorm(id);
  dt.balance += amt; // balance is increased by amt

  // write lock the database record space
  int norm_fd = open("Normal_account.txt", O_RDWR);

  struct flock recordlock;
  recordlock.l_type = F_WRLCK;
  recordlock.l_whence = SEEK_SET;
  recordlock.l_start = id*sizeof(struct Data);
  recordlock.l_len = sizeof(struct Data);
  recordlock.l_pid = getpid();
  fcntl(norm_fd, F_SETLKW, &recordlock);

  lseek(norm_fd, recordlock.l_start, SEEK_SET);

  write(norm_fd, &dt, sizeof(struct Data));

  // unlock the database record space
  recordlock.l_type = F_UNLCK;
  fcntl(norm_fd, F_SETLK, &recordlock);

  // unlock 4th semaphore (exclusive to modification operation)
  semlock.sem_op = 1;
  semop(semid, &semlock, 1);

  return 0;
}

int depositAmtJoint(int id, long int amt){

  // 4th semaphore (exclusive to modification operation)
  int key = ftok(".", 'a');
  int semid = semget(key, 0, 0);
  struct sembuf semlock = {4, -1, SEM_UNDO}; // 4th semaphore
  semop(semid, &semlock, 1);

  struct JointData jdt = searchJoint(id);
  jdt.balance += amt; // only balance is increased


  // write lock the database record space
  int joint_fd = open("Joint_account.txt", O_RDWR);

  struct flock recordlock;
  recordlock.l_type = F_WRLCK;
  recordlock.l_whence = SEEK_SET;
  recordlock.l_start = id*sizeof(struct JointData);
  recordlock.l_len = sizeof(struct JointData);
  recordlock.l_pid = getpid();
  fcntl(joint_fd, F_SETLKW, &recordlock);

  lseek(joint_fd, recordlock.l_start, SEEK_SET);

  write(joint_fd, &jdt, sizeof(struct JointData));

  // unlock the database record space
  recordlock.l_type = F_UNLCK;
  fcntl(joint_fd, F_SETLK, &recordlock);

  // unlock 4th semaphore (exclusive to modification operation)
  semlock.sem_op = 1;
  semop(semid, &semlock, 1);

  return 0;
}

int withdrawAmtNorm(int id, long int amt){
  // 4th semaphore (exclusive to modification operation)
  int key = ftok(".", 'a');
  int semid = semget(key, 0, 0);
  struct sembuf semlock = {4, -1, SEM_UNDO}; // 4th semaphore
  semop(semid, &semlock, 1);

  struct Data dt = searchNorm(id);
  if(dt.balance-amt<0)
    return -1; // error in withdrawal process. Not sufficient amt in the a/c
  dt.balance -= amt; // balance is decreased by amt


  // write lock the database record space
  int norm_fd = open("Normal_account.txt", O_RDWR);

  struct flock recordlock;
  recordlock.l_type = F_WRLCK;
  recordlock.l_whence = SEEK_SET;
  recordlock.l_start = id*sizeof(struct Data);
  recordlock.l_len = sizeof(struct Data);
  recordlock.l_pid = getpid();
  fcntl(norm_fd, F_SETLKW, &recordlock);

  lseek(norm_fd, recordlock.l_start, SEEK_SET);

  write(norm_fd, &dt, sizeof(struct Data));

  // unlock the database record space
  recordlock.l_type = F_UNLCK;
  fcntl(norm_fd, F_SETLK, &recordlock);

  // unlock 4th semaphore (exclusive to modification operation)
  semlock.sem_op = 1;
  semop(semid, &semlock, 1);

  return 0;
}

int withdrawAmtJoint(int id, long int amt){

  // 4th semaphore (exclusive to modification operation)
  int key = ftok(".", 'a');
  int semid = semget(key, 0, 0);
  struct sembuf semlock = {4, -1, SEM_UNDO}; // 4th semaphore
  semop(semid, &semlock, 1);

  struct JointData jdt = searchJoint(id);
  if(jdt.balance - amt<0)
    return -1;          // illegal withdrawal. Not sufficient amt in the a/c

  jdt.balance -= amt; // only balance is increased


  // write lock the database record space
  int joint_fd = open("Joint_account.txt", O_RDWR);

  struct flock recordlock;
  recordlock.l_type = F_WRLCK;
  recordlock.l_whence = SEEK_SET;
  recordlock.l_start = id*sizeof(struct JointData);
  recordlock.l_len = sizeof(struct JointData);
  recordlock.l_pid = getpid();
  fcntl(joint_fd, F_SETLKW, &recordlock);

  lseek(joint_fd, recordlock.l_start, SEEK_SET);

  write(joint_fd, &jdt, sizeof(struct JointData));

  // unlock the database record space
  recordlock.l_type = F_UNLCK;
  fcntl(joint_fd, F_SETLK, &recordlock);

  // unlock 4th semaphore (exclusive to modification operation)
  semlock.sem_op = 1;
  semop(semid, &semlock, 1);

  return 0;
}
