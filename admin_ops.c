
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

// ----- Add account operation------------------------
int addNormal(char* username, char* password){
  struct Data dt;
  strcpy(dt.user.username, username);
  strcpy(dt.user.password, password);
  dt.balance = 0;


  // semaphore 1
  int key = ftok(".", 'a');
  int semid = semget(key, 0, 0);
  struct sembuf semlock = {1, -1, SEM_UNDO}; // using 1st semaphore
  semop(semid, &semlock, 1);

  int lst_reusable_id = -1;
  for(int i=0; i<MAX_USERS; i++){
    if(userIdTable[NORMAL_USER][i]==NULL){
      userIdTable[NORMAL_USER][i] = (char*)malloc(MAX_CHAR_LEN*sizeof(char));
      strcpy(userIdTable[NORMAL_USER][i], dt.user.username); // update the table
      lst_reusable_id = i;
      break;
    }
  }

  dt.id = lst_reusable_id;
  semlock.sem_op = 1;
  semop(semid, &semlock, 1);

  // write to Normal account database
  int norm_fd = open("Normal_account.txt", O_RDWR);
  // lock the record in the file
  struct flock recordlock;
  recordlock.l_type = F_WRLCK;
  recordlock.l_whence = SEEK_SET;
  recordlock.l_start = dt.id*sizeof(struct Data);
  recordlock.l_len = sizeof(struct Data);
  recordlock.l_pid = getpid();
  fcntl(norm_fd, F_SETLKW, &recordlock);

  // move the file descriptor
  lseek(norm_fd, recordlock.l_start, SEEK_SET);

  // write the new account into database file
  write(norm_fd, &dt, sizeof(struct Data));

  // unlock
  recordlock.l_type = F_UNLCK;
  fcntl(norm_fd, F_SETLK, &recordlock);
  close(norm_fd);

  return 0;
}

int addJoint(char* keyword, char* username1, char* password1, char* username2, char* password2){
  struct JointData jdt;
  strncpy(jdt.keyword, keyword, MAX_CHAR_LEN-1);
  strncpy(jdt.user1.username, username1, MAX_CHAR_LEN-1);
  strncpy(jdt.user1.password, password1, MAX_CHAR_LEN-1);
  strncpy(jdt.user2.username, username2, MAX_CHAR_LEN-1);
  strncpy(jdt.user2.password, password2, MAX_CHAR_LEN-1);
  jdt.balance = 0;

  // semaphore 2
  int key = ftok(".", 'a');
  int semid = semget(key, 0, 0);
  struct sembuf semlock = {2, -1, SEM_UNDO}; // using 2nd semaphore
  semop(semid, &semlock, 1);

  int lst_reusable_id = -1;
  for(int i=0; i<MAX_USERS; i++){
    if(userIdTable[JOINT_USER][i]==NULL){
      userIdTable[JOINT_USER][i] = (char*)malloc(MAX_CHAR_LEN*sizeof(char));
      strncpy(userIdTable[JOINT_USER][i], jdt.keyword, MAX_CHAR_LEN-1);
      lst_reusable_id = i;
      break;
    }
  }

  jdt.id = lst_reusable_id;
  semlock.sem_op = 1;
  semop(semid, &semlock, 1);

  // write to Joint account database file
  int joint_fd = open("Joint_account.txt", O_RDWR);
  // lock the record in the file
  struct flock recordlock;
  recordlock.l_type = F_WRLCK;
  recordlock.l_whence = SEEK_SET;
  recordlock.l_start = jdt.id*sizeof(struct JointData);
  recordlock.l_len = sizeof(struct JointData);
  recordlock.l_pid = getpid();
  fcntl(joint_fd, F_SETLKW, &recordlock);

  // move the file descriptor
  lseek(joint_fd, recordlock.l_start, SEEK_SET);

  // write the new account record into database file
  write(joint_fd, &jdt, sizeof(struct JointData));

  // unlock
  recordlock.l_type = F_UNLCK;
  fcntl(joint_fd, F_SETLK, &recordlock);
  close(joint_fd);

  return 0;
}

int addAdmin(char* username, char* password){
  struct Data dt;
  strncpy(dt.user.username, username, MAX_CHAR_LEN-1);
  strncpy(dt.user.password, password, MAX_CHAR_LEN-1);
  dt.balance = 0;



  // semaphore 3
  int key = ftok(".", 'a');
  int semid = semget(key, 0, 0);
  struct sembuf semlock = {3, -1, SEM_UNDO}; // using 3rd semaphore
  semop(semid, &semlock, 1);

  int lst_reusable_id = -1;
  for(int i=0; i<MAX_USERS; i++){
    if(userIdTable[ADMIN_USER][i]==NULL){
      userIdTable[ADMIN_USER][i] = (char*)malloc(MAX_CHAR_LEN*sizeof(char));
      strncpy(userIdTable[ADMIN_USER][i], dt.user.username, MAX_CHAR_LEN-1); // update the table
      lst_reusable_id = i;
      break;
    }
  }

  dt.id = lst_reusable_id;
  semlock.sem_op = 1;
  semop(semid, &semlock, 1);

  // write to Admin account database
  int admin_fd = open("Admin_account.txt", O_RDWR);
  // lock the record in the file
  struct flock recordlock;
  recordlock.l_type = F_WRLCK;
  recordlock.l_whence = SEEK_SET;
  recordlock.l_start = dt.id*sizeof(struct Data);
  recordlock.l_len = sizeof(struct Data);
  recordlock.l_pid = getpid();
  fcntl(admin_fd, F_SETLKW, &recordlock);

  // move the file descriptor
  lseek(admin_fd, recordlock.l_start, SEEK_SET);

  // write the new account into database file
  write(admin_fd, &dt, sizeof(struct Data));

  // unlock
  recordlock.l_type = F_UNLCK;
  fcntl(admin_fd, F_SETLK, &recordlock);
  close(admin_fd);

  return 1;
}

// ----------------- Delete Operations ----------------------
int deleteNorm(int id){
  // lock semaphore 1
  int key = ftok(".", 'a');
  int semid = semget(key, 0, 0);
  struct sembuf semlock = {1, -1, SEM_UNDO}; // using 1st semaphore
  semop(semid, &semlock, 1);

  free(userIdTable[NORMAL_USER][id]);
  userIdTable[NORMAL_USER][id] = NULL;

  // unlock semaphore 1

  semlock.sem_op = 1;
  semop(semid, &semlock, 1);

  return 0;
}

int deleteJoint(int id){
  // lock semaphore 2
  int key = ftok(".", 'a');
  int semid = semget(key, 0, 0);
  struct sembuf semlock = {2, -1, SEM_UNDO}; // using 2n semaphore
  semop(semid, &semlock, 1);

  free(userIdTable[JOINT_USER][id]);
  userIdTable[JOINT_USER][id] = NULL;

  // unlock semaphore 2
  semlock.sem_op = 1;
  semop(semid, &semlock, 1);

  return 0;
}

int deleteAdmin(int id){
  // lock semaphore 3

  int key = ftok(".", 'a');
  int semid = semget(key, 0, 0);
  struct sembuf semlock = {3, -1, SEM_UNDO}; // using 3rd semaphore
  semop(semid, &semlock, 1);

  free(userIdTable[ADMIN_USER][id]);
  userIdTable[ADMIN_USER][id] = NULL;

  // unlock semaphore 3
  semlock.sem_op = 1;
  semop(semid, &semlock, 1);

  return 0;
}

// ------------- Search Operations --------------------
struct Data searchNorm(int id){

  struct Data dt;

  // reading from database file
  int norm_fd = open("Normal_account.txt", O_RDWR);
  // lock the record in the file
  struct flock recordlock;
  recordlock.l_type = F_RDLCK; // read lock
  recordlock.l_whence = SEEK_SET;
  recordlock.l_start = id*sizeof(struct Data);
  recordlock.l_len = sizeof(struct Data);
  recordlock.l_pid = getpid();
  fcntl(norm_fd, F_SETLKW, &recordlock);

  // move the file descriptor
  lseek(norm_fd, recordlock.l_start, SEEK_SET);

  // write the new account into database file
  read(norm_fd, &dt, sizeof(struct Data));

  // unlock
  recordlock.l_type = F_UNLCK;
  fcntl(norm_fd, F_SETLK, &recordlock);
  close(norm_fd);

  return dt;
}

struct JointData searchJoint(int id){
  struct JointData jdt;

  // reading from database file
  int joint_fd = open("Joint_account.txt", O_RDWR);
  // lock the record in the file
  struct flock recordlock;
  recordlock.l_type = F_RDLCK; // read lock
  recordlock.l_whence = SEEK_SET;
  recordlock.l_start = id*sizeof(struct JointData);
  recordlock.l_len = sizeof(struct JointData);
  recordlock.l_pid = getpid();
  fcntl(joint_fd, F_SETLKW, &recordlock);

  // move the file descriptor
  lseek(joint_fd, recordlock.l_start, SEEK_SET);

  // write the new account into database file
  read(joint_fd, &jdt, sizeof(struct JointData));

  // unlock
  recordlock.l_type = F_UNLCK;
  fcntl(joint_fd, F_SETLK, &recordlock);
  close(joint_fd);

  return jdt;
}

struct Data searchAdmin(int id){
  struct Data dt;

  // reading from database file
  int admin_fd = open("Admin_account.txt", O_RDWR);
  // lock the record in the file
  struct flock recordlock;
  recordlock.l_type = F_RDLCK; // read lock
  recordlock.l_whence = SEEK_SET;
  recordlock.l_start = id*sizeof(struct Data);
  recordlock.l_len = sizeof(struct Data);
  recordlock.l_pid = getpid();
  fcntl(admin_fd, F_SETLKW, &recordlock);

  // move the file descriptor
  lseek(admin_fd, recordlock.l_start, SEEK_SET);

  // write the new account into database file
  read(admin_fd, &dt, sizeof(struct Data));

  // unlock
  recordlock.l_type = F_UNLCK;
  fcntl(admin_fd, F_SETLK, &recordlock);
  close(admin_fd);

  return dt;

}


// ---------------- Modify Operations ----------------------
int modifyNorm(int id, char* username, char* password){
  // functionality: Semapore lock -> search + add -> semahpore unlock
  // use semaphore exclusive to modification functions

  // 4th semaphore (exclusive to modification operation)
  int key = ftok(".", 'a');
  int semid = semget(key, 0, 0);
  struct sembuf semlock = {4, -1, SEM_UNDO}; // 4th semaphore
  semop(semid, &semlock, 1);

  struct Data dt = searchNorm(id);
  strncpy(dt.user.username, username, MAX_CHAR_LEN-1);
  strncpy(dt.user.password, password, MAX_CHAR_LEN-1);

  // updating the record in the database
  struct sembuf semlock_addop = {1, -1, SEM_UNDO}; //locking 1st semaphore
  semop(semid, &semlock_addop, 1);

  strncpy(userIdTable[NORMAL_USER][id], username, MAX_CHAR_LEN-1);

  // unlock 1st semaphore
  semlock_addop.sem_op = 1;
  semop(semid, &semlock_addop, 1);

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

int modifyJoint(int id, char* keyword, char* username1, char* password1, char* username2, char* password2){

    // 4th semaphore (exclusive to modification operation)
    int key = ftok(".", 'a');
    int semid = semget(key, 0, 0);
    struct sembuf semlock = {4, -1, SEM_UNDO}; // 4th semaphore
    semop(semid, &semlock, 1);

    struct JointData jdt = searchJoint(id);
    strncpy(jdt.keyword, keyword, MAX_CHAR_LEN-1);
    strncpy(jdt.user1.username, username1, MAX_CHAR_LEN-1);
    strncpy(jdt.user1.password, password1, MAX_CHAR_LEN-1);
    strncpy(jdt.user2.username, username2, MAX_CHAR_LEN-1);
    strncpy(jdt.user2.password, password2, MAX_CHAR_LEN-1);

    // updating the record in the database
    struct sembuf semlock_addop = {2, -1, SEM_UNDO}; //locking 2nd semaphore (for joint a/c)
    semop(semid, &semlock_addop, 1);

    strncpy(userIdTable[JOINT_USER][id], jdt.keyword, MAX_CHAR_LEN-1);

    // unlock 2nd semaphore
    semlock_addop.sem_op = 1;
    semop(semid, &semlock_addop, 1);

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

int modifyAdmin(int id, char* username, char* password){

    // 4th semaphore (exclusive to modification operation)
    int key = ftok(".", 'a');
    int semid = semget(key, 0, 0);
    struct sembuf semlock = {4, -1, SEM_UNDO}; // 4th semaphore
    semop(semid, &semlock, 1);

    struct Data dt = searchAdmin(id);
    strncpy(dt.user.username, username, MAX_CHAR_LEN-1);
    strncpy(dt.user.password, password, MAX_CHAR_LEN-1);

    // updating the record in the database
    struct sembuf semlock_addop = {3, -1, SEM_UNDO}; //locking 3rd semaphore (for admin a/c)
    semop(semid, &semlock_addop, 1);

    strncpy(userIdTable[ADMIN_USER][id], username, MAX_CHAR_LEN-1);

    // unlock 3rd semaphore
    semlock_addop.sem_op = 1;
    semop(semid, &semlock_addop, 1);

    // write lock the database record space
    int admin_fd = open("Admin_account.txt", O_RDWR);

    struct flock recordlock;
    recordlock.l_type = F_WRLCK;
    recordlock.l_whence = SEEK_SET;
    recordlock.l_start = id*sizeof(struct Data);
    recordlock.l_len = sizeof(struct Data);
    recordlock.l_pid = getpid();
    fcntl(admin_fd, F_SETLKW, &recordlock);

    lseek(admin_fd, recordlock.l_start, SEEK_SET);

    write(admin_fd, &dt, sizeof(struct Data));

    // unlock the database record space
    recordlock.l_type = F_UNLCK;
    fcntl(admin_fd, F_SETLK, &recordlock);

    // unlock 4th semaphore (exclusive to modification operation)
    semlock.sem_op = 1;
    semop(semid, &semlock, 1);

    return 0;
}
